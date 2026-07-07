/* ili9341.c - ILI9341 TFT driver, direct register programming throughout.
 *
 * Wiring (module pin -> board pin):
 *   VCC  -> VU (DIP 24, 5 V; module has its own regulator)  GND -> DIP 25
 *   CS   -> DIP 38 (SPI_SS0)      SCK -> DIP 35     SDI/MOSI -> DIP 36
 *   SDO/MISO -> DIP 37 (used once, to identify the chip)
 *   DC   -> DIP 48 (GPIO_C0)      RESET -> DIP 47 (GPIO_C1)
 *   LED  -> 3.3 V (Pmod VCC pin; ~50 mA backlight, do NOT feed from a GPIO)
 *
 * The SPI unit runs on its own software-settable clock (control block at
 * 0x4090_0000).  This driver identifies the panel at 2 MHz, then raises the
 * clock to TFT_SCK_HZ for drawing.  12.5 MHz is reliable on a breadboard;
 * change to 25000000 for short, soldered wiring.
 *
 * Transfer rules (board-derived, see examples/07_spi_clock):
 *   - after a reset or reconfigure, wait ~10 us before the next register write
 *   - keep at most 15 bytes in flight (the status flags lag the SPI domain)
 *   - drain RX by the RX-empty *flag*; when RX count == TX count the last
 *     byte has fully left the shifter, so DC/CS may change safely
 */
#include "ili9341.h"
#include "showcase.h"
#include "font5x7.h"
#include "sleep.h"

#define TFT_SCK_HZ   12500000u

/* ---- SPI master (spi_0) ---- */
#define SPI          XPAR_SPI_0_BASEADDR
#define SPI_SRR      0x40
#define SPI_CR       0x60
#define SPI_SR       0x64
#define SPI_DTR      0x68
#define SPI_DRR      0x6C
#define SPI_SSR      0x70
#define CR_IDLE      0x1E6u     /* inhibit + manual SS + FIFO reset + master + SPE */
#define CR_RUN       0x086u     /* manual SS + master + SPE - NO reset bits: the
                                 * FIFO-reset pulse crosses into the (slow) SPI
                                 * clock domain and eats the writes that follow */
#define SR_RX_EMPTY  0x01u

/* ---- SPI clock control block ---- */
#define SPICLK       XPAR_SPI_0_CLK_BASEADDR

/* ---- DC / RESET on GPIO group C (DIP 48 / DIP 47) ---- */
#define GPIOC        XPAR_GPIO_C_0_6_BASEADDR
#define DC_BIT       0x01u
#define RST_BIT      0x02u

static u32 gpioc_shadow;
static int present;
static int spi_stalled;         /* set if a transfer times out: all tft_*
                                 * calls become no-ops so the rest of the
                                 * showcase keeps running */

static void gpioc_set(u32 bit, int level)
{
    if (level) gpioc_shadow |= bit;
    else       gpioc_shadow &= ~bit;
    REG32(GPIOC + 0x0) = gpioc_shadow;
}

static void spin_wait(void)                     /* ~10 us settle */
{
    for (volatile int i = 0; i < 400; i++) ;
}

/* set the SPI serial clock; returns the achieved rate (from 07_spi_clock) */
static u32 spi_clk_set(u32 sck_hz)
{
    u32 n = (200000000u + sck_hz / 2) / sck_hz;
    if (n < 8)   n = 8;
    if (n > 128) n = 128;
    Xil_Out32(SPICLK + 0x200, 0x0801);
    Xil_Out32(SPICLK + 0x208, n);
    Xil_Out32(SPICLK + 0x25C, 0x3);
    while (!(Xil_In32(SPICLK + 0x004) & 1)) ;
    return 200000000u / n;
}

/* full-duplex paced transfer while CS is held; rx may be NULL (discard).
 * A watchdog aborts (and mutes the whole driver) if the unit ever stops
 * responding, so a wiring or clock mistake can't freeze the application. */
__attribute__((optimize("O2"), noinline))
static void spi_xfer(const u8 *tx, u8 *rx, int n)
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
            if (++idle > 3000000u) { spi_stalled = 1; return; }
        } else {
            idle = 0;
            last_r = r;
        }
    }
}

/* like spi_xfer but transmits one constant byte pattern pair (hi,lo) n times;
 * used for solid fills so no pixel buffer is needed */
__attribute__((optimize("O2"), noinline))
static void spi_fill(u8 hi, u8 lo, int npx)
{
    int n = npx * 2, s = 0, r = 0, last_r = -1;
    u32 idle = 0;
    while (r < n) {
        int room = 15 - (s - r);
        if (room > n - s) room = n - s;
        while (room-- > 0) {
            REG32(SPI + SPI_DTR) = (s & 1) ? lo : hi;
            s++;
        }
        while (!(REG32(SPI + SPI_SR) & SR_RX_EMPTY)) {
            (void)REG32(SPI + SPI_DRR);
            r++;
        }
        if (r == last_r) {
            if (++idle > 3000000u) { spi_stalled = 1; return; }
        } else {
            idle = 0;
            last_r = r;
        }
    }
}

static void spi_begin(void)
{
    REG32(SPI + SPI_CR) = CR_IDLE;              /* FIFO reset while inhibited */
    spin_wait();                                /* let the reset land         */
    REG32(SPI + SPI_SSR) = ~1u;                 /* assert SS0                 */
    REG32(SPI + SPI_CR) = CR_RUN;               /* release, no reset bits     */
}

static void spi_end(void)
{
    REG32(SPI + SPI_SSR) = ~0u;
    REG32(SPI + SPI_CR) = CR_IDLE;
}

/* command + optional data, one CS frame */
static void tft_cmd(u8 cmd, const u8 *data, int n)
{
    spi_begin();
    gpioc_set(DC_BIT, 0);
    spi_xfer(&cmd, 0, 1);                       /* rx drained = fully shifted */
    gpioc_set(DC_BIT, 1);
    if (n > 0)
        spi_xfer(data, 0, n);
    spi_end();
}

/* open an address window and leave CS asserted, DC = data */
static void window(int x0, int y0, int x1, int y1)
{
    u8 ca[4] = { (u8)(x0 >> 8), (u8)x0, (u8)(x1 >> 8), (u8)x1 };
    u8 pa[4] = { (u8)(y0 >> 8), (u8)y0, (u8)(y1 >> 8), (u8)y1 };
    tft_cmd(0x2A, ca, 4);
    tft_cmd(0x2B, pa, 4);
    spi_begin();
    gpioc_set(DC_BIT, 0);
    u8 ramwr = 0x2C;
    spi_xfer(&ramwr, 0, 1);
    gpioc_set(DC_BIT, 1);
}

int tft_present(void) { return present; }
int tft_stalled(void) { return spi_stalled; }

int tft_init(void)
{
    /* DC/RST as outputs, RST high */
    REG32(GPIOC + 0x4) = ~(DC_BIT | RST_BIT) & 0x7F;
    gpioc_shadow = DC_BIT | RST_BIT;
    REG32(GPIOC + 0x0) = gpioc_shadow;

    /* SPI unit: one reset at start-up, then leave it configured */
    REG32(SPI + SPI_SRR) = 0xA;
    spin_wait();
    REG32(SPI + SPI_CR) = CR_IDLE;
    spin_wait();

    /* hardware reset */
    gpioc_set(RST_BIT, 0);
    usleep(20000);
    gpioc_set(RST_BIT, 1);
    usleep(150000);

    /* identify at a gentle 2 MHz: 0xD3 returns .., 0x93, 0x41 */
    spi_clk_set(2000000);
    u8 id_cmd = 0xD3, zeros[4] = {0, 0, 0, 0}, id[4] = {0, 0, 0, 0};
    spi_begin();
    gpioc_set(DC_BIT, 0);
    spi_xfer(&id_cmd, 0, 1);
    gpioc_set(DC_BIT, 1);
    spi_xfer(zeros, id, 4);
    spi_end();
    present = 0;
    for (int i = 0; i < 3; i++)
        if (id[i] == 0x93 && id[i + 1] == 0x41)
            present = 1;

    /* init sequence (works on the common 2.4"/2.8" modules) */
    static const u8 pwr1[]  = {0x23};
    static const u8 pwr2[]  = {0x10};
    static const u8 vcom1[] = {0x3E, 0x28};
    static const u8 vcom2[] = {0x86};
    static const u8 mad[]   = {0x28};           /* landscape, BGR */
    static const u8 pix[]   = {0x55};           /* RGB565 */
    static const u8 frc[]   = {0x00, 0x18};
    static const u8 dfc[]   = {0x08, 0x82, 0x27};
    tft_cmd(0xC0, pwr1, 1);
    tft_cmd(0xC1, pwr2, 1);
    tft_cmd(0xC5, vcom1, 2);
    tft_cmd(0xC7, vcom2, 1);
    tft_cmd(0x36, mad, 1);
    tft_cmd(0x3A, pix, 1);
    tft_cmd(0xB1, frc, 2);
    tft_cmd(0xB6, dfc, 3);
    tft_cmd(0x11, 0, 0);                        /* sleep out */
    usleep(120000);
    tft_cmd(0x29, 0, 0);                        /* display on */

    spi_clk_set(TFT_SCK_HZ);                    /* full speed for drawing */
    tft_fill_rect(0, 0, TFT_W, TFT_H, C_BLACK);
    return present;
}

void tft_fill_rect(int x, int y, int w, int h, u16 color)
{
    if (w <= 0 || h <= 0 || spi_stalled)
        return;
    window(x, y, x + w - 1, y + h - 1);
    spi_fill((u8)(color >> 8), (u8)color, w * h);
    spi_end();
}

void tft_text(int x, int y, int scale, u16 fg, u16 bg, const char *s)
{
    static u8 cell[6 * 3 * 8 * 3 * 2];          /* one glyph at scale <= 3 */
    if (spi_stalled)
        return;
    if (scale < 1) scale = 1;
    if (scale > 3) scale = 3;
    int cw = 6 * scale, chh = 8 * scale;

    for (; *s; s++, x += cw) {
        u8 code = (u8)*s;
        const unsigned char *col =
            (code >= FONT_FIRST && code <= FONT_LAST) ? font5x7[code - FONT_FIRST]
                                                      : font5x7[0];
        int i = 0;
        for (int py = 0; py < chh; py++) {
            int row = py / scale;               /* 0..7, row 7 = spacing */
            for (int px = 0; px < cw; px++) {
                int c = px / scale;             /* 0..5, col 5 = spacing */
                u16 v = (c < 5 && row < 7 && ((col[c] >> row) & 1)) ? fg : bg;
                cell[i++] = (u8)(v >> 8);
                cell[i++] = (u8)v;
            }
        }
        window(x, y, x + cw - 1, y + chh - 1);
        spi_xfer(cell, 0, i);
        spi_end();
    }
}

void tft_bar_init(tft_bar_t *b, int x, int y, int w, int h, u16 color)
{
    b->x = x; b->y = y; b->w = w; b->h = h;
    b->color = color;
    b->cur_px = 0;
    tft_fill_rect(x - 2, y - 2, w + 4, h + 4, C_GRAY);    /* frame  */
    tft_fill_rect(x, y, w, h, C_DGRAY);                   /* trough */
}

void tft_bar_set(tft_bar_t *b, int permille)
{
    if (permille < 0)    permille = 0;
    if (permille > 1000) permille = 1000;
    int px = b->w * permille / 1000;
    if (px > b->cur_px)
        tft_fill_rect(b->x + b->cur_px, b->y, px - b->cur_px, b->h, b->color);
    else if (px < b->cur_px)
        tft_fill_rect(b->x + px, b->y, b->cur_px - px, b->h, C_DGRAY);
    b->cur_px = px;
}
