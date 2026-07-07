/* uart1.c - the second 16550 UART, exposed on the DIP header.
 *
 * Wire a 3.3 V USB-TTL adapter: DIP 11 (TX) -> adapter RX, DIP 12 (RX) ->
 * adapter TX, GND common.  The showcase streams a 1 Hz telemetry line out
 * of it and accepts the same single-key commands as the USB console.
 *
 * Note: DIP 12 has no pull-up, so an unwired RX floats and may deliver
 * garbage bytes - the command parser only reacts to whitelisted letters.
 */
#include "uart1.h"
#include "xparameters.h"
#include "xuartns550_l.h"

#define U1 XPAR_UART_1_BASEADDR

void uart1_init(void)
{
    XUartNs550_SetBaud(U1, XPAR_UART_1_CLOCK_FREQ, 115200);
    XUartNs550_SetLineControlReg(U1, XUN_LCR_8_DATA_BITS);
}

void uart1_puts(const char *s)
{
    while (*s)
        XUartNs550_SendByte(U1, *s++);
}

int uart1_getc(void)
{
    if (!XUartNs550_IsReceiveData(U1))
        return -1;
    return XUartNs550_RecvByte(U1);
}
