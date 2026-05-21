#ifndef BRIDGE_H
#define BRIDGE_H

#include "esp_err.h"
#include "hid_types.h"

#define MAX_DEVICE_SLOTS 3

esp_err_t bridge_init(void);

void bridge_handle_keyboard_report(const uint8_t *data, size_t length);

void bridge_handle_led_report(uint8_t led_bitmap);

uint8_t bridge_get_current_slot(void);

void bridge_on_ble_connected(void);

void bridge_on_ble_disconnected(void);

/**
 * @brief 请求清除当前连接设备的配对信息
 * 调用后触发 esp_ble_remove_bond_device()，断开和清除在 GAP 回调中完成
 */
void bridge_request_unpair(void);

#endif
