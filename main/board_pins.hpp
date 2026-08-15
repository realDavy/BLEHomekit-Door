#pragma once

// ESP32-C3 coin-cell HomeKit door (hall / reed). No LCD, encoder, or LED.
// Hall is on GPIO4 (this board's reed/hall pad). C3 deep-sleep GPIO wakeup
// is GPIO0–5. Do not use GPIO11–17: those are in-package SPI flash (CS=14).

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#ifndef BOARD_HALL_GPIO
#define BOARD_HALL_GPIO            GPIO_NUM_4
#endif

// Hold to GND for ~1.5 s at power-on to clear HomeKit pairings.
#ifndef BOARD_RESET_GPIO
#define BOARD_RESET_GPIO           GPIO_NUM_3
#endif

// VBAT -- 1M -- GPIO1 -- 1M -- GND  (2:1 divider). Leave floating if unused.
#ifndef BOARD_BATTERY_ADC_GPIO
#define BOARD_BATTERY_ADC_GPIO     GPIO_NUM_1
#define BOARD_BATTERY_ADC_UNIT     ADC_UNIT_1
#define BOARD_BATTERY_ADC_CHANNEL  ADC_CHANNEL_1
#endif

// Digital hall / reed: magnet present (door closed) drives the pin this level.
#ifndef BOARD_HALL_CLOSED_LEVEL
#define BOARD_HALL_CLOSED_LEVEL    0
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
