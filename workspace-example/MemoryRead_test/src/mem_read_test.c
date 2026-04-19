/******************************************************************************
 * Memory Read / Write Test — BRAM · SRAM · QSPI Flash (XIP)
 *
 * Regions under test:
 *   - BRAM  : On-chip Block RAM (128 KB, via LMB)        — static buffer
 *   - SRAM  : axi_emc cellular RAM @ 0x6000_0000 (512 KB, 32 MB range)
 *   - Flash : axi_quad_spi XIP window @ 0x4400_0000 (4 MB NOR, read-only)
 *
 * Notes:
 *   - BRAM/SRAM are tested with write + read-verify.
 *   - The Flash XIP window is read-only — writing Flash requires sending
 *     WREN/ERASE/PROGRAM commands through the AXI_LITE @ 0x44A2_0000 side,
 *     which is out of scope for this simple benchmark. We read-checksum
 *     the first TEST_SIZE bytes instead.
 *
 * Timing: uses axi_timer instance `timer_2` as a free-running 32-bit
 * up-counter. System clock is 100 MHz → one cycle = 10 ns.
 *****************************************************************************/

#include "platform.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_types.h"

/* ---- Memory regions ---- */
#define SRAM_BASE        0x60000000U
#define FLASH_BASE       0x44000000U

/* Window per region. 16 KB leaves plenty of headroom in BRAM for code+stack. */
#define TEST_SIZE_BYTES  (16U * 1024U)
#define TEST_SIZE_WORDS  (TEST_SIZE_BYTES / sizeof(u32))
#define PATTERN_SEED     0xA5A50000U

/* ---- axi_timer (timer_2) direct register access ----
 * Register map (Xilinx PG079):
 *   +0x00 TCSR0, +0x04 TLR0, +0x08 TCR0
 */
#define TIMER_BASE       XPAR_TIMER_2_BASEADDR   /* 0x41C5_0000 */
#define TCSR0_OFF        0x00
#define TLR0_OFF         0x04
#define TCR0_OFF         0x08
#define TCSR_LOAD0       (1U << 5)   /* TCR ← TLR when written */
#define TCSR_ENT0        (1U << 7)   /* enable timer, default UDT=0 → up-count */

#define CYCLES_PER_US    100U        /* 100 MHz system clock */

/* ---- On-board LEDs (progress indicator, in case UART is silent) ----
 *   LED 0b00 : program did not reach main (or LEDs not initialized)
 *   LED 0b01 : reached main, about to run tests
 *   LED 0b11 : all tests finished, hanging in idle loop
 */
#define LED_BASE         XPAR_BOARD_LED_2BITS_BASEADDR
#define GPIO_DATA_OFF    0x00
#define GPIO_TRI_OFF     0x04

static inline void led_init(void)
{
    Xil_Out32(LED_BASE + GPIO_TRI_OFF, 0U);   /* both bits = output */
    Xil_Out32(LED_BASE + GPIO_DATA_OFF, 0U);  /* off */
}

static inline void led_set(u32 mask)
{
    Xil_Out32(LED_BASE + GPIO_DATA_OFF, mask & 0x3U);
}

static inline void timer_start(void)
{
    Xil_Out32(TIMER_BASE + TLR0_OFF,  0U);
    Xil_Out32(TIMER_BASE + TCSR0_OFF, TCSR_LOAD0);   /* load 0 into TCR */
    Xil_Out32(TIMER_BASE + TCSR0_OFF, TCSR_ENT0);    /* start up-counter */
}

static inline u32 timer_stop_cycles(void)
{
    u32 cyc = Xil_In32(TIMER_BASE + TCR0_OFF);
    Xil_Out32(TIMER_BASE + TCSR0_OFF, 0U);           /* stop */
    return cyc;
}

/* ---- BRAM buffer: BSS is placed in Block RAM by the default linker script ---- */
static volatile u32 bram_buf[TEST_SIZE_WORDS] __attribute__((aligned(8)));

/* ---- Core test primitives ---- */

static u32 do_write(volatile u32 *dst, u32 words, u32 seed)
{
    timer_start();
    for (u32 i = 0; i < words; i++) {
        dst[i] = seed ^ i;
    }
    return timer_stop_cycles();
}

static u32 do_read_verify(volatile u32 *src, u32 words, u32 seed, u32 *errors)
{
    u32 err = 0;
    timer_start();
    for (u32 i = 0; i < words; i++) {
        if (src[i] != (seed ^ i)) err++;
    }
    u32 cyc = timer_stop_cycles();
    *errors = err;
    return cyc;
}

static u32 do_read_xor(volatile u32 *src, u32 words, u32 *out_xor)
{
    u32 acc = 0;
    timer_start();
    for (u32 i = 0; i < words; i++) {
        acc ^= src[i];
    }
    u32 cyc = timer_stop_cycles();
    *out_xor = acc;
    return cyc;
}

static void print_rw(u32 wr_cyc, u32 rd_cyc, u32 errs)
{
    xil_printf("  Write  : %10u cyc  (%6u us)\r\n",
               wr_cyc, wr_cyc / CYCLES_PER_US);
    xil_printf("  Read   : %10u cyc  (%6u us)   verify %s  (errors=%u)\r\n",
               rd_cyc, rd_cyc / CYCLES_PER_US,
               (errs == 0) ? "PASS" : "FAIL", errs);
}

int main(void)
{
    init_platform();

    /* Sanity indicator: LED0 on as soon as we reach main */
    led_init();
    led_set(0x1);

    xil_printf("\r\n=== Memory R/W Test  BRAM · SRAM · Flash ===\r\n");
    xil_printf("Test window: %u bytes (%u 32-bit words) per region\r\n",
               TEST_SIZE_BYTES, TEST_SIZE_WORDS);
    xil_printf("Timer      : 0x%08X  (100 MHz, 10 ns/cycle)\r\n\r\n",
               (u32)TIMER_BASE);

    /* ---------------- BRAM ---------------- */
    xil_printf("[BRAM]  On-chip Block RAM (LMB)\r\n");
    xil_printf("  Buffer @ 0x%08X\r\n", (u32)bram_buf);
    u32 bram_wr = do_write(bram_buf, TEST_SIZE_WORDS, PATTERN_SEED);
    u32 bram_err;
    u32 bram_rd = do_read_verify(bram_buf, TEST_SIZE_WORDS, PATTERN_SEED, &bram_err);
    print_rw(bram_wr, bram_rd, bram_err);
    xil_printf("\r\n");

    /* ---------------- SRAM ---------------- */
    xil_printf("[SRAM]  axi_emc cellular RAM @ 0x%08X\r\n", SRAM_BASE);
    volatile u32 *sram = (volatile u32 *)SRAM_BASE;
    u32 sram_wr = do_write(sram, TEST_SIZE_WORDS, PATTERN_SEED);
    u32 sram_err;
    u32 sram_rd = do_read_verify(sram, TEST_SIZE_WORDS, PATTERN_SEED, &sram_err);
    print_rw(sram_wr, sram_rd, sram_err);
    xil_printf("\r\n");

    /* ---------------- Flash (XIP, read-only) ---------------- */
    xil_printf("[Flash] axi_quad_spi XIP @ 0x%08X  (NOR, read-only window)\r\n",
               FLASH_BASE);
    volatile u32 *flash = (volatile u32 *)FLASH_BASE;
    u32 flash_xor;
    u32 flash_rd = do_read_xor(flash, TEST_SIZE_WORDS, &flash_xor);
    xil_printf("  Read   : %10u cyc  (%6u us)   XOR=0x%08X\r\n",
               flash_rd, flash_rd / CYCLES_PER_US, flash_xor);

    volatile u8 *b = (volatile u8 *)flash;
    xil_printf("  Dump16 : ");
    for (u32 i = 0; i < 16; i++) xil_printf("%02X ", b[i]);
    xil_printf("\r\n");
    xil_printf("  Note   : XIP is read-only; writing Flash needs erase/program\r\n");
    xil_printf("           commands via AXI_LITE @ 0x44A2_0000.\r\n\r\n");

    xil_printf("=== Done ===\r\n");

    /* Both LEDs on → all tests completed */
    led_set(0x3);

    /* Hang here so the serial terminal keeps the output visible. */
    while (1) { }

    cleanup_platform();
    return 0;
}
