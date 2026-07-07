/* servo.h - hobby servo (SG90 class) on PWM_0, DIP 10 */
#ifndef SERVO_H
#define SERVO_H

void servo_init(void);          /* 50 Hz, starts at 90 degrees */
void servo_set_angle(int deg);  /* clamped to 0..180 */
int  servo_get_angle(void);

#endif
