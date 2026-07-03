/******************************************************************************
 * SG90 Servo Control via AXI Timer PWM
 *
 * Hardware:
 *   - 3x AXI Timer IP with PWM output
 *   - System clock: 100 MHz
 *   - PWM_0 base: 0x40200000  (DIP pin 10)
 *   - PWM1  base: 0x40210000  (DIP pin 34)
 *   - PWM_2 base: 0x40220000  (DIP pin 40)
 *
 * SG90 Servo Spec:
 *   - PWM frequency: 50 Hz → period = 20,000,000 ns
 *   - Pulse width: 0.5 ms (0°) to 2.5 ms (180°)
 *     -   0° →   500,000 ns high time
 *     -  90° → 1,500,000 ns high time
 *     - 180° → 2,500,000 ns high time
 *
 * Note: XTmrCtr_PwmConfigure() takes period and high time in nanoseconds.
 *****************************************************************************/

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xtmrctr.h"
#include "sleep.h"

/*--- Base addresses (from xparameters.h) ---*/
#define PWM0_BASEADDR   XPAR_PWM_0_BASEADDR    /* 0x40200000, DIP pin 10 */
#define PWM1_BASEADDR   XPAR_PWM_1_BASEADDR     /* 0x40210000, DIP pin 34 */
#define PWM2_BASEADDR   XPAR_PWM_2_BASEADDR    /* 0x40220000, DIP pin 40 */

/*--- SG90 timing in nanoseconds ---*/
#define PWM_PERIOD_NS   20000000U   /* 20 ms = 50 Hz */
#define SERVO_MIN_NS    500000U     /* 0.5 ms =   0° */
#define SERVO_MAX_NS    2500000U    /* 2.5 ms = 180° */

/* Timer instances */
static XTmrCtr pwm0, pwm1, pwm2;

/* Convert angle (0–180) to high time in nanoseconds */
static u32 angle_to_ns(int angle)
{
    if (angle < 0)   angle = 0;
    if (angle > 180) angle = 180;
    return SERVO_MIN_NS +
           (u32)((u64)(SERVO_MAX_NS - SERVO_MIN_NS) * angle / 180);
}

/* Initialize one AXI Timer for PWM, start at given angle */
static int pwm_init(XTmrCtr *inst, UINTPTR base_addr, int initial_angle)
{
    int status;
    u32 high_ns;

    status = XTmrCtr_Initialize(inst, base_addr);
    if (status != XST_SUCCESS) {
        xil_printf("ERROR: Timer init failed @ 0x%08X\r\n", (u32)base_addr);
        return XST_FAILURE;
    }

    high_ns = angle_to_ns(initial_angle);

    XTmrCtr_PwmConfigure(inst, PWM_PERIOD_NS, high_ns);
    XTmrCtr_PwmEnable(inst);

    return XST_SUCCESS;
}

/* Set servo angle — reconfigure PWM duty cycle */
static void servo_set(XTmrCtr *inst, int angle)
{
    u32 high_ns = angle_to_ns(angle);

    XTmrCtr_PwmDisable(inst);
    XTmrCtr_PwmConfigure(inst, PWM_PERIOD_NS, high_ns);
    XTmrCtr_PwmEnable(inst);
}

/* Set all 3 servos to the same angle */
static void servo_set_all(int angle)
{
    servo_set(&pwm0, angle);
    servo_set(&pwm1, angle);
    servo_set(&pwm2, angle);
}

int main()
{
    init_platform();
    xil_printf("\r\n=== SG90 Servo Control ===\r\n");

    /* Initialize all 3 PWM timers, start at 90° (center) */
    if (pwm_init(&pwm0, PWM0_BASEADDR, 90) != XST_SUCCESS) return -1;
    if (pwm_init(&pwm1, PWM1_BASEADDR, 90) != XST_SUCCESS) return -1;
    if (pwm_init(&pwm2, PWM2_BASEADDR, 90) != XST_SUCCESS) return -1;

    xil_printf("All servos initialized at 90 deg (center)\r\n");

    /* Sweep loop: 0 -> 90° -> 180° -> 90° -> repeat */
    int angles[] = {0, 90, 180, 90};
    int num_steps = sizeof(angles) / sizeof(angles[0]);

    while (1) {
        for (int i = 0; i < num_steps; i++) {
            int ang = angles[i];
            xil_printf("Servo -> %d deg\r\n", ang);
            servo_set_all(ang);
            sleep(2);   /* 2 seconds between positions */
        }
    }

    cleanup_platform();
    return 0;
}