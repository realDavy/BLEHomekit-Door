#include "power_save.hpp"

#include "board_pins.hpp"

#include "driver/gpio.h"
#include "esp_bt.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "nimble/nimble_port.h"

static const char* TAG = "power";
static int64_t s_last_activity_us;

void power_save_init() {
    s_last_activity_us = esp_timer_get_time();

#if CONFIG_PM_ENABLE
    esp_pm_config_t pm = {
        .max_freq_mhz = 80,
        .min_freq_mhz = 10,
        .light_sleep_enable = true,
    };
    const esp_err_t err = esp_pm_configure(&pm);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_pm_configure failed: %s", esp_err_to_name(err));
    }
#endif

    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_N0);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_N0);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_N0);
}

void power_save_note_activity() {
    s_last_activity_us = esp_timer_get_time();
}

bool power_save_should_sleep(bool paired, uint16_t ble_links) {
    if (!paired || ble_links > 0) {
        return false;
    }
    const int64_t idle_us = esp_timer_get_time() - s_last_activity_us;
    return idle_us >= static_cast<int64_t>(POWER_IDLE_SLEEP_MS) * 1000;
}

static void isolate_unused_gpios(gpio_num_t keep) {
    // ESP32-C3: skip USB-Serial/JTAG (18/19) and UART0 (20/21) if a debug
    // cable is attached; floating unused pins still waste microamps.
    for (int n = 0; n <= 21; ++n) {
        if (n == static_cast<int>(keep)) {
            continue;
        }
        if (n == 18 || n == 19 || n == 20 || n == 21) {
            continue;
        }
        const gpio_num_t pin = static_cast<gpio_num_t>(n);
        gpio_reset_pin(pin);
    }
}

void power_save_enter_deep_sleep(bool door_open) {
    ESP_LOGW(TAG, "deep sleep (door %s, GPIO wakeup + %llu us timer)",
             door_open ? "OPEN" : "CLOSED",
             static_cast<unsigned long long>(POWER_KEEPALIVE_US));

    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << BOARD_HALL_GPIO;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io);

    isolate_unused_gpios(BOARD_HALL_GPIO);

    gpio_sleep_sel_en(BOARD_HALL_GPIO);
    gpio_sleep_set_direction(BOARD_HALL_GPIO, GPIO_MODE_INPUT);
    gpio_sleep_set_pull_mode(BOARD_HALL_GPIO, GPIO_PULLUP_ONLY);

    // Level wakeup: opposite of the current door level so the next edge wakes us.
    const int level = gpio_get_level(BOARD_HALL_GPIO);
    const esp_deepsleep_gpio_wake_up_mode_t wake_mode =
        level ? ESP_GPIO_WAKEUP_GPIO_LOW : ESP_GPIO_WAKEUP_GPIO_HIGH;
    ESP_ERROR_CHECK(esp_deep_sleep_enable_gpio_wakeup(1ULL << BOARD_HALL_GPIO, wake_mode));
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(POWER_KEEPALIVE_US));

    int rc = nimble_port_stop();
    if (rc == 0) {
        nimble_port_deinit();
    } else {
        ESP_LOGW(TAG, "nimble_port_stop rc=%d", rc);
    }
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    esp_deep_sleep_start();
}
