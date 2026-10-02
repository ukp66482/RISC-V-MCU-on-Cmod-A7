/* ===========================================================================
 * main.c — DIP pin test for the 8-bit GPIO A/B/C layout
 *
 * Walks a single HIGH across all 24 GPIO pins, one pin per second, and
 * prints which DIP pin is driven. Probe each pin with an LED + 330R to GND
 * (or a multimeter): exactly the printed pin should read 3.3 V.
 *
 * Every step also prints the 4 INTR inputs (DIP 9, 10, 17, 18). Tie one to
 * 3.3 V or GND with a jumper and watch its bit change.
 *
 * Use app_template/src/lscript.ld as the linker script.
 * =========================================================================*/
#include <string.h>
#include "xil_printf.h"
#include "xil_io.h"
#include "xuartns550_l.h"
#include "xparameters.h"

#define GPIO_A_BASE  0x40030000
#define GPIO_B_BASE  0x40040000
#define GPIO_C_BASE  0x40050000
#define EXTI_BASE    0x40070000
#define GPIO_DATA    0x000
#define GPIO_TRI     0x004

static void mcu_init(void)
{
    extern char __itcm_lma[], __itcm_start[], __itcm_end[];
    extern char __dtcm_start[], __dtcm_end[];

    memcpy(__itcm_start, __itcm_lma, (size_t)(__itcm_end - __itcm_start));
    memset(__dtcm_start, 0, (size_t)(__dtcm_end - __dtcm_start));
    __asm__ volatile("fence.i");

    XUartNs550_SetBaud(XPAR_UART_USB_BASEADDR, 100000000, 115200);
}

/* one entry per GPIO pin, in walk order */
static const struct {
    u32  base;
    u8   bit;
    u8   dip;
    char name[8];
} pins[] = {
    { GPIO_A_BASE, 0,  1, "GPIO_A0" }, { GPIO_A_BASE, 1,  2, "GPIO_A1" },
    { GPIO_A_BASE, 2,  3, "GPIO_A2" }, { GPIO_A_BASE, 3,  4, "GPIO_A3" },
    { GPIO_A_BASE, 4,  5, "GPIO_A4" }, { GPIO_A_BASE, 5,  6, "GPIO_A5" },
    { GPIO_A_BASE, 6,  7, "GPIO_A6" }, { GPIO_A_BASE, 7,  8, "GPIO_A7" },
    { GPIO_B_BASE, 0, 33, "GPIO_B0" }, { GPIO_B_BASE, 1, 32, "GPIO_B1" },
    { GPIO_B_BASE, 2, 31, "GPIO_B2" }, { GPIO_B_BASE, 3, 30, "GPIO_B3" },
    { GPIO_B_BASE, 4, 29, "GPIO_B4" }, { GPIO_B_BASE, 5, 28, "GPIO_B5" },
    { GPIO_B_BASE, 6, 27, "GPIO_B6" }, { GPIO_B_BASE, 7, 26, "GPIO_B7" },
    { GPIO_C_BASE, 0, 48, "GPIO_C0" }, { GPIO_C_BASE, 1, 47, "GPIO_C1" },
    { GPIO_C_BASE, 2, 46, "GPIO_C2" }, { GPIO_C_BASE, 3, 45, "GPIO_C3" },
    { GPIO_C_BASE, 4, 44, "GPIO_C4" }, { GPIO_C_BASE, 5, 43, "GPIO_C5" },
    { GPIO_C_BASE, 6, 42, "GPIO_C6" }, { GPIO_C_BASE, 7, 41, "GPIO_C7" },
};
#define NPINS (sizeof pins / sizeof pins[0])

int main(void)
{
    mcu_init();

    xil_printf("\r\nDIP pin test - GPIO A/B/C 8-bit layout\r\n");

    /* all three ports: 8 outputs, driven low */
    Xil_Out32(GPIO_A_BASE + GPIO_DATA, 0);
    Xil_Out32(GPIO_B_BASE + GPIO_DATA, 0);
    Xil_Out32(GPIO_C_BASE + GPIO_DATA, 0);
    Xil_Out32(GPIO_A_BASE + GPIO_TRI, 0);
    Xil_Out32(GPIO_B_BASE + GPIO_TRI, 0);
    Xil_Out32(GPIO_C_BASE + GPIO_TRI, 0);

    /* readback of the width: writing 0xFF to an 8-bit port reads back 0xFF,
     * a leftover 7-bit port would read back 0x7F */
    Xil_Out32(GPIO_A_BASE + GPIO_DATA, 0xFF);
    Xil_Out32(GPIO_B_BASE + GPIO_DATA, 0xFF);
    Xil_Out32(GPIO_C_BASE + GPIO_DATA, 0xFF);
    xil_printf("  width check (expect 0xff): A=0x%02x B=0x%02x C=0x%02x\r\n\r\n",
               Xil_In32(GPIO_A_BASE + GPIO_DATA) & 0xFF,
               Xil_In32(GPIO_B_BASE + GPIO_DATA) & 0xFF,
               Xil_In32(GPIO_C_BASE + GPIO_DATA) & 0xFF);

    u32 i = 0;
    while (1) {
        Xil_Out32(GPIO_A_BASE + GPIO_DATA, 0);
        Xil_Out32(GPIO_B_BASE + GPIO_DATA, 0);
        Xil_Out32(GPIO_C_BASE + GPIO_DATA, 0);
        Xil_Out32(pins[i].base + GPIO_DATA, 1u << pins[i].bit);

        u32 intr = Xil_In32(EXTI_BASE + GPIO_DATA) & 0xF;
        xil_printf("  DIP %2d  %s  HIGH    INTR[3:0]=%d%d%d%d\r\n",
                   pins[i].dip, pins[i].name,
                   (intr >> 3) & 1, (intr >> 2) & 1, (intr >> 1) & 1, intr & 1);

        i = (i + 1) % NPINS;
        for (volatile int d = 0; d < 20000000; d++)
            ;
    }
    return 0;
}
