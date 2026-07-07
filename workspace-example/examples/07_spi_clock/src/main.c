/* ===========================================================================
 * 07_spi_clock — the SPI clock is software-adjustable. Prove it.
 *
 * The external SPI master (spi_0, DIP 35-39) powers up at 6.25 MHz, but its
 * serial clock can be changed at runtime through the SPI clock-control block
 * at 0x4090_0000 (see the IP peripheral reference §7.2):
 *
 *      spi_set_clock(hz);      // ~1.6 MHz … 25 MHz
 *
 * The demo needs no wiring: the SPI unit is put in internal LOOPBACK mode
 * (MOSI fed back to MISO inside the chip), a 256-byte transfer is timed
 * with a hardware timer, and the measured bit-rate is printed — watch it
 * track every clock change.
 *
 * Everything here is direct register programming (no driver): with the SPI
 * unit on its own adjustable clock, its status flags reach the CPU a few
 * cycles late, and the stock XSpi driver trips over that (board-verified:
 * its FIFO-reset/refill and full-speed fill both lose data). The rules that
 * make it reliable are marked NOTE below — they matter for any fast device
 * you talk to on this bus.
 *
 * The chosen clock persists until changed again or power-off; a power-cycle
 * (or reloading the bitstream) restores the 6.25 MHz default.
 * =========================================================================*/
#include <string.h>
#include "xparameters.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xuartns550_l.h"

#define ITCM_FUNC __attribute__((section(".itcm.text"), noinline))
#define DTCM_DATA __attribute__((section(".dtcm")))

static void mcu_init(void)
{
    extern char __itcm_lma[], __itcm_start[], __itcm_end[];
    extern char __dtcm_start[], __dtcm_end[];
    memcpy(__itcm_start, __itcm_lma, (size_t)(__itcm_end - __itcm_start));
    memset(__dtcm_start, 0, (size_t)(__dtcm_end - __dtcm_start));
    __asm__ volatile("fence.i");
    XUartNs550_SetBaud(XPAR_UART_USB_BASEADDR, 100000000, 115200);
}

/* ---- timer 2 as a free-running cycle counter (100 MHz) ---- */
#define TIMER2_BASE  XPAR_TIMER_2_BASEADDR
static void tick_start(void)
{
    Xil_Out32(TIMER2_BASE + 0x04, 0);            /* TLR0 = 0        */
    Xil_Out32(TIMER2_BASE + 0x00, 1u << 5);      /* load            */
    Xil_Out32(TIMER2_BASE + 0x00, 1u << 7);      /* run, count up   */
}
static u32 tick_now(void) { return Xil_In32(TIMER2_BASE + 0x08); }

/* ---- SPI master (spi_0) registers ---- */
#define SPI          XPAR_SPI_0_BASEADDR
#define SPI_SRR      0x40        /* software reset: write 0xA          */
#define SPI_CR       0x60        /* control                            */
#define SPI_SR       0x64        /* status                             */
#define SPI_DTR      0x68        /* transmit data                      */
#define SPI_DRR      0x6C        /* receive data                       */
#define CR_SETUP     0x167u      /* inhibit + FIFO reset + master + enable + loopback */
#define CR_RUN       0x007u      /* master + enable + loopback         */
#define SR_RX_EMPTY  0x01u
#define SR_TX_FULL   0x08u

/* Full-duplex polled transfer.
 * NOTE 1: configure with the transmitter inhibited, then release — the
 *         unit starts cleanly from the released edge.
 * NOTE 2: self-pace to at most 15 bytes in flight. The FIFO is 16 deep
 *         and the status flags arrive a few cycles late, so at full CPU
 *         speed TX_FULL warns you too late — unpaced bursts drop bytes
 *         (board-measured: 15 of 32 lost).                              */
#define REG32(a)     (*(volatile u32 *)(UINTPTR)(a))

/* NOTE 3: resets (SPI_SRR, and the FIFO-reset bits in CR_SETUP) take
 *         effect in the SPI clock domain — a dozen SPI clocks, which at
 *         the slowest setting is ~10 us. Writes issued before that are
 *         swallowed. Wait it out after every reset.                     */
static void spin_wait(void)
{
    for (volatile int i = 0; i < 400; i++)
        ;
}

__attribute__((optimize("O2"), noinline))   /* poll faster than the wire */
static void spi_xfer(const u8 *tx, u8 *rx, int n)
{
    REG32(SPI + SPI_SRR) = 0xA;
    spin_wait();
    REG32(SPI + SPI_CR)  = CR_SETUP;
    spin_wait();
    REG32(SPI + SPI_CR)  = CR_RUN;
    int s = 0, r = 0;
    while (r < n) {
        int room = 15 - (s - r);             /* pacing IS the flow control */
        if (room > n - s) room = n - s;
        while (room-- > 0)
            REG32(SPI + SPI_DTR) = tx[s++];
        /* drain by the empty flag only — occupancy counts cross clock
         * domains with lag and can over-report, causing phantom reads  */
        while (!(REG32(SPI + SPI_SR) & SR_RX_EMPTY))
            rx[r++] = (u8)REG32(SPI + SPI_DRR);
    }
}

/* ---- SPI clock control block (0x4090_0000) --------------------------
 * Three registers are all it takes:
 *   +0x200  reference setting (keep 0x0801: 800 MHz internal reference)
 *   +0x208  divider N         (SPI serial clock = 200 MHz / N)
 *   +0x25C  write 3 = apply
 *   +0x004  bit0 = clock stable — wait for it after applying
 * ------------------------------------------------------------------- */
#define SPICLK  XPAR_SPI_0_CLK_BASEADDR

/* Set the SPI serial clock (~1.6 – 25 MHz). Returns the actual rate. */
static u32 spi_set_clock(u32 sck_hz)
{
    u32 n = (200000000u + sck_hz / 2) / sck_hz;   /* round(200 MHz / f) */
    if (n < 8)   n = 8;                           /* cap:   25 MHz      */
    if (n > 128) n = 128;                         /* floor: ~1.56 MHz   */

    Xil_Out32(SPICLK + 0x200, 0x0801);
    Xil_Out32(SPICLK + 0x208, n);
    Xil_Out32(SPICLK + 0x25C, 0x3);
    while (!(Xil_In32(SPICLK + 0x004) & 1))       /* wait until stable  */
        ;
    return 200000000u / n;
}

/* one timed loopback burst; returns measured kbit/s (0 = data error) */
static u32 measure(void)
{
    static u8 tx[256], rx[256];
    for (int i = 0; i < 256; i++) { tx[i] = (u8)(i * 7 + 3); rx[i] = 0; }

    tick_start();
    u32 t0 = tick_now();
    spi_xfer(tx, rx, sizeof tx);
    u32 cyc = tick_now() - t0;

    if (memcmp(tx, rx, sizeof tx) != 0)
        return 0;
    /* 256 bytes * 8 bits, cycles @ 100 MHz -> kbit/s */
    return (u32)((2048ull * 100000000ull) / cyc / 1000ull);
}

int main(void)
{
    mcu_init();

    xil_printf("\r\n07_spi_clock - software-adjustable SPI clock\r\n");

    /* boot default first, then sweep — watch the rate track the clock */
    xil_printf("  boot default (6.25 MHz) : %u kbit/s\r\n", measure());

    static const u32 sweep[] = {2500000, 12500000, 25000000, 6250000};
    for (unsigned i = 0; i < sizeof sweep / sizeof sweep[0]; i++) {
        u32 got = spi_set_clock(sweep[i]);
        xil_printf("  set %8u Hz (got %8u) : %u kbit/s\r\n",
                   sweep[i], got, measure());
    }

    xil_printf("done - clock returned to 6.25 MHz\r\n");
    while (1)
        ;
    return 0;
}
