#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

#include "esp_err.h"
#include <stdint.h>

esp_err_t nvs_storage_init(void);

esp_err_t nvs_storage_get_slot(uint8_t *slot);

esp_err_t nvs_storage_set_slot(uint8_t slot);

#endif
