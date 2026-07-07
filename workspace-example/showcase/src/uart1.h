/* uart1.h - external UART on DIP 11 (TX) / DIP 12 (RX), 115200 8N1 */
#ifndef UART1_H
#define UART1_H

void uart1_init(void);
void uart1_puts(const char *s);
int  uart1_getc(void);          /* -1 when no byte is waiting */

#endif
