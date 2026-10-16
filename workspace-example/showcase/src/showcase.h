/* showcase.h - shared definitions for the full-feature showcase app */
#ifndef SHOWCASE_H
#define SHOWCASE_H

#include "xparameters.h"
#include "xil_io.h"
#include "xil_types.h"

#include "cmod_mcu.h"        /* REG32, ITCM_FUNC, DTCM_DATA, mcu_init */

/* 100 Hz system tick, maintained by the timer_0 ISR (main.c) */
extern volatile u32 g_ticks;
static inline u32 now_ms(void) { return g_ticks * 10; }

#endif
