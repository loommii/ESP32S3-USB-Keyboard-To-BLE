#ifndef BLE_HID_REPORT_H
#define BLE_HID_REPORT_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

esp_err_t ble_hid_report_init(void);

esp_err_t ble_hid_send_keyboard_value(uint8_t modifier, const uint8_t *keys, size_t key_count);

#endif
