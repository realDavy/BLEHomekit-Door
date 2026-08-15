#include "hall_sensor.hpp"

#include "board_pins.hpp"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"

static const char* TAG = "hall";
static constexpr int64_t kDebounceUs = 40000;

static std::function<void(bool)> s_listener;
static int s_stable_level = 1;
static int s_candidate_level = 1;
static int64_t s_candidate_us;
static uint32_t s_edges;

static bool level_is_open(int level) {
    return level != BOARD_HALL_CLOSED_LEVEL;
}

// Light sleep gates digital GPIO unless the pad stays active and can wake
// the CPU. Wake on the opposite of the current level so a door event
// interrupts idle instead of waiting for the 50 ms poll.
static void sync_light_sleep_wakeup(int level) {
    // ESP32-C3 GPIO wakeup (light and deep) is only GPIO0–5.
    if (BOARD_HALL_GPIO > GPIO_NUM_5) {
        return;
    }
    gpio_wakeup_disable(BOARD_HALL_GPIO);
    const gpio_int_type_t wake = level ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL;
    gpio_wakeup_enable(BOARD_HALL_GPIO, wake);
    esp_sleep_enable_gpio_wakeup();
}

static void commit_level(int level, const char* why) {
    if (level == s_stable_level) {
        return;
    }
    s_stable_level = level;
    ++s_edges;
    sync_light_sleep_wakeup(level);
    const bool open = level_is_open(level);
    ESP_LOGW(TAG, "GPIO%d %s → door %s (%s)",
             static_cast<int>(BOARD_HALL_GPIO),
             level ? "HIGH" : "LOW",
             open ? "OPEN" : "CLOSED",
             why);
    if (s_listener) {
        s_listener(open);
    }
}

void hall_sensor_init() {
    // LCDkit leftover hold on the hall pad (was GPIO5 backlight on some boards).
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis(BOARD_HALL_GPIO);
    gpio_reset_pin(BOARD_HALL_GPIO);
    gpio_sleep_sel_dis(BOARD_HALL_GPIO);

    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << BOARD_HALL_GPIO;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io);
    gpio_set_direction(BOARD_HALL_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOARD_HALL_GPIO, GPIO_PULLUP_ONLY);
    gpio_sleep_sel_dis(BOARD_HALL_GPIO);

    const int level = gpio_get_level(BOARD_HALL_GPIO);
    s_stable_level = level;
    s_candidate_level = level;
    s_candidate_us = esp_timer_get_time();
    sync_light_sleep_wakeup(level);

    // Extra pull-ups on other wakeup GPIOs so a jumper to the wrong pad
    // shows up in the pin scan.
    for (int n : {0, 2, 4, 5}) {
        if (n == static_cast<int>(BOARD_HALL_GPIO)) {
            continue;
        }
        const gpio_num_t pin = static_cast<gpio_num_t>(n);
        gpio_reset_pin(pin);
        gpio_set_direction(pin, GPIO_MODE_INPUT);
        gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    }

    ESP_LOGW(TAG, "GPIO%d initial %s (raw=%d, pull-up, LOW=closed)",
             static_cast<int>(BOARD_HALL_GPIO),
             level_is_open(level) ? "OPEN" : "CLOSED",
             level);
    gpio_dump_io_configuration(stdout, 1ULL << BOARD_HALL_GPIO);
    hall_sensor_log_pin_scan();
}

void hall_sensor_log_pin_scan() {
    // C3 deep-sleep wakeup set is GPIO0–5. GPIO1 is the battery ADC.
    ESP_LOGW(TAG, "pin scan IO0=%d IO2=%d IO3=%d IO4=%d IO5=%d IO14=%d  (0=shorted to GND)",
             gpio_get_level(GPIO_NUM_0),
             gpio_get_level(GPIO_NUM_2),
             gpio_get_level(GPIO_NUM_3),
             gpio_get_level(GPIO_NUM_4),
             gpio_get_level(GPIO_NUM_5),
             gpio_get_level(GPIO_NUM_14));
}

void hall_sensor_poll() {
    const int level = gpio_get_level(BOARD_HALL_GPIO);
    const int64_t now = esp_timer_get_time();
    if (level != s_candidate_level) {
        s_candidate_level = level;
        s_candidate_us = now;
        return;
    }
    if (now - s_candidate_us < kDebounceUs) {
        return;
    }
    commit_level(level, "poll");
}

bool hall_sensor_is_open() {
    return level_is_open(gpio_get_level(BOARD_HALL_GPIO));
}

int hall_sensor_raw_level() {
    return gpio_get_level(BOARD_HALL_GPIO);
}

void hall_sensor_set_listener(std::function<void(bool open)> listener) {
    s_listener = std::move(listener);
}

uint32_t hall_sensor_edge_count() {
    return s_edges;
}
