/* ili9341.h - 240x320 SPI TFT on the external SPI port (DIP 35-39) */
#ifndef ILI9341_H
#define ILI9341_H

#include "xil_types.h"

/* landscape: 320 wide, 240 high */
#define TFT_W  320
#define TFT_H  240

/* RGB565 colors */
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_RED     0xF800
#define C_GREEN   0x07E0
#define C_BLUE    0x001F
#define C_YELLOW  0xFFE0
#define C_CYAN    0x07FF
#define C_ORANGE  0xFD20
#define C_GRAY    0x8410
#define C_DGRAY   0x2104
#define C_NAVY    0x0010

int  tft_init(void);        /* 1 = display identified, 0 = driving blind */
int  tft_present(void);
int  tft_stalled(void);     /* 1 = SPI stopped responding, driver muted  */
void tft_fill_rect(int x, int y, int w, int h, u16 color);
void tft_text(int x, int y, int scale, u16 fg, u16 bg, const char *s);

/* horizontal bar with remembered fill level - only the delta is redrawn */
typedef struct {
    short x, y, w, h;
    u16   color;
    short cur_px;           /* current fill width, managed by tft_bar_set */
} tft_bar_t;

void tft_bar_init(tft_bar_t *b, int x, int y, int w, int h, u16 color);
void tft_bar_set(tft_bar_t *b, int permille);   /* 0..1000 */

#endif
