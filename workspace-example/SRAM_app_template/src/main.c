/*
 * main.c — student application template (runs from SRAM)
 *
 * Build in Vitis, then either:
 *   - click Run/Debug (JTAG), or
 *   - python3 upload.py build/<app>.elf     (persist to flash, runs at power-on)
 *
 * Memory layout (see lscript.ld):
 *   SRAM  0x6000_0000  512 KB  code / data / heap  (I/D-cached)
 *   ITCM  0x0000_8000   32 KB  fast code  — 1-cycle BRAM, never cached
 *   DTCM  0x0001_0000   64 KB  stack (top-down) + fast data — 1-cycle BRAM
 *
 * ITCM_FUNC / DTCM_DATA place code or data into the tightly-coupled RAMs,
 * where the worst case equals the best case (no cache misses) — put
 * interrupt handlers and timing-critical loops there. tcm_init() must run
 * before anything in ITCM/DTCM is used: as on an STM32H7, ITCM code ships
 * inside the main image and is copied out at startup.
 */
#include <string.h>
#include "xil_printf.h"
#include "xil_io.h"
#include "xuartns550_l.h"
#include "xparameters.h"

#define ITCM_FUNC __attribute__((section(".itcm.text"), noinline))
#define DTCM_DATA __attribute__((section(".dtcm")))

/* Copy ITCM code out of the SRAM image and zero DTCM data — call FIRST. */
static void tcm_init(void)
{
    extern char __itcm_lma[], __itcm_start[], __itcm_end[];
    extern char __dtcm_start[], __dtcm_end[];
    memcpy(__itcm_start, __itcm_lma, (size_t)(__itcm_end - __itcm_start));
    memset(__dtcm_start, 0, (size_t)(__dtcm_end - __dtcm_start));
    __asm__ volatile("fence.i");
}

#define LED_BASE 0x40000000  /* board_led_2bits */

/* Example: this function executes from ITCM — print its address to check. */
static ITCM_FUNC void blink_step(u32 v)
{
    Xil_Out32(LED_BASE, v & 0x3);
}

int main(void)
{
    tcm_init();

    /* match the bootloader/upload.py baud so one terminal session fits all */
    XUartNs550_SetBaud(XPAR_UART_USB_BASEADDR, 100000000, 115200);

    xil_printf("\r\nHello from SRAM! (RISC-V MCU on Cmod A7)\r\n");
    xil_printf("blink_step() runs from ITCM @ 0x%08x\r\n", (u32)(UINTPTR)blink_step);

    Xil_Out32(LED_BASE + 0x4, 0x0);       /* both LEDs as outputs */
    u32 v = 1;
    while (1) {
        blink_step(v);
        v ^= 0x3;
        for (volatile int i = 0; i < 20000000; i++)
            ;
    }
    return 0;
}
