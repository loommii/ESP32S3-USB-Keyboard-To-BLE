#ifndef BLE_HID_MANAGER_H
#define BLE_HID_MANAGER_H

#include "esp_err.h"
#include "esp_bt_defs.h"
#include <stdint.h>

struct esp_hidd_dev_s;
typedef struct esp_hidd_dev_s esp_hidd_dev_t;

esp_err_t ble_hid_manager_init(void);

esp_err_t ble_hid_manager_deinit(void);

bool ble_hid_is_connected(void);

const esp_bd_addr_t *ble_hid_get_remote_addr(void);

esp_hidd_dev_t *ble_hid_get_hid_dev(void);

void ble_hid_set_device_name(const char *name);

#endif
