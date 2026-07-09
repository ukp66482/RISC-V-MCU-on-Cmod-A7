/* spi0.h - generic driver for the external SPI master (DIP 35-39) */
#ifndef SPI0_H
#define SPI0_H

#include "xil_types.h"

u32  spi0_set_clock(u32 sck_hz);              /* ~1.6-25 MHz, returns actual */
void spi0_xfer(const u8 *tx, u8 *rx, int n);  /* one CS0-framed transfer;
                                                 rx may be NULL (discard)    */
int  spi0_stalled(void);                      /* 1 = unit stopped responding */

#endif
