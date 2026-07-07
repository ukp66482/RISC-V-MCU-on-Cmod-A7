/* servo.c - servo control with one PWM channel, direct register programming.
 *
 * The PWM block is a dual 32-bit timer: timer 0 sets the period, timer 1
 * the high time, both in bus-clock cycles (100 MHz).  Hobby servos want a
 * 50 Hz frame with a 0.5 ms (0 deg) .. 2.5 ms (180 deg) pulse.
 *
 * After init, only the high-time reload register is rewritten; the new
 * width is picked up at the next 20 ms frame, so there are no glitches or
 * dropped pulses while the knob (or the sweep) moves the servo.
 */
#include "servo.h"
#include "showcase.h"

#define PWM0        XPAR_PWM_0_BASEADDR
#define TCSR0       0x00
#define TLR0        0x04
#define TCSR1       0x10
#define TLR1        0x14
#define TCSR_PWM    ((1u << 9) | (1u << 2) | (1u << 4) | (1u << 1) | (1u << 7))
                    /* PWMA0 | GENT | ARHT | UDT | ENT */
#define TCSR_LOAD   (1u << 5)

#define FRAME_CYC   2000000u              /* 20 ms @ 100 MHz = 50 Hz   */
#define MIN_CYC     50000u                /* 0.5 ms =   0 deg          */
#define MAX_CYC     250000u               /* 2.5 ms = 180 deg          */

static int cur_angle = 90;

static u32 angle_to_cyc(int deg)
{
    if (deg < 0)   deg = 0;
    if (deg > 180) deg = 180;
    return MIN_CYC + (u32)((u64)(MAX_CYC - MIN_CYC) * deg / 180);
}

void servo_init(void)
{
    REG32(PWM0 + TLR0) = FRAME_CYC;
    REG32(PWM0 + TCSR0) = TCSR_LOAD;
    REG32(PWM0 + TLR1) = angle_to_cyc(90);
    REG32(PWM0 + TCSR1) = TCSR_LOAD;
    REG32(PWM0 + TCSR0) = TCSR_PWM;
    REG32(PWM0 + TCSR1) = TCSR_PWM;
    cur_angle = 90;
}

void servo_set_angle(int deg)
{
    if (deg < 0)   deg = 0;
    if (deg > 180) deg = 180;
    cur_angle = deg;
    REG32(PWM0 + TLR1) = angle_to_cyc(deg);   /* applied at next frame */
}

int servo_get_angle(void)
{
    return cur_angle;
}
