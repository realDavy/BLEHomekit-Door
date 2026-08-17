#include <esp_event.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <host/ble_att.h>
#include <nvs_flash.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>
#include <sodium.h>

#include "Esp32Ble.hpp"
#include "Esp32Crypto.hpp"
#include "Esp32Platform.hpp"
#include "Esp32Storage.hpp"
#include "hap/AccessoryServer.hpp"
#include "hap/core/Accessory.hpp"
#include "hap/core/Characteristic.hpp"
#include "hap/types/CharacteristicTypes.hpp"
#include "hap/types/ServiceTypes.hpp"

#include <functional>
#include <memory>
#include <string>

#include "battery_monitor.hpp"
#include "board_pins.hpp"
#include "hall_sensor.hpp"
#include "hap_pairing.hpp"
#include "power_save.hpp"
#include "setup_code.hpp"

static const char* TAG = "HAP_DOOR";

class DoorBle : public Esp32Ble {
public:
    using Esp32Ble::Esp32Ble;

    uint16_t att_mtu(uint16_t connection_id) const override {
        const uint16_t mtu = ble_att_mtu(connection_id);
        return mtu >= 23 ? mtu : 23;
    }
};

static hap::AccessoryServer* s_server = nullptr;
static std::shared_ptr<hap::core::Characteristic> s_contact_char;
static std::shared_ptr<hap::core::Characteristic> s_contact_low_bat;
static std::shared_ptr<hap::core::Characteristic> s_battery_level;
static std::shared_ptr<hap::core::Characteristic> s_charging_state;
static std::shared_ptr<hap::core::Characteristic> s_status_low_battery;
static bool s_paired = false;

static std::shared_ptr<hap::core::Characteristic> find_characteristic(
    const std::shared_ptr<hap::core::Service>& service, uint64_t type) {
    for (const auto& ch : service->characteristics()) {
        if (ch->type() == type) {
            return ch;
        }
    }
    return nullptr;
}

static void apply_door_state(bool open, bool from_sensor) {
    const uint8_t contact = open
        ? static_cast<uint8_t>(hap::characteristic::ContactSensorState::NotDetected)
        : static_cast<uint8_t>(hap::characteristic::ContactSensorState::Detected);

    if (s_contact_char) {
        s_contact_char->set_value(contact);
    } else {
        ESP_LOGE(TAG, "ContactSensorState characteristic missing");
    }
    if (from_sensor) {
        power_save_note_activity();
    }
    ESP_LOGW(TAG, "HomeKit contact %s (ContactSensorState=%u %s)",
             open ? "OPEN" : "CLOSED",
             static_cast<unsigned>(contact),
             open ? "NotDetected" : "Detected");
}

static void apply_battery(const BatteryReading& bat) {
    const uint8_t low = bat.low
        ? static_cast<uint8_t>(hap::characteristic::StatusLowBattery::Low)
        : static_cast<uint8_t>(hap::characteristic::StatusLowBattery::Normal);
    const uint8_t charging =
        static_cast<uint8_t>(hap::characteristic::ChargingState::NotChargeable);

    if (s_battery_level) {
        s_battery_level->set_value(bat.percent);
    }
    if (s_charging_state) {
        s_charging_state->set_value(charging);
    }
    if (s_status_low_battery) {
        s_status_low_battery->set_value(low);
    }
    if (s_contact_low_bat) {
        s_contact_low_bat->set_value(low);
    }
}

static void log_pairing_banner(bool paired, const std::string& setup_code) {
    if (paired) {
        ESP_LOGW(TAG, "================================================");
        ESP_LOGW(TAG, "HomeKit BLE already paired (SF=0)");
        ESP_LOGW(TAG, "Remove it in the Home app (keep BLE connected) to unpair.");
        ESP_LOGW(TAG, "================================================");
    } else {
        ESP_LOGI(TAG, "================================================");
        ESP_LOGI(TAG, "HomeKit BLE unpaired (SF=1) — iPhone can add it");
        ESP_LOGI(TAG, "Device: %s", HAP_DEVICE_NAME);
        ESP_LOGI(TAG, "Setup code: %s", setup_code.c_str());
        ESP_LOGI(TAG, "Home -> Add Accessory -> More Options");
        ESP_LOGI(TAG, "================================================");
    }
}

static void log_heap(const char* where) {
    ESP_LOGI(TAG, "heap %s: free=%u largest=%u",
             where,
             static_cast<unsigned>(esp_get_free_heap_size()),
             static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)));
}

static void hap_work_task(void* arg) {
    auto* queue = static_cast<QueueHandle_t>(arg);
    while (true) {
        std::function<void()>* job = nullptr;
        if (xQueueReceive(queue, &job, portMAX_DELAY) == pdTRUE && job) {
            (*job)();
            delete job;
        }
    }
}

static const char* wakeup_reason_str() {
    switch (power_save_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_GPIO:
        return "gpio";
    case ESP_SLEEP_WAKEUP_TIMER:
        return "timer";
    default:
        return "power-on";
    }
}

extern "C" void app_main() {
    ESP_LOGI(TAG, "ESP32-C3 HAP-BLE door starting (wake=%s)", wakeup_reason_str());
    log_heap("boot");

    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_ret);
    hap_wipe_legacy_nvs();

    if (sodium_init() < 0) {
        ESP_LOGE(TAG, "sodium_init failed");
    }

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    auto* work_queue = xQueueCreate(16, sizeof(std::function<void()>*));
    xTaskCreate(hap_work_task, "hap_work", 4096, work_queue, 5, nullptr);
    hap::core::Characteristic::set_dispatcher([work_queue](std::function<void()> fn) {
        auto* job = new std::function<void()>(std::move(fn));
        if (xQueueSend(work_queue, &job, 0) != pdTRUE) {
            delete job;
        }
    });

    hall_sensor_init();
    battery_monitor_init();
    const std::string setup_code = hap_setup_code_from_mac();
    const std::string serial = hap_serial_from_mac();

    static Esp32System system_impl;
    static Esp32Storage storage_impl;
    static Esp32Crypto crypto_impl;

    bool paired = hap_sanitize_pairings(storage_impl);
    if (!hap_align_ble_identity(storage_impl)) {
        paired = false;
    }
    s_paired = paired;

    static DoorBle ble_impl(&storage_impl);
    log_pairing_banner(paired, setup_code);

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(HAP_DEVICE_NAME);
    power_save_init();
    log_heap("after nimble_port_init");

    hap::AccessoryServer::Config config;
    config.system = &system_impl;
    config.storage = &storage_impl;
    config.crypto = &crypto_impl;
    config.ble = &ble_impl;
    config.network = nullptr;
    config.device_name = HAP_DEVICE_NAME;
    config.setup_code = setup_code;
    config.category_id = hap::core::AccessoryCategory::Sensor;
    config.on_identify = []() {
        ESP_LOGW(TAG, "Identify (no LED on this hardware)");
    };
    config.on_pairings_changed = [work_queue](const hap::PairingEvent& event) {
        auto* job = new std::function<void()>([event]() {
            bool paired_now = event.type == hap::PairingEventType::Added;
            if (event.type == hap::PairingEventType::Removed ||
                event.type == hap::PairingEventType::AllRemoved) {
                auto list = storage_impl.get("pairing_list");
                if (list && list->size() > 2) {
                    const std::string raw(list->begin(), list->end());
                    paired_now = raw != "[]";
                } else {
                    paired_now = false;
                }
            }
            s_paired = paired_now;
            power_save_note_activity();
            ESP_LOGI(TAG, "Pairing event %d id=%s paired=%d",
                     static_cast<int>(event.type), event.pairing_id.c_str(), paired_now ? 1 : 0);
            if (paired_now) {
                ESP_LOGW(TAG, "Paired. A distant HomePod/Apple TV makes Home show 未响应.");
            } else {
                ESP_LOGW(TAG, "HomeKit unpaired — iPhone can add this accessory again");
                log_pairing_banner(false, hap_setup_code_from_mac());
            }
        });
        if (xQueueSend(work_queue, &job, 0) != pdTRUE) {
            delete job;
        }
    };

    static hap::AccessoryServer server(std::move(config));
    s_server = &server;

    auto accessory = std::make_shared<hap::core::Accessory>(1);
    auto info_service = hap::service::AccessoryInformationBuilder()
        .name(HAP_DEVICE_NAME)
        .manufacturer("Aidaegis")
        .model("ESP32-C3-Door")
        .serial_number(serial)
        .firmware_revision("1.0.8")
        .hardware_revision("ESP32-C3")
        .on_identify([]() {
            ESP_LOGW(TAG, "Identify (no LED on this hardware)");
        })
        .build();
    accessory->add_service(info_service);

    auto contact_builder = hap::service::ContactSensorBuilder();
    contact_builder.with_name("Door")
        .with_battery_status();
    auto contact_service = contact_builder.build();
    s_contact_char = contact_builder.contact_sensor_state();
    s_contact_low_bat = find_characteristic(
        contact_service, hap::characteristic::kType_StatusLowBattery);
    if (s_contact_char) {
        s_contact_char->on_read([]() -> hap::core::ReadResponse {
            const uint8_t contact = hall_sensor_is_open()
                ? static_cast<uint8_t>(hap::characteristic::ContactSensorState::NotDetected)
                : static_cast<uint8_t>(hap::characteristic::ContactSensorState::Detected);
            return hap::core::Value{contact};
        });
    }
    accessory->add_service(contact_service);

    auto battery_builder = hap::service::BatteryServiceBuilder();
    battery_builder.with_name("Battery");
    auto battery_service = battery_builder.build();
    s_battery_level = find_characteristic(
        battery_service, hap::characteristic::kType_BatteryLevel);
    s_charging_state = find_characteristic(
        battery_service, hap::characteristic::kType_ChargingState);
    s_status_low_battery = find_characteristic(
        battery_service, hap::characteristic::kType_StatusLowBattery);
    accessory->add_service(battery_service);

    apply_door_state(hall_sensor_is_open(), false);
    apply_battery(battery_monitor_read());

    hall_sensor_set_listener([](bool open) {
        apply_door_state(open, true);
    });

    server.add_accessory(accessory);

    ESP_LOGI(TAG, "HAP-BLE advertising as '%s', setup code %s",
             HAP_DEVICE_NAME, setup_code.c_str());
    log_heap("before hap start");
    server.start();
    log_heap("after hap start");
    apply_door_state(hall_sensor_is_open(), true);
    apply_battery(battery_monitor_read());
    ESP_LOGW(TAG, "GPIO%d raw=%d → HomeKit %s (HIGH=closed)",
             static_cast<int>(BOARD_HALL_GPIO),
             hall_sensor_raw_level(),
             hall_sensor_is_open() ? "OPEN" : "CLOSED");
    power_save_note_activity();

    int64_t last_battery_us = esp_timer_get_time();
    int64_t last_gpio_log_us = last_battery_us;

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(50));
        hall_sensor_poll();
        server.tick();

        const int64_t now = esp_timer_get_time();
        if (now - last_gpio_log_us > 2LL * 1000000LL) {
            last_gpio_log_us = now;
            ESP_LOGI(TAG, "GPIO%d=%d %s (HIGH=closed, LOW=open)",
                     static_cast<int>(BOARD_HALL_GPIO),
                     hall_sensor_raw_level(),
                     hall_sensor_is_open() ? "OPEN" : "CLOSED");
            hall_sensor_log_pin_scan();
        }
        if (now - last_battery_us > 30LL * 1000000LL) {
            last_battery_us = now;
            apply_battery(battery_monitor_read());
        }

        if (ble_impl.active_connections() > 0) {
            power_save_note_activity();
        }
        if (power_save_should_sleep(s_paired, ble_impl.active_connections())) {
            apply_battery(battery_monitor_read());
            power_save_enter_deep_sleep(hall_sensor_is_open());
        }
    }
}
