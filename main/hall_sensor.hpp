#pragma once

#include <cstdint>
#include <functional>

// Digital hall / reed switch on BOARD_HALL_GPIO.
// Closed (magnet present) maps to HomeKit Contact Detected / door position 0.

void hall_sensor_init();
bool hall_sensor_is_open();
void hall_sensor_set_listener(std::function<void(bool open)> listener);

// Count open/close edges since boot (for factory-reset gesture).
uint32_t hall_sensor_edge_count();
