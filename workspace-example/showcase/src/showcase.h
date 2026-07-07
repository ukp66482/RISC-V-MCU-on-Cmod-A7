/* showcase.h - shared definitions for the full-feature showcase app */
#ifndef SHOWCASE_H
#define SHOWCASE_H

#include "xparameters.h"
#include "xil_io.h"
#include "xil_types.h"

#define REG32(a)   (*(volatile u32 *)(UINTPTR)(a))
#define ITCM_FUNC  __attribute__((section(".itcm.text"), noinline))
#define DTCM_DATA  __attribute__((section(".dtcm")))

/* 100 Hz system tick, maintained by the timer_0 ISR (main.c) */
extern volatile u32 g_ticks;
static inline u32 now_ms(void) { return g_ticks * 10; }

#endif
