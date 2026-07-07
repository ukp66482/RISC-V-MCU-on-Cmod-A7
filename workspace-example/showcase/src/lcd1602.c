/* lcd1602.c - HD44780-class 16x2 LCD behind a PCF8574 I2C backpack.
 *
 * Wiring: backpack VCC -> 3.3 V (Pmod VCC pin), GND -> GND,
 *         SCL -> DIP 13, SDA -> DIP 14.
 * Most backpacks carry their own 4.7k pull-ups, which is exactly what the
 * bus wants.  If the display is too dim at 3.3 V, turn the contrast pot
 * almost all the way; if it is still unreadable, power the module from
 * VU (5 V) *through an I2C level shifter* - never pull SCL/SDA above 3.3 V,
 * the MCU pins are not 5 V tolerant.
 *
 * The backpack drives the LCD in 4-bit mode through the PCF8574's 8 pins:
 *   P0 = RS, P1 = R/W, P2 = EN, P3 = backlight, P4..P7 = D4..D7.
 * One LCD byte = two nibbles; each nibble is latched by an EN high->low
 * pulse.  The PCF8574 applies every byte it receives immediately, so a
 * 4-byte I2C write ({nib|EN, nib, nib|EN, nib}) transfers one full byte.
 */
#include "lcd1602.h"
#include "showcase.h"
#include "xiic_l.h"
#include "sleep.h"

#define I2C_BASE   XPAR_I2C_0_BASEADDR
#define BL         0x08                 /* backlight bit */
#define EN         0x04
#define RS         0x01

static u8  addr;                        /* 7-bit backpack address, 0 = absent */

static int pcf_write(u8 v)
{
    return XIic_Send(I2C_BASE, addr, &v, 1, XIIC_STOP) == 1;
}

/* send one LCD byte (rs=0 command, rs=1 data) as two EN-pulsed nibbles */
static void lcd_byte(u8 val, u8 rs)
{
    u8 hi = (val & 0xF0) | BL | rs;
    u8 lo = (u8)(val << 4) | BL | rs;
    u8 seq[4] = { hi | EN, hi, lo | EN, lo };
    XIic_Send(I2C_BASE, addr, seq, 4, XIIC_STOP);
    usleep(50);                          /* > 37 us instruction time */
}

static void lcd_cmd(u8 c)  { lcd_byte(c, 0); }
static void lcd_data(u8 d) { lcd_byte(d, RS); }

int lcd_present(void) { return addr != 0; }

int lcd_init(void)
{
    static const u8 candidates[] = { 0x27, 0x3F };
    addr = 0;
    for (unsigned i = 0; i < sizeof candidates; i++) {
        u8 dummy;
        if (XIic_Recv(I2C_BASE, candidates[i], &dummy, 1, XIIC_STOP) == 1) {
            addr = candidates[i];
            break;
        }
    }
    if (!addr)
        return 0;

    /* HD44780 4-bit wake-up dance (datasheet fig. 24) */
    usleep(50000);
    u8 wake = 0x30 | BL;                 /* function set, 8-bit, on D4..D7 */
    for (int i = 0; i < 3; i++) {
        u8 seq[2] = { wake | EN, wake };
        XIic_Send(I2C_BASE, addr, seq, 2, XIIC_STOP);
        usleep(5000);
    }
    u8 four = 0x20 | BL;                 /* switch to 4-bit */
    u8 seq[2] = { four | EN, four };
    XIic_Send(I2C_BASE, addr, seq, 2, XIIC_STOP);
    usleep(1000);

    lcd_cmd(0x28);                       /* 4-bit, 2 lines, 5x8 font */
    lcd_cmd(0x08);                       /* display off               */
    lcd_cmd(0x01);                       /* clear                     */
    usleep(2000);
    lcd_cmd(0x06);                       /* entry mode: increment     */
    lcd_cmd(0x0C);                       /* display on, cursor off    */
    return 1;
}

void lcd_line(int row, const char *s)
{
    if (!addr)
        return;
    lcd_cmd(0x80 | (row ? 0x40 : 0x00));
    for (int i = 0; i < 16; i++) {
        if (*s) lcd_data((u8)*s++);
        else    lcd_data(' ');
    }
}
