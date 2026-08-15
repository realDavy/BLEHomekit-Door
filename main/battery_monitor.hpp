#pragma once

#include <cstdint>

struct BatteryReading {
    int millivolts = 0;
    uint8_t percent = 100;
    bool low = false;
    bool present = false;
};

void battery_monitor_init();
BatteryReading battery_monitor_read();
