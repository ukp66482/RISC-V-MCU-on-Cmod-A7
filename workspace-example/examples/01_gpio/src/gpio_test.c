/******************************************************************************
 * GPIO LED Blink Test
 *
 * Toggles all 4 GPIO groups (A, B, C, D) between 0x7F and 0x00.
 * All 28 GPIO pins configured as output. Connect LEDs to verify.
 *
 * GPIO Groups (7-bit each):
 *   Group A: 0x40030000  (DIP pin 1–7)
 *   Group B: 0x40040000  (DIP pin 17–23)
 *   Group C: 0x40050000  (DIP pin 42–48)
 *   Group D: 0x40060000  (DIP pin 26–32)
 *
 * AXI GPIO Registers:
 *   0x00  GPIO_DATA   — Read/Write data
 *   0x04  GPIO_TRI    — Direction: 0 = output, 1 = input
 *****************************************************************************/

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "sleep.h"

/*--- Base addresses (from xparameters.h) ---*/
#define GPIO_A_BASE   XPAR_GPIO_A_0_6_BASEADDR   /* 0x40030000 */
#define GPIO_B_BASE   XPAR_GPIO_B_0_6_BASEADDR   /* 0x40040000 */
#define GPIO_C_BASE   XPAR_GPIO_C_0_6_BASEADDR   /* 0x40050000 */
#define GPIO_D_BASE   XPAR_GPIO_D_0_6_BASEADDR   /* 0x40060000 */

/*--- AXI GPIO register offsets ---*/
#define GPIO_DATA     0x00
#define GPIO_TRI      0x04

/*--- 7-bit all on ---*/
#define ALL_BITS      0x7F

/* Set GPIO group as all output and write value */
static void gpio_init_output(u32 base)
{
    Xil_Out32(base + GPIO_TRI, 0x00);   /* all bits = output */
    Xil_Out32(base + GPIO_DATA, 0x00);  /* start low */
}

static void gpio_write(u32 base, u32 val)
{
    Xil_Out32(base + GPIO_DATA, val);
}

int main()
{
    u32 state = 0;

    init_platform();
    xil_printf("\r\n=== GPIO LED Blink Test ===\r\n");

    /* Set all 4 groups as output */
    gpio_init_output(GPIO_A_BASE);
    gpio_init_output(GPIO_B_BASE);
    gpio_init_output(GPIO_C_BASE);
    gpio_init_output(GPIO_D_BASE);

    xil_printf("All GPIO set to output. Toggling...\r\n");

    while (1) {
        state = state ? 0x00 : ALL_BITS;

        gpio_write(GPIO_A_BASE, state);
        gpio_write(GPIO_B_BASE, state);
        gpio_write(GPIO_C_BASE, state);
        gpio_write(GPIO_D_BASE, state);

        xil_printf("GPIO = 0x%02X\r\n", state);
        sleep(1);   /* 1 second toggle */
    }

    cleanup_platform();
    return 0;
}