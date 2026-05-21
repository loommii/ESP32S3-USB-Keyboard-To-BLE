#ifndef BLE_HID_MANAGER_H
#define BLE_HID_MANAGER_H

#include "esp_err.h"
#include "esp_bt_defs.h"
#include <stdint.h>

esp_err_t ble_hid_manager_init(void);

esp_err_t ble_hid_manager_deinit(void);

bool ble_hid_is_connected(void);

uint16_t ble_hid_get_conn_id(void);

const esp_bd_addr_t *ble_hid_get_remote_addr(void);

void ble_hid_set_device_name(const char *name);

#endif
