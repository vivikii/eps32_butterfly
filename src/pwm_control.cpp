#include <Arduino.h>
#include "pwm_control.h"
#include "config.h"

static int center_us = DEFAULT_CENTER_LEFT_US;

static uint32_t us_to_duty(int pulse_us) {
    if (pulse_us < 500) pulse_us = 500;
    if (pulse_us > 2500) pulse_us = 2500;
    uint32_t max_duty = (1u << SERVO_RES) - 1;
    return (uint32_t)pulse_us * max_duty / 20000u;
}

void servo_init() {
    ledcSetup(0, SERVO_FREQ, SERVO_RES);
    ledcSetup(1, SERVO_FREQ, SERVO_RES);
    ledcAttachPin(SERVO1_PIN, 0);
    ledcAttachPin(SERVO2_PIN, 1);
    servo_center_both();
}

void servo_write_us(uint8_t channel, int pulse_us) {
    ledcWrite(channel, us_to_duty(pulse_us));
}

void servo_both_us(int pulse1_us, int pulse2_us) {
    servo_write_us(0, pulse1_us);
    servo_write_us(1, pulse2_us);
}

void servo_center_both() {
    servo_both_us(center_us, center_us);
}
