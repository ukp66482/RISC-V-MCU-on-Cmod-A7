/* ===========================================================================
 * main.c — application template for the RISC-V MCU on Cmod A7
 *
 * Two ways to run the SAME .elf (no rebuild, no board switch):
 *
 *   JTAG (develop):    click Run/Debug in Vitis
 *                      or  python3 tools/jtag_run.py build/<app>.elf
 *                      -> loads into RAM, gone at power-off, debugger works
 *
 *   Standalone (ship): write the app into flash (Standalone Boot Mode guide)
 *                      -> runs by itself at every power-on
 *
 * Memory map (details in ../../mcu/cmod_mcu.ld):
 *
 *   SRAM  0x6000_0000  512 KB  your code / data / heap   (cached)
 *   ITCM  0x0000_8000   32 KB  ITCM_FUNC fast code       (1-cycle BRAM)
 *   DTCM  0x0001_0000   64 KB  DTCM_DATA data + stack    (1-cycle BRAM)
 *
 * cmod_mcu.h (in ../../mcu) names every peripheral register and provides
 * REG32(), ITCM_FUNC, DTCM_DATA and mcu_init(). Call mcu_init() first in
 * main(): it sets the console to 115200 baud.
 * =========================================================================*/
#include "xil_printf.h"
#include "cmod_mcu.h"

/* ==========================================================================
 * Demo: one thing in each memory, then cycle the RGB LED and report on the
 * UART once per step. Replace from here down.
 * ========================================================================*/
DTCM_DATA static u32 blink_count;       /* lives in DTCM, starts at 0      */

static ITCM_FUNC void blink_step(u32 v) /* executes from ITCM              */
{
    REG32(RGB_BASE + GPIO_DATA) = v & 0x7;  /* 3 bits = R/G/B: 8 colors    */
}

static inline u32 sp_now(void)          /* current stack pointer           */
{
    u32 sp;
    __asm__ volatile("mv %0, sp" : "=r"(sp));
    return sp;
}

int main(void)
{
    mcu_init();                         /* console to 115200 baud */

    xil_printf("\r\nRISC-V MCU on Cmod A7 - memory tour\r\n");
    xil_printf("  main()       @ 0x%08x   (SRAM,  cached)\r\n",
               (u32)(UINTPTR)main);
    xil_printf("  blink_step() @ 0x%08x   (ITCM,  1-cycle)\r\n",
               (u32)(UINTPTR)blink_step);
    xil_printf("  blink_count  @ 0x%08x   (DTCM,  1-cycle)\r\n",
               (u32)(UINTPTR)&blink_count);
    xil_printf("  stack        @ 0x%08x   (DTCM,  grows down)\r\n\r\n",
               sp_now());

    u32 v = 1;                          /* (the RGB port is output-only) */
    while (1) {
        blink_step(v);
        xil_printf("  alive: blink_count=%u (DTCM)  rgb=0x%x\r\n",
                   blink_count, v & 0x7);
        v = (v + 1) & 0x7;              /* watch the 3 bits count in binary */
        blink_count++;
        for (volatile int i = 0; i < 20000000; i++)
            ;
    }
    return 0;
}
