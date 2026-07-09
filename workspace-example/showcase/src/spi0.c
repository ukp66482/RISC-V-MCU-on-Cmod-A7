/* spi0.c - external SPI master (spi_0, DIP 35-39), direct register level.
 *
 * The SPI unit runs on its own software-settable clock: the clock-control
 * block at 0x4090_0000 sets the serial clock anywhere in ~1.6-25 MHz
 * (see the IP peripheral reference §7.2).
 *
 * Transfer rules (board-derived; the stock XSpi driver violates them and
 * loses data on this async-clocked unit):
 *   - after a reset or the FIFO-reset bits, wait ~10 us before the next
 *     register write - writes inside the settling window are swallowed
 *   - the run-mode control value must NOT keep the FIFO-reset bits set
 *   - keep at most 15 bytes in flight (the status flags lag the SPI domain)
 *   - drain RX by the RX-empty *flag*; RX count == TX count also proves the
 *     last byte fully left the shifter, so CS may be released safely
 * A watchdog aborts (and mutes the driver) if the unit stops responding,
 * so a wiring or clocking mistake can't freeze the application.
 */
#include "spi0.h"
#include "showcase.h"

#define SPI          XPAR_SPI_0_BASEADDR
#define SPI_SRR      0x40
#define SPI_CR       0x60
#define SPI_SR       0x64
#define SPI_DTR      0x68
#define SPI_DRR      0x6C
#define SPI_SSR      0x70
#define CR_IDLE      0x1E6u     /* inhibit + manual SS + FIFO reset + master + SPE */
#define CR_RUN       0x086u     /* manual SS + master + SPE - no reset bits */
#define SR_RX_EMPTY  0x01u

#define SPICLK       XPAR_SPI_0_CLK_BASEADDR

static int stalled;
static int initialized;

int spi0_stalled(void) { return stalled; }

static void spin_wait(void)                     /* ~10 us settle */
{
    for (volatile int i = 0; i < 400; i++) ;
}

u32 spi0_set_clock(u32 sck_hz)
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

/* full-duplex paced transfer while CS is held */
__attribute__((optimize("O2"), noinline))
static void xfer(const u8 *tx, u8 *rx, int n)
{
    int s = 0, r = 0, last_r = -1;
    u32 idle = 0;
    while (r < n) {
        int room = 15 - (s - r);
        if (room > n - s) room = n - s;
        while (room-- > 0)
            REG32(SPI + SPI_DTR) = tx[s++];
        while (!(REG32(SPI + SPI_SR) & SR_RX_EMPTY)) {
            u8 b = (u8)REG32(SPI + SPI_DRR);
            if (rx) rx[r] = b;
            r++;
        }
        if (r == last_r) {
            if (++idle > 3000000u) { stalled = 1; return; }
        } else {
            idle = 0;
            last_r = r;
        }
    }
}

void spi0_xfer(const u8 *tx, u8 *rx, int n)
{
    if (stalled)
        return;
    if (!initialized) {
        REG32(SPI + SPI_SRR) = 0xA;             /* one reset at first use */
        spin_wait();
        initialized = 1;
    }
    REG32(SPI + SPI_CR) = CR_IDLE;              /* FIFO reset while inhibited */
    spin_wait();                                /* let the reset land         */
    REG32(SPI + SPI_SSR) = ~1u;                 /* assert SS0                 */
    REG32(SPI + SPI_CR) = CR_RUN;               /* release, no reset bits     */
    xfer(tx, rx, n);
    REG32(SPI + SPI_SSR) = ~0u;
    REG32(SPI + SPI_CR) = CR_IDLE;
}
