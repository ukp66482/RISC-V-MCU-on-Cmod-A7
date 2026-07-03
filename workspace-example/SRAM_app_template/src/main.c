/*
 * main.c — student application template (runs from SRAM)
 *
 * Build in Vitis, then either:
 *   - click Run/Debug (JTAG), or
 *   - python3 upload.py build/<app>.elf     (persist to flash, runs at power-on)
 *
 * Layout (see lscript.ld): code/data/heap in 512 KB SRAM (cached),
 * stack in on-chip BRAM (1-cycle, deterministic).
 */
#include "xil_printf.h"
#include "xil_io.h"
#include "xuartns550_l.h"
#include "xparameters.h"

#define LED_BASE 0x40000000  /* board_led_2bits */

int main(void)
{
    /* match the bootloader/upload.py baud so one terminal session fits all */
    XUartNs550_SetBaud(XPAR_UART_USB_BASEADDR, 100000000, 115200);

    xil_printf("\r\nHello from SRAM! (RISC-V MCU on Cmod A7)\r\n");

    Xil_Out32(LED_BASE + 0x4, 0x0);       /* both LEDs as outputs */
    u32 v = 1;
    while (1) {
        Xil_Out32(LED_BASE, v & 0x3);
        v ^= 0x3;
        for (volatile int i = 0; i < 20000000; i++)
            ;
    }
    return 0;
}
