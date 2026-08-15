#pragma once

#include <cstdint>
#include <functional>

// Digital hall / reed switch on BOARD_HALL_GPIO.
// Closed (magnet present / pin pulled to GND) maps to HomeKit Contact Detected.

void hall_sensor_init();
bool hall_sensor_is_open();
int hall_sensor_raw_level();
void hall_sensor_poll();
void hall_sensor_set_listener(std::function<void(bool open)> listener);
uint32_t hall_sensor_edge_count();
