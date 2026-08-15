#pragma once

#include <cstdint>
#include "esp_sleep.h"

void power_save_init();
void power_save_note_activity();
bool power_save_should_sleep(bool paired, uint16_t ble_links);
void power_save_enter_deep_sleep(bool door_open);
esp_sleep_wakeup_cause_t power_save_wakeup_cause();
