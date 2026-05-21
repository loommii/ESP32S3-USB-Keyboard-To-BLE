#include "ble_hid_report.h"
#include "esp_hidd_prf_api.h"
#include "esp_log.h"

static const char *TAG = "BLE_REPORT";

esp_err_t ble_hid_report_init(void)
{
    return ESP_OK;
}

esp_err_t ble_hid_send_keyboard_value(uint16_t conn_id, uint8_t modifier, const uint8_t *keys, size_t key_count)
{
    if (key_count > 6) {
        ESP_LOGW(TAG, "key_count %d exceeds max 6, truncating", key_count);
        key_count = 6;
    }

    uint8_t keyboard_cmd[6] = {0};
    for (size_t i = 0; i < key_count; i++) {
        keyboard_cmd[i] = keys[i];
    }

    esp_hidd_send_keyboard_value(conn_id, modifier, keyboard_cmd, key_count);
    return ESP_OK;
}
