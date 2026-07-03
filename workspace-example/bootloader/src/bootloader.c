/*
 * bootloader.c — RISC-V MCU on Cmod A7 : UART flash bootloader
 *
 * Lives in BRAM (part of the bitstream, restored on every power-on).
 * At power-on it either:
 *   (a) receives a new application over UART and programs it into QSPI flash, or
 *   (b) loads the application image from flash into SRAM and jumps to it.
 *
 * Flash layout:
 *   0x000000 - 0x21FFFF : FPGA bitstream (NEVER touched by this code)
 *   0x300000            : application slot:
 *                         16-byte header  [magic|size|entry|crc32]
 *                         followed by the raw binary image (max 512 KB)
 *
 * UART protocol (115200 8N1, PC side: tools/upload.py):
 *   PC -> 'U' (sync, repeated)         BL -> 'K'
 *   PC -> 'H' size32 entry32 crc32     BL -> 'K' | 'E'
 *   PC -> 'D' len16 data[len] crc32    BL -> 'K' | 'E'   (repeat until size bytes)
 *   PC -> 'G'  = program flash + run   BL -> 'K' (after verify) then jumps
 *   PC -> 'R'  = run from RAM only     BL -> 'K' then jumps (nothing written)
 *
 * The image is received directly into SRAM at its final address (0x60000000),
 * so 'G' only has to copy SRAM -> flash, and 'R' costs nothing.
 */

#include <stdint.h>
#include "xparameters.h"
#include "xspi.h"
#include "xuartns550_l.h"
#include "xil_io.h"

/* ---- fixed platform addresses (see xparameters.h) ---- */
#define UART_BASE       XPAR_UART_USB_BASEADDR      /* 16550, USB channel   */
#define LED_BASE        0x40000000u                 /* board_led_2bits      */
#define BTN_BASE        0x40010000u                 /* board_button         */

#define SRAM_BASE       0x60000000u
#define APP_MAX_SIZE    (512u * 1024u)
#define FLASH_APP_OFF   0x300000u                   /* application slot     */
#define FLASH_PROT_LIM  0x220000u                   /* never write below    */

#define HDR_MAGIC       0x52564D43u                 /* "RVMC"               */
#define BAUD            115200u
#define SYNC_WINDOW_MS  1000u                       /* listen after power-on */

/* ---- MX25L3233F / N25Q032A common SPI commands ---- */
#define CMD_WREN        0x06
#define CMD_RDSR        0x05
#define CMD_FAST_READ   0x0B    /* 1 dummy byte, ok at 50 MHz SCK */
#define CMD_PAGE_PROG   0x02    /* 256-byte pages                 */
#define CMD_BLOCK_ERASE 0xD8    /* 64 KB blocks                   */

static XSpi Spi;
static uint8_t spi_tx[260 + 5], spi_rx[260 + 5];

/* ---------------- tiny helpers ---------------- */

static inline uint64_t rdcycle(void)
{
    uint32_t lo, hi;
    __asm__ volatile("csrr %0, mcycle" : "=r"(lo));
    __asm__ volatile("csrr %0, mcycleh" : "=r"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static void led(uint32_t v) { Xil_Out32(LED_BASE, v & 3); }

static void putb(uint8_t c) { XUartNs550_SendByte(UART_BASE, c); }

/* blocking receive with optional deadline (0 = wait forever); -1 on timeout */
static int getb(uint64_t deadline)
{
    while (!XUartNs550_IsReceiveData(UART_BASE))
        if (deadline && rdcycle() > deadline)
            return -1;
    return XUartNs550_RecvByte(UART_BASE);
}

static uint32_t get_u32(void)          /* little-endian */
{
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v |= (uint32_t)getb(0) << (8 * i);
    return v;
}

/* CRC-32 (IEEE 802.3, bitwise — small over fast, this is a bootloader) */
static uint32_t crc32(const uint8_t *p, uint32_t n)
{
    uint32_t c = 0xFFFFFFFFu;
    while (n--) {
        c ^= *p++;
        for (int k = 0; k < 8; k++)
            c = (c >> 1) ^ (0xEDB88320u & (-(int32_t)(c & 1)));
    }
    return ~c;
}

/* ---------------- SPI flash driver (polled, x1 wide) ---------------- */

static void spi_xfer(uint32_t n)
{
    XSpi_SetSlaveSelect(&Spi, 1);
    XSpi_Transfer(&Spi, spi_tx, spi_rx, n);
    XSpi_SetSlaveSelect(&Spi, 0);
}

static void flash_wait_idle(void)
{
    do {
        spi_tx[0] = CMD_RDSR; spi_tx[1] = 0xFF;
        spi_xfer(2);
    } while (spi_rx[1] & 0x01);                    /* WIP bit */
}

static void flash_wren(void) { spi_tx[0] = CMD_WREN; spi_xfer(1); }

static void flash_addr(uint32_t a) /* 24-bit address, MSB first */
{
    spi_tx[1] = a >> 16; spi_tx[2] = a >> 8; spi_tx[3] = a;
}

static void flash_erase_64k(uint32_t addr)
{
    flash_wren();
    spi_tx[0] = CMD_BLOCK_ERASE; flash_addr(addr);
    spi_xfer(4);
    flash_wait_idle();
}

static void flash_prog_page(uint32_t addr, const uint8_t *d, uint32_t n)
{
    flash_wren();
    spi_tx[0] = CMD_PAGE_PROG; flash_addr(addr);
    for (uint32_t i = 0; i < n; i++) spi_tx[4 + i] = d[i];
    spi_xfer(4 + n);
    flash_wait_idle();
}

static void flash_read(uint32_t addr, uint8_t *d, uint32_t n)
{
    while (n) {                                    /* buffer-sized chunks */
        uint32_t c = n > 256 ? 256 : n;
        spi_tx[0] = CMD_FAST_READ; flash_addr(addr); spi_tx[4] = 0; /* dummy */
        spi_xfer(5 + c);
        for (uint32_t i = 0; i < c; i++) *d++ = spi_rx[5 + i];
        addr += c; n -= c;
    }
}

/* program [SRAM image, size] into the app slot and verify by read-back CRC */
static int flash_write_app(uint32_t size, uint32_t entry, uint32_t crc)
{
    uint32_t total = 16 + size;                    /* header + image        */
    if (FLASH_APP_OFF < FLASH_PROT_LIM) return 0;  /* paranoia              */

    for (uint32_t a = 0; a < total; a += 0x10000)  /* 64 KB erase blocks    */
        flash_erase_64k(FLASH_APP_OFF + a);

    /* Page programming must never cross a 256 B page boundary (it wraps!).
     * First page = 16 B header + first 240 B of image, so everything after
     * it lands page-aligned at FLASH_APP_OFF + 256, 512, ...              */
    const uint8_t *src = (const uint8_t *)SRAM_BASE;
    uint8_t page[256];
    uint32_t *h = (uint32_t *)page;
    h[0] = HDR_MAGIC; h[1] = size; h[2] = entry; h[3] = crc;
    uint32_t first = size < 240 ? size : 240;
    for (uint32_t i = 0; i < first; i++) page[16 + i] = src[i];
    flash_prog_page(FLASH_APP_OFF, page, 16 + first);

    for (uint32_t a = first; a < size; a += 256) {
        uint32_t n = size - a > 256 ? 256 : size - a;
        flash_prog_page(FLASH_APP_OFF + 16 + a, src + a, n);
    }

    /* verify: read image back into the upper half of SRAM and CRC it      */
    uint8_t *tmp = (uint8_t *)(SRAM_BASE);         /* re-read over itself   */
    flash_read(FLASH_APP_OFF + 16, tmp, size);
    return crc32(tmp, size) == crc;
}

/* ---------------- jump to application ---------------- */

static void run_app(uint32_t entry)
{
    led(0x2);
    __asm__ volatile("fence.i");                   /* I-cache sees new code */
    ((void (*)(void))entry)();
    while (1) ;                                    /* app returned: hang    */
}

/* try to boot the app image stored in flash; returns only on failure */
static void try_boot_from_flash(void)
{
    uint32_t hdr[4];
    flash_read(FLASH_APP_OFF, (uint8_t *)hdr, 16);
    if (hdr[0] != HDR_MAGIC || hdr[1] == 0 || hdr[1] > APP_MAX_SIZE)
        return;                                    /* no valid app          */
    flash_read(FLASH_APP_OFF + 16, (uint8_t *)SRAM_BASE, hdr[1]);
    if (crc32((const uint8_t *)SRAM_BASE, hdr[1]) != hdr[3])
        return;                                    /* corrupted             */
    run_app(hdr[2]);
}

/* ---------------- UART upload protocol ---------------- */

static void serve_upload(void)
{
    uint32_t size = 0, entry = SRAM_BASE, crc = 0, got = 0;

    putb('K');                                     /* sync acknowledged     */
    while (1) {
        int cmd = getb(0);
        switch (cmd) {
        case 'U': putb('K'); break;                /* extra syncs are fine  */

        case 'H':
            size  = get_u32();
            entry = get_u32();
            crc   = get_u32();
            got   = 0;
            putb(size > 0 && size <= APP_MAX_SIZE ? 'K' : 'E');
            break;

        case 'D': {                                /* data chunk            */
            uint32_t len = (uint32_t)getb(0) | ((uint32_t)getb(0) << 8);
            uint8_t *dst = (uint8_t *)(SRAM_BASE + got);
            for (uint32_t i = 0; i < len; i++) dst[i] = (uint8_t)getb(0);
            uint32_t c = get_u32();
            if (crc32(dst, len) == c && got + len <= APP_MAX_SIZE) {
                got += len;
                putb('K');
            } else {
                putb('E');                         /* PC retries this chunk */
            }
            break;
        }

        case 'G':                                  /* flash + go            */
            if (got == size && crc32((const uint8_t *)SRAM_BASE, size) == crc
                && flash_write_app(size, entry, crc)) {
                /* verify read-back rewrote SRAM with the same image, so   */
                /* it is already in place — just go                        */
                putb('K');
                run_app(entry);
            }
            putb('E');
            break;

        case 'R':                                  /* volatile run-from-RAM */
            if (got == size && crc32((const uint8_t *)SRAM_BASE, size) == crc) {
                putb('K');
                run_app(entry);
            }
            putb('E');
            break;

        default: break;                            /* ignore line noise     */
        }
    }
}

/* ---------------- main ---------------- */

int main(void)
{
    led(0x1);

    XUartNs550_SetBaud(UART_BASE, XPAR_XUARTNS550_0_CLOCK_FREQ, BAUD);
    XUartNs550_SetLineControlReg(UART_BASE, XUN_LCR_8_DATA_BITS);

    XSpi_Config *cfg = XSpi_LookupConfig(XPAR_XSPI_0_BASEADDR);
    XSpi_CfgInitialize(&Spi, cfg, cfg->BaseAddress);
    XSpi_SetOptions(&Spi, XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION);
    XSpi_Start(&Spi);
    XSpi_IntrGlobalDisable(&Spi);                  /* polled mode           */

    /* stay in the bootloader if the user button is held at power-on       */
    int forced = Xil_In32(BTN_BASE) & 1;

    /* listen for the upload sync byte for a short window                  */
    uint64_t deadline = rdcycle() + (uint64_t)SYNC_WINDOW_MS * 100000ull;
    while (forced || rdcycle() < deadline) {
        if (XUartNs550_IsReceiveData(UART_BASE) &&
            XUartNs550_RecvByte(UART_BASE) == 'U')
            serve_upload();                        /* never returns         */
    }

    try_boot_from_flash();                         /* returns only on fail  */

    /* no valid app: wait for upload forever, blink LEDs                   */
    while (1) {
        if (XUartNs550_IsReceiveData(UART_BASE) &&
            XUartNs550_RecvByte(UART_BASE) == 'U')
            serve_upload();
        led((rdcycle() >> 25) & 3);                /* slow blink            */
    }
}
