#include "nvs_storage.h"

esp_err_t nvs_storage_init(void) {
    return ESP_OK;
}

esp_err_t nvs_storage_get_slot(uint8_t *slot) {
    *slot = 0;
    return ESP_OK;
}

esp_err_t nvs_storage_set_slot(uint8_t slot) {
    return ESP_OK;
}
