idf_component_register(
    SRCS
        "main.cpp"
        "hall_sensor.cpp"
        "battery_monitor.cpp"
        "power_save.cpp"
        "hap_pairing.cpp"
        "setup_code.cpp"
    INCLUDE_DIRS "."
    REQUIRES hap_wrapper yahap-pal esp_event nvs_flash driver esp_timer esp_adc esp_pm bt libsodium
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
