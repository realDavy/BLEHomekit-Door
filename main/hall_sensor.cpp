#include "hall_sensor.hpp"

#include "board_pins.hpp"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char* TAG = "hall";
static constexpr int kDebounceUs = 40000;

static std::function<void(bool)> s_listener;
static QueueHandle_t s_edge_q;
static esp_timer_handle_t s_debounce;
static volatile int s_raw_level;
static int s_stable_level;
static uint32_t s_edges;

static bool level_is_open(int level) {
    return level != BOARD_HALL_CLOSED_LEVEL;
}

static void IRAM_ATTR hall_isr(void* arg) {
    (void)arg;
    int dummy = 0;
    xQueueOverwriteFromISR(s_edge_q, &dummy, nullptr);
}

static void debounce_cb(void* arg) {
    (void)arg;
    const int level = gpio_get_level(BOARD_HALL_GPIO);
    s_raw_level = level;
    if (level == s_stable_level) {
        return;
    }
    s_stable_level = level;
    ++s_edges;
    const bool open = level_is_open(level);
    ESP_LOGI(TAG, "door %s", open ? "OPEN" : "CLOSED");
    if (s_listener) {
        s_listener(open);
    }
}

static void hall_task(void* arg) {
    (void)arg;
    int dummy = 0;
    while (true) {
        if (xQueueReceive(s_edge_q, &dummy, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        esp_timer_stop(s_debounce);
        esp_timer_start_once(s_debounce, kDebounceUs);
    }
}

void hall_sensor_init() {
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << BOARD_HALL_GPIO;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&io);

    s_stable_level = gpio_get_level(BOARD_HALL_GPIO);
    s_raw_level = s_stable_level;
    ESP_LOGI(TAG, "GPIO%d initial %s (level=%d)",
             static_cast<int>(BOARD_HALL_GPIO),
             level_is_open(s_stable_level) ? "OPEN" : "CLOSED",
             s_stable_level);

    s_edge_q = xQueueCreate(1, sizeof(int));
    const esp_timer_create_args_t targs = {
        .callback = debounce_cb,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "hall_deb",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&targs, &s_debounce));

    const esp_err_t isr = gpio_install_isr_service(0);
    if (isr != ESP_OK && isr != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(isr);
    }
    gpio_isr_handler_add(BOARD_HALL_GPIO, hall_isr, nullptr);
    xTaskCreate(hall_task, "hall", 2048, nullptr, 6, nullptr);
}

bool hall_sensor_is_open() {
    return level_is_open(s_stable_level);
}

void hall_sensor_set_listener(std::function<void(bool open)> listener) {
    s_listener = std::move(listener);
}

uint32_t hall_sensor_edge_count() {
    return s_edges;
}
