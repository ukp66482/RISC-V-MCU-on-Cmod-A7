/* ===========================================================================
 * showcase - every major MCU feature, one program, one breadboard.
 *
 *   1  SPI  : frames to an ESP32 receiver station           DIP 35-38
 *   2  I2C  : telemetry to the same receiver (slave 0x28)   DIP 13/14
 *   3  PWM  : hobby servo, 50 Hz / 0.5-2.5 ms               DIP 10
 *   4  ADC  : potentiometer, live millivolts                DIP 15
 *   5  IRQ  : timer_0, 100 Hz system tick (ISR in ITCM)     no wiring
 *   6  IRQ  : external button on INTR_0                     DIP 8 (+10k to 3.3V)
 *   7  UART : telemetry + commands on the DIP UART          DIP 11/12
 *
 * The serial buses are demonstrated against an ESP32 "receiver station"
 * (see esp32_bridge/): the MCU streams its telemetry over I2C and SPI once
 * a second, and everything scrolls by on the receiver's USB serial monitor
 * - no display modules needed.  The 'n' command runs a full round-trip
 * PASS/FAIL check on both buses.
 *
 * The pieces are wired together, not just demonstrated side by side:
 * the tick interrupt schedules everything; the knob (or the auto sweep,
 * or a UART command) steers the servo; both serial ports and both buses
 * carry the same live state; the button - through a real GPIO interrupt -
 * switches the control mode.
 *
 * Modes (cycle with the wired button, on-board BTN1, or the 'm' command):
 *   A POT     servo follows the potentiometer
 *   B SWEEP   servo sweeps 0-180 by itself
 *   C MANUAL  servo moved by 'a'/'d' commands (USB console or DIP UART)
 *
 * Every stage degrades gracefully: missing hardware is reported once and
 * the rest keeps running, so the demo can be wired up piece by piece.
 * =========================================================================*/
#include <string.h>
#include <stdio.h>
#include "showcase.h"
#include "adc.h"
#include "servo.h"
#include "uart1.h"
#include "spi0.h"
#include "xil_printf.h"
#include "xuartns550_l.h"
#include "xiic_l.h"
#include "xintc.h"
#include "xil_exception.h"
#include "sleep.h"

#define UART_USB    XPAR_UART_USB_BASEADDR
#define TIMER0      XPAR_TIMER_0_BASEADDR
#define EXTI        XPAR_INT_0_3_BASEADDR
#define RGB         XPAR_BOARD_RGB_BASEADDR
#define LED2        XPAR_BOARD_LED_2BITS_BASEADDR
#define BTN1        XPAR_BOARD_BUTTON_BASEADDR
#define I2C         XPAR_I2C_0_BASEADDR

#define TCSR0       0x00
#define TLR0        0x04
#define TCSR_ENT    (1u << 7)
#define TCSR_ARHT   (1u << 4)
#define TCSR_UDT    (1u << 1)
#define TCSR_ENIT   (1u << 6)
#define TCSR_T0INT  (1u << 8)
#define TCSR_LOAD   (1u << 5)

/* AXI GPIO interrupt registers (EXTI port) */
#define GIER        0x11C
#define IPISR       0x120
#define IPIER       0x128

static void mcu_init(void)
{
    extern char __itcm_lma[], __itcm_start[], __itcm_end[];
    extern char __dtcm_start[], __dtcm_end[];

    /* start from a quiet state even if loaded over a running program
     * (JTAG reload): global interrupts off, interrupt controller muted */
    __asm__ volatile("csrc mstatus, %0" :: "r"(8));
    Xil_Out32(XPAR_XINTC_0_BASEADDR + 0x1C, 0);        /* MER = 0 */
    Xil_Out32(XPAR_XINTC_0_BASEADDR + 0x08, 0);        /* IER = 0 */

    memcpy(__itcm_start, __itcm_lma, (size_t)(__itcm_end - __itcm_start));
    memset(__dtcm_start, 0, (size_t)(__dtcm_end - __dtcm_start));
    __asm__ volatile("fence.i");
    XUartNs550_SetBaud(UART_USB, 100000000, 115200);
    XUartNs550_SetLineControlReg(UART_USB, XUN_LCR_8_DATA_BITS);
}

/* ------------------------------------------------------------------ */
/* interrupt service routines                                         */
/* ------------------------------------------------------------------ */
volatile u32 g_ticks;                     /* 100 Hz system tick        */
static volatile u32 btn_presses;          /* debounced button presses  */
static volatile u32 exti_raw;             /* every EXTI event (guard)  */

/* The tick ISR lives in ITCM: 1-cycle instruction fetches, immune to
 * cache misses - this is where interrupt handlers belong on this MCU. */
ITCM_FUNC static void tick_isr(void *ref)
{
    (void)ref;
    REG32(TIMER0 + TCSR0) |= TCSR_T0INT;               /* clear flag  */
    g_ticks++;
    REG32(RGB) = ((g_ticks % 100) < 4) ? 2 : 0;        /* green blink */
}

static void exti_isr(void *ref)
{
    (void)ref;
    u32 pend = REG32(EXTI + IPISR);
    REG32(EXTI + IPISR) = pend;                        /* toggle-clear */
    exti_raw++;
    if ((REG32(EXTI) & 1) == 0) {                      /* INTR_0 low = pressed */
        static u32 last;
        if (g_ticks - last >= 15) {                    /* 150 ms debounce */
            last = g_ticks;
            btn_presses++;
        }
    }
}

static XIntc intc;

static int irq_init(void)
{
    if (XIntc_Initialize(&intc, XPAR_XINTC_0_BASEADDR) != XST_SUCCESS)
        return -1;
    XIntc_Connect(&intc, 0, (XInterruptHandler)tick_isr, 0);   /* timer_0 */
    XIntc_Connect(&intc, 5, (XInterruptHandler)exti_isr, 0);   /* INTR_0-3 */
    XIntc_Start(&intc, XIN_REAL_MODE);
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XIntc_InterruptHandler,
                                 &intc);

    /* EXTI port: inputs, channel interrupt + global interrupt enable */
    REG32(EXTI + 0x4)   = 0xF;
    REG32(EXTI + IPISR) = REG32(EXTI + IPISR);
    REG32(EXTI + IPIER) = 1;
    REG32(EXTI + GIER)  = 0x80000000u;

    XIntc_Enable(&intc, 0);
    XIntc_Enable(&intc, 5);
    Xil_ExceptionEnable();

    /* 100 Hz tick */
    REG32(TIMER0 + TLR0)  = 1000000;                   /* 10 ms @ 100 MHz */
    REG32(TIMER0 + TCSR0) = TCSR_LOAD;
    REG32(TIMER0 + TCSR0) = TCSR_ENIT | TCSR_ARHT | TCSR_UDT | TCSR_ENT;
    return 0;
}

/* ------------------------------------------------------------------ */
/* application state                                                  */
/* ------------------------------------------------------------------ */
enum { MODE_POT, MODE_SWEEP, MODE_MANUAL, MODE_COUNT };
static const char *const mode_name[] = { "POT", "SWEEP", "MANUAL" };

static int mode = MODE_POT;
static int manual_angle = 90;
static int exti_ok = 1;

/* ------------------------------------------------------------------ */
/* ESP32 receiver station on the serial buses (esp32_bridge/ sketch)  */
/* ------------------------------------------------------------------ */
#define BRIDGE_ADDR  0x28
static int bridge_ok;

/* Never touch the I2C controller while the bus is wedged: a glitched
 * slave can hold SDA low, and the polled XIic calls would block forever.
 * Returns 1 when the bus is idle (after one controller-reset attempt). */
static int i2c_ready(void)
{
    if (!(Xil_In32(I2C + 0x104) & 0x4))        /* SR bit2 = bus busy */
        return 1;
    Xil_Out32(I2C + 0x40, 0xA);                /* soft-reset controller */
    usleep(1000);
    return !(Xil_In32(I2C + 0x104) & 0x4);
}

static int bridge_init(void)
{
    /* reset the I2C controller first: a JTAG reload may have stopped the
     * previous run in the middle of a transfer, wedging the bus */
    Xil_Out32(I2C + 0x40, 0xA);
    usleep(1000);

    spi0_set_clock(2000000);            /* SPI slaves like it gentle */

    u8 d;
    bridge_ok = i2c_ready() &&
                (XIic_Recv(I2C, BRIDGE_ADDR, &d, 1, XIIC_STOP) == 1);
    return bridge_ok;
}

/* push one telemetry line to the receiver: I2C write + one raw SPI frame */
static void bridge_push(const char *line, int len)
{
    if (i2c_ready())
        XIic_Send(I2C, BRIDGE_ADDR, (u8 *)line, len, XIIC_STOP);

    u8 tx[32] = {0};
    memcpy(tx, line, len > 32 ? 32 : len);
    spi0_xfer(tx, 0, 32);
}

/* round-trip PASS/FAIL on both buses (ESP32 receiver) */
static void bridge_test(void)
{
    u8 ping[4] = { 'P', 'I', 'N', 'G' }, sum = 0;
    int sent = 0;
    if (i2c_ready()) {
        sent = XIic_Send(I2C, BRIDGE_ADDR, ping, 4, XIIC_STOP);
        if (i2c_ready())
            XIic_Recv(I2C, BRIDGE_ADDR, &sum, 1, XIIC_STOP);
        xil_printf("bridge I2C: sent %d/4, checksum %02X (want 10) -> %s\r\n",
                   sent, sum, (sent == 4 && sum == 0x10) ? "PASS" : "FAIL");
    } else {
        xil_printf("bridge I2C: bus wedged (SDA held low) -> FAIL\r\n");
    }

    /* SPI: raw 32-byte frames; the receiver's armed reply starts with
     * "ESP32-OK", so frame 2 carries it back */
    static const char hello[] = "HELLO FROM RISC-V MCU";
    u8 tx[32] = {0}, rx[32] = {0};
    const u8 *reply = 0;

    memcpy(tx, hello, sizeof hello);
    spi0_xfer(tx, rx, 32);              /* frame 1 arms the reply       */
    spi0_xfer(tx, rx, 32);              /* frame 2 reads it back        */
    for (int i = 0; i + 8 <= 32 && !reply; i++)
        if (memcmp(rx + i, "ESP32-OK", 8) == 0)
            reply = rx;

    xil_printf("bridge SPI: reply \"");
    const u8 *show = reply ? reply : rx;
    for (int i = 0; i < 32; i++)
        xil_printf("%c", (show[i] >= 32 && show[i] < 127) ? show[i] : '.');
    xil_printf("\" -> %s\r\n", reply ? "PASS" : "FAIL");
}

/* ------------------------------------------------------------------ */
static void console_help(void)
{
    xil_printf("commands: m=next mode  a/d=servo -/+10 (MANUAL)  r=reset  "
               "b=re-probe bridge  n=bridge test  h=help\r\n");
}

static void handle_cmd(int c)
{
    switch (c) {
    case 'm':
        mode = (mode + 1) % MODE_COUNT;
        xil_printf("mode -> %c (%s)\r\n", 'A' + mode, mode_name[mode]);
        break;
    case 'a':
    case 'd':
        if (mode == MODE_MANUAL) {
            manual_angle += (c == 'd') ? 10 : -10;
            if (manual_angle < 0)   manual_angle = 0;
            if (manual_angle > 180) manual_angle = 180;
            xil_printf("manual angle -> %d\r\n", manual_angle);
        } else {
            xil_printf("(a/d only steer in mode C - press 'm')\r\n");
        }
        break;
    case 'r':
        btn_presses = 0;
        xil_printf("button counter reset\r\n");
        break;
    case 'n':
        bridge_test();
        break;
    case 'b':
        /* re-probe the bridge without a reset (e.g. it was plugged late) */
        bridge_ok = 0;
        for (int i = 0; i < 5 && !bridge_ok; i++) {
            usleep(500000);
            u8 d;
            bridge_ok = i2c_ready() &&
                        (XIic_Recv(I2C, BRIDGE_ADDR, &d, 1, XIIC_STOP) == 1);
        }
        if (bridge_ok) {
            xil_printf("bridge detected at 0x28 - running round-trip test\r\n");
            bridge_test();
        } else {
            xil_printf("no bridge at 0x28 - check wiring/power\r\n");
        }
        break;
    case 'h':
    case '?':
        console_help();
        break;
    default:                       /* ignore noise (floating DIP-UART RX) */
        break;
    }
}

/* ------------------------------------------------------------------ */

int main(void)
{
    mcu_init();

    xil_printf("\r\n=====================================================\r\n");
    xil_printf("  RISC-V MCU showcase - 7 features on one breadboard\r\n");
    xil_printf("=====================================================\r\n");
    xil_printf("  ESP32 bridge    : SCL13->G22 SDA14->G21 | SCK35->G18\r\n");
    xil_printf("                    MOSI36->G23 MISO37<-G19 CS38->G5\r\n");
    xil_printf("                    common GND (ESP32 on its own USB)\r\n");
    xil_printf("  Servo (SG90)    : signal DIP10, power VU(5V), GND\r\n");
    xil_printf("  Potentiometer   : wiper DIP15, ends 3.3V / GND\r\n");
    xil_printf("  Button          : DIP8 -> button -> GND, 10k DIP8->3.3V\r\n");
    xil_printf("  DIP UART        : DIP11 TX / DIP12 RX, 115200 8N1\r\n");
    xil_printf("  3.3V comes from the Pmod connector VCC pins.\r\n\r\n");

    REG32(RGB + 0x4)  = 0;                             /* LEDs = outputs */
    REG32(LED2 + 0x4) = 0;

    int adc_ok = (adc_init() == 0);
    servo_init();
    uart1_init();
    bridge_init();
    int irq_ok = (irq_init() == 0);
    REG32(BTN1 + 0x4) = 1;                             /* BTN1 = input */

    xil_printf("  ADC ............ %s\r\n", adc_ok ? "OK" : "FAILED");
    xil_printf("  Servo PWM ...... OK (50 Hz on DIP 10)\r\n");
    xil_printf("  DIP UART ....... OK (telemetry at 1 Hz)\r\n");
    xil_printf("  ESP bridge ..... %s\r\n",
               bridge_ok ? "OK at 0x28 (I2C+SPI telemetry on, try 'n')"
                         : "not detected (wire it + reset, or ignore)");
    xil_printf("  Interrupts ..... %s\r\n",
               irq_ok ? "OK (timer_0 + button EXTI)" : "FAILED");
    xil_printf("  tick ISR runs from ITCM at 0x%08x (see lscript.ld)\r\n",
               (unsigned)(UINTPTR)tick_isr);
    console_help();

    u32 last_fast = 0, last_slow = 0;
    u32 seen_presses = 0, exti_raw_last = 0;
    int btn1_prev = 0;
    int angle = 90;
    u32 mv = 0;

    while (1) {
        /* command sources: USB console + DIP UART, same handler */
        if (XUartNs550_IsReceiveData(UART_USB))
            handle_cmd(XUartNs550_RecvByte(UART_USB));
        int c = uart1_getc();
        if (c > 0)
            handle_cmd(c);

        /* button interrupt events -> mode change */
        if (btn_presses != seen_presses) {
            seen_presses = btn_presses;
            mode = (mode + 1) % MODE_COUNT;
            xil_printf("button IRQ #%u: mode -> %c (%s)\r\n",
                       (unsigned)seen_presses, 'A' + mode, mode_name[mode]);
        }

        u32 t = g_ticks;

        /* every 50 ms: sense + act */
        if (t - last_fast >= 5) {
            last_fast = t;
            mv = adc_pot_mv();
            switch (mode) {
            case MODE_POT:
                angle = (int)(mv * 180 / 3300);
                break;
            case MODE_SWEEP: {
                u32 ph = t % 720;                      /* 7.2 s triangle */
                angle = (ph < 360) ? (int)ph / 2 : (int)(720 - ph) / 2;
                break;
            }
            default:
                angle = manual_angle;
            }
            servo_set_angle(angle);

            /* on-board BTN1 also cycles the mode (works with no wiring) */
            int b = REG32(BTN1) & 1;
            if (b && !btn1_prev) {
                mode = (mode + 1) % MODE_COUNT;
                xil_printf("BTN1: mode -> %c (%s)\r\n",
                           'A' + mode, mode_name[mode]);
            }
            btn1_prev = b;
            REG32(LED2) = (u32)mode & 3;               /* mode on LEDs */
        }

        /* every second: telemetry (UART + both buses), console, guard */
        if (t - last_slow >= 100) {
            last_slow = t;
            u32 up_s = t / 100;
            char line[40];

            int len = sprintf(line, "$MCU,%u,%u,%d,%c,%u\r\n",
                              (unsigned)up_s, (unsigned)mv, angle,
                              'A' + mode, (unsigned)btn_presses);
            uart1_puts(line);
            if (bridge_ok)
                bridge_push(line, len - 2);            /* no CRLF */

            xil_printf("[%5us] POT %4u mV | servo %3d deg | mode %c | btn %u\r\n",
                       (unsigned)up_s, (unsigned)mv, angle, 'A' + mode,
                       (unsigned)btn_presses);

            /* an unwired INTR_0 floats (no pull-up on the pin) and can
             * fire continuously - detect the storm and mute it */
            u32 raw = exti_raw;
            if (exti_ok && raw - exti_raw_last > 500) {
                XIntc_Disable(&intc, 5);
                exti_ok = 0;
                xil_printf("button IRQ muted: INTR_0 is floating "
                           "(wire button + 10k pull-up, then reset)\r\n");
            }
            exti_raw_last = raw;
        }
    }
    return 0;
}
