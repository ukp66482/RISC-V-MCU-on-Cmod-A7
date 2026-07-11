/* ===========================================================================
 * main.c — application template for the RISC-V MCU on Cmod A7
 *
 * Two ways to run the SAME .elf (no rebuild, no board switch):
 *
 *   JTAG (develop):    click Run/Debug in Vitis
 *                      or  python3 tools/jtag_run.py build/<app>.elf
 *                      -> loads into RAM, gone at power-off, debugger works
 *
 *   Standalone (ship): python3 tools/upload.py build/<app>.elf
 *                      -> written to flash, runs by itself at every power-on
 *
 * Memory map (details in lscript.ld):
 *
 *   SRAM  0x6000_0000  512 KB  your code / data / heap   (cached)
 *   ITCM  0x0000_8000   32 KB  ITCM_FUNC fast code       (1-cycle BRAM)
 *   DTCM  0x0001_0000   64 KB  stack + DTCM_DATA data    (1-cycle BRAM)
 *
 * ONE RULE: keep  mcu_init();  as the first line of main(). It copies
 * ITCM code out of the SRAM image, clears DTCM, and sets the UART to
 * 115200 — nothing placed by ITCM_FUNC/DTCM_DATA works before it runs.
 * =========================================================================*/
#include <string.h>
#include "xil_printf.h"
#include "xil_io.h"
#include "xuartns550_l.h"
#include "xparameters.h"

/* Put a function in ITCM / a variable in DTCM (1-cycle BRAM, never cached —
 * worst case == best case; ideal for interrupt handlers and control loops). */
#define ITCM_FUNC __attribute__((section(".itcm.text"), noinline))
#define DTCM_DATA __attribute__((section(".dtcm")))

/* --------------------------------------------------------------------------
 * mcu_init() — one-time setup, ALWAYS the first line of main():
 *   1. copy ITCM code from its SRAM load address into BRAM (STM32H7 does
 *      exactly this for its ITCM at startup),
 *   2. zero the DTCM data section,
 *   3. fence.i so the CPU refetches the freshly written instructions,
 *   4. set the USB UART to the project-wide 115200 baud.
 * ------------------------------------------------------------------------*/
static void mcu_init(void)
{
    extern char __itcm_lma[], __itcm_start[], __itcm_end[];
    extern char __dtcm_start[], __dtcm_end[];

    memcpy(__itcm_start, __itcm_lma, (size_t)(__itcm_end - __itcm_start));
    memset(__dtcm_start, 0, (size_t)(__dtcm_end - __dtcm_start));
    __asm__ volatile("fence.i");

    XUartNs550_SetBaud(XPAR_UART_USB_BASEADDR, 100000000, 115200);
}

/* ==========================================================================
 * Demo: one thing in each memory, then blink. Replace from here down.
 * ========================================================================*/
#define LED_BASE  0x40000000            /* board LEDs (see the datasheet)  */

DTCM_DATA static u32 blink_count;       /* lives in DTCM (zeroed for you)  */

static ITCM_FUNC void blink_step(u32 v) /* executes from ITCM              */
{
    Xil_Out32(LED_BASE, v & 0x3);
}

static inline u32 sp_now(void)          /* current stack pointer           */
{
    u32 sp;
    __asm__ volatile("mv %0, sp" : "=r"(sp));
    return sp;
}

int main(void)
{
    mcu_init();                         /* <-- the one rule */

    xil_printf("\r\nRISC-V MCU on Cmod A7 - memory tour\r\n");
    xil_printf("  main()       @ 0x%08x   (SRAM,  cached)\r\n",
               (u32)(UINTPTR)main);
    xil_printf("  blink_step() @ 0x%08x   (ITCM,  1-cycle)\r\n",
               (u32)(UINTPTR)blink_step);
    xil_printf("  blink_count  @ 0x%08x   (DTCM,  1-cycle)\r\n",
               (u32)(UINTPTR)&blink_count);
    xil_printf("  stack        @ 0x%08x   (DTCM,  grows down)\r\n\r\n",
               sp_now());

    Xil_Out32(LED_BASE + 0x4, 0x0);     /* both LEDs as outputs */
    u32 v = 1;
    while (1) {
        blink_step(v);
        v ^= 0x3;
        blink_count++;
        for (volatile int i = 0; i < 20000000; i++)
            ;
    }
    return 0;
}
