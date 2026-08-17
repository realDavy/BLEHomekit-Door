#pragma once

// ESP32-C3 coin-cell HomeKit door (side-sensing hall). No LCD, encoder, or LED.
// Hall is on GPIO5 so deep-sleep GPIO wakeup works (C3 only wakes from GPIO0–5).
// Do not use GPIO11–17: those are in-package SPI flash (CS=14).

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#ifndef BOARD_HALL_GPIO
#define BOARD_HALL_GPIO            GPIO_NUM_5
#endif

#ifndef BOARD_BATTERY_ADC_GPIO
#define BOARD_BATTERY_ADC_GPIO     GPIO_NUM_1
#define BOARD_BATTERY_ADC_UNIT     ADC_UNIT_1
#define BOARD_BATTERY_ADC_CHANNEL  ADC_CHANNEL_1
#endif

// After the 2N7002 inverter: GPIO high = door closed (usual), GPIO low = door open.
// Magnet near the side-sensing TMAG5233 pulls hall OUT low; the MOSFET then lets
// the 1 MΩ pull-up hold GPIO5 high so a closed door does not burn pull-up current.
#ifndef BOARD_HALL_CLOSED_LEVEL
#define BOARD_HALL_CLOSED_LEVEL    1
#endif

#define HAP_DEVICE_NAME            "Door"
#define HAP_SETUP_ID               "DOOR"

// CR2032 / CR2450: 3.0 V full, ~2.0 V empty. Divider ratio is Vbat / Vadc.
#define BATTERY_DIVIDER_RATIO      2.0f
#define BATTERY_FULL_MV            3000
#define BATTERY_EMPTY_MV           2000
#define BATTERY_LOW_PERCENT        20
#define BATTERY_UNCONNECTED_MV     1800

// After a door event, stay awake this long so the hub can connect.
#define POWER_IDLE_SLEEP_MS        12000
// Periodic wakeup to refresh battery and HAP advertisement.
#define POWER_KEEPALIVE_US         (30ULL * 60ULL * 1000000ULL)
