#ifndef BUTTERFLY_MOTION_H
#define BUTTERFLY_MOTION_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    FLIGHT_STOP = 0,
    FLIGHT_NEUTRAL,
    FLIGHT_LEFT,
    FLIGHT_RIGHT,
    FLIGHT_CLIMB,
    FLIGHT_DESCEND
} FlightMode;

void butterfly_init();
void butterfly_update();
void butterfly_reset_params();

void butterfly_start();
void butterfly_start_mode(FlightMode mode);
void butterfly_stop();
void butterfly_go_center();
void butterfly_set_mode(FlightMode mode);

bool butterfly_is_running();
FlightMode butterfly_get_mode();
int butterfly_get_center_left();
int butterfly_get_center_right();
int butterfly_get_amplitude();
uint32_t butterfly_get_step_delay();
int butterfly_get_turn_delta();
int butterfly_get_climb_delta();
int butterfly_get_climb_speed_delta();
int butterfly_get_point_index();

void butterfly_set_center_left(int us);
void butterfly_set_center_right(int us);
void butterfly_set_amplitude(int us);
void butterfly_set_step_delay(uint32_t us);
void butterfly_set_turn_delta(int us);
void butterfly_set_climb_delta(int us);
void butterfly_set_climb_speed_delta(int us);
void butterfly_adjust_amplitude(int delta);
void butterfly_adjust_step_delay(int delta_us);

#endif
