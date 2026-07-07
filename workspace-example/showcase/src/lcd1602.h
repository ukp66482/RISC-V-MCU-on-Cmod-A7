/* lcd1602.h - 16x2 character LCD on the I2C bus (PCF8574 backpack) */
#ifndef LCD1602_H
#define LCD1602_H

int  lcd_init(void);                    /* probes 0x27 then 0x3F; 1 = found */
int  lcd_present(void);
void lcd_line(int row, const char *s);  /* row 0/1, text padded to 16 cols */

#endif
