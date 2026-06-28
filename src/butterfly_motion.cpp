#include <Arduino.h>
#include <math.h>
#include "butterfly_motion.h"
#include "pwm_control.h"
#include "config.h"

static int center_left_us = DEFAULT_CENTER_LEFT_US;
static int center_right_us = DEFAULT_CENTER_RIGHT_US;
static int amplitude_us = DEFAULT_AMPLITUDE_US;
static uint32_t step_delay_us = DEFAULT_STEP_DELAY_US;
static int turn_amp_delta = TURN_AMP_DELTA;
static int climb_amp_delta = CLIMB_AMP_DELTA;
static int climb_speed_delta = CLIMB_SPEED_DELTA;

static bool running = false;
static FlightMode flight_mode = FLIGHT_STOP;
static int point_index = 0;
static uint32_t last_step_us = 0;

static int clamp_center(int us) {
    if (us < 500) us = 500;
    if (us > 2500) us = 2500;
    return us;
}

static int clamp_amp(int amp) {
    if (amp < 0) return 0;
    if (amp > 900) return 900;
    return amp;
}

static uint32_t clamp_delay(uint32_t us) {
    if (us < 1000) return 1000;
    if (us > 100000) us = 100000;
    return us;
}

static void apply_rest_pose() {
    servo_both_us(center_left_us, center_right_us);
}

void butterfly_init() {
    running = false;
    flight_mode = FLIGHT_STOP;
    point_index = 0;
    butterfly_reset_params();
    butterfly_go_center();
}

void butterfly_reset_params() {
    center_left_us = DEFAULT_CENTER_LEFT_US;
    center_right_us = DEFAULT_CENTER_RIGHT_US;
    amplitude_us = DEFAULT_AMPLITUDE_US;
    step_delay_us = DEFAULT_STEP_DELAY_US;
    turn_amp_delta = TURN_AMP_DELTA;
    climb_amp_delta = CLIMB_AMP_DELTA;
    climb_speed_delta = CLIMB_SPEED_DELTA;
}

void butterfly_go_center() {
    running = false;
    flight_mode = FLIGHT_STOP;
    point_index = 0;
    apply_rest_pose();
}

void butterfly_start() {
    butterfly_start_mode(FLIGHT_NEUTRAL);
}

void butterfly_start_mode(FlightMode mode) {
    running = true;
    flight_mode = mode;
    last_step_us = micros();
}

void butterfly_stop() {
    running = false;
    flight_mode = FLIGHT_STOP;
}

void butterfly_set_mode(FlightMode mode) {
    if (mode == FLIGHT_STOP) {
        butterfly_stop();
        return;
    }
    flight_mode = mode;
    if (!running) {
        running = true;
        last_step_us = micros();
    }
}

bool butterfly_is_running() {
    return running;
}

FlightMode butterfly_get_mode() {
    return flight_mode;
}

int butterfly_get_center_left() {
    return center_left_us;
}

int butterfly_get_center_right() {
    return center_right_us;
}

int butterfly_get_amplitude() {
    return amplitude_us;
}

uint32_t butterfly_get_step_delay() {
    return step_delay_us;
}

int butterfly_get_turn_delta() {
    return turn_amp_delta;
}

int butterfly_get_climb_delta() {
    return climb_amp_delta;
}

int butterfly_get_climb_speed_delta() {
    return climb_speed_delta;
}

int butterfly_get_point_index() {
    return point_index;
}

void butterfly_set_center_left(int us) {
    center_left_us = clamp_center(us);
    if (!running) {
        apply_rest_pose();
    }
}

void butterfly_set_center_right(int us) {
    center_right_us = clamp_center(us);
    if (!running) {
        apply_rest_pose();
    }
}

void butterfly_set_amplitude(int us) {
    amplitude_us = clamp_amp(us);
}

void butterfly_set_step_delay(uint32_t us) {
    step_delay_us = clamp_delay(us);
}

void butterfly_set_turn_delta(int us) {
    if (us < 0) us = 0;
    if (us > 400) us = 400;
    turn_amp_delta = us;
}

void butterfly_set_climb_delta(int us) {
    if (us < 0) us = 0;
    if (us > 300) us = 300;
    climb_amp_delta = us;
}

void butterfly_set_climb_speed_delta(int us) {
    if (us < 0) us = 0;
    if (us > 8000) us = 8000;
    climb_speed_delta = us;
}

void butterfly_adjust_amplitude(int delta) {
    butterfly_set_amplitude(amplitude_us + delta);
}

void butterfly_adjust_step_delay(int delta_us) {
    butterfly_set_step_delay((uint32_t)(step_delay_us + delta_us));
}

void butterfly_update() {
    if (!running) {
        return;
    }

    int amp1 = amplitude_us;
    int amp2 = amplitude_us;
    uint32_t delay_us = step_delay_us;

    switch (flight_mode) {
        case FLIGHT_LEFT:
            amp1 = clamp_amp(amplitude_us - turn_amp_delta);
            amp2 = clamp_amp(amplitude_us + turn_amp_delta);
            break;
        case FLIGHT_RIGHT:
            amp1 = clamp_amp(amplitude_us + turn_amp_delta);
            amp2 = clamp_amp(amplitude_us - turn_amp_delta);
            break;
        case FLIGHT_CLIMB:
            amp1 = clamp_amp(amplitude_us + climb_amp_delta);
            amp2 = clamp_amp(amplitude_us + climb_amp_delta);
            delay_us = clamp_delay(step_delay_us - climb_speed_delta);
            break;
        case FLIGHT_DESCEND:
            amp1 = clamp_amp(amplitude_us - climb_amp_delta);
            amp2 = clamp_amp(amplitude_us - climb_amp_delta);
            delay_us = clamp_delay(step_delay_us + climb_speed_delta);
            break;
        case FLIGHT_NEUTRAL:
        default:
            break;
    }

    uint32_t now = micros();
    if (now - last_step_us < delay_us) {
        return;
    }
    last_step_us = now;

    int i = point_index % (2 * HALF_CYCLE_POINTS);
    float angle_rad = PI * i / HALF_CYCLE_POINTS;
    int offset1 = (int)(amp1 * cos(angle_rad));
    int offset2 = (int)(amp2 * cos(angle_rad));

    servo_both_us(center_left_us + offset1, center_right_us - offset2);

    point_index++;
    if (point_index >= 2 * HALF_CYCLE_POINTS) {
        point_index = 0;
    }
}
