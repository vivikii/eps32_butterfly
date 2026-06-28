#ifndef PWM_CONTROL_H
#define PWM_CONTROL_H

void servo_init();
void servo_write_us(uint8_t channel, int pulse_us);
void servo_both_us(int pulse1_us, int pulse2_us);
void servo_center_both();

#endif
