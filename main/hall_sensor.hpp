#pragma once

#include <cstdint>
#include <functional>

// Side-sensing TMAG5233 + 2N7002 inverter on BOARD_HALL_GPIO.
// GPIO high = door closed (magnet near); GPIO low = door open.

void hall_sensor_init();
bool hall_sensor_is_open();
int hall_sensor_raw_level();
void hall_sensor_poll();
void hall_sensor_log_pin_scan();
void hall_sensor_set_listener(std::function<void(bool open)> listener);
uint32_t hall_sensor_edge_count();
