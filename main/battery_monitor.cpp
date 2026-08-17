#include "battery_monitor.hpp"

#include "board_pins.hpp"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_log.h"

#include <algorithm>
#include <cmath>

static const char* TAG = "battery";

static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;
static bool s_have_cali;

static int clamp_percent(int mv) {
    if (mv >= BATTERY_FULL_MV) {
        return 100;
    }
    if (mv <= BATTERY_EMPTY_MV) {
        return 0;
    }
    return (mv - BATTERY_EMPTY_MV) * 100 / (BATTERY_FULL_MV - BATTERY_EMPTY_MV);
}

void battery_monitor_init() {
    adc_oneshot_unit_init_cfg_t unit_cfg = {};
    unit_cfg.unit_id = BOARD_BATTERY_ADC_UNIT;
    unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, BOARD_BATTERY_ADC_CHANNEL, &chan_cfg));

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = BOARD_BATTERY_ADC_UNIT,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali) == ESP_OK) {
        s_have_cali = true;
    }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = BOARD_BATTERY_ADC_UNIT,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_line_fitting(&cali_cfg, &s_cali) == ESP_OK) {
        s_have_cali = true;
    }
#endif
    if (!s_have_cali) {
        ESP_LOGW(TAG, "ADC calibration unavailable, using raw millivolt estimate");
    }
}

BatteryReading battery_monitor_read() {
    BatteryReading out;
    int raw_sum = 0;
    const int samples = 8;
    for (int i = 0; i < samples; ++i) {
        int raw = 0;
        if (adc_oneshot_read(s_adc, BOARD_BATTERY_ADC_CHANNEL, &raw) != ESP_OK) {
            continue;
        }
        raw_sum += raw;
    }
    const int raw_avg = raw_sum / samples;
    int adc_mv = 0;
    if (s_have_cali) {
        adc_cali_raw_to_voltage(s_cali, raw_avg, &adc_mv);
    } else {
        // 12 dB atten ≈ 0..3300 mV over 12-bit full scale.
        adc_mv = raw_avg * 3300 / 4095;
    }

    const int vbat_mv = static_cast<int>(std::lround(static_cast<float>(adc_mv) * BATTERY_DIVIDER_RATIO));
    out.millivolts = vbat_mv;
    out.present = vbat_mv >= BATTERY_UNCONNECTED_MV;
    if (!out.present) {
        ESP_LOGW(TAG, "battery ADC %.3f V (unconnected divider?) — report 100%%",
                 adc_mv / 1000.0f);
        out.percent = 100;
        out.low = false;
        out.millivolts = 0;
        return out;
    }

    out.percent = static_cast<uint8_t>(std::clamp(clamp_percent(vbat_mv), 0, 100));
    out.low = out.percent <= BATTERY_LOW_PERCENT;
    ESP_LOGI(TAG, "VBAT %d mV  %u%%  %s",
             vbat_mv, static_cast<unsigned>(out.percent),
             out.low ? "LOW" : "ok");
    return out;
}
