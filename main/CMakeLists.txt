idf_component_register(
    SRCS
        "app_main.c"
        "usb_host_manager.c"
        "ble_hid_manager.c"
        "ble_hid_report.c"
        "bridge.c"
        "led_status.c"
        "nvs_storage.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_driver_gpio
        nvs_flash
        bt
        led_strip
        usb_host_hid
        esp_hid
)
