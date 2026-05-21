#include "nvs_storage.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG = "NVS";

#define NVS_NAMESPACE "usb_ble_kb"
#define NVS_KEY_CURRENT_SLOT "cur_slot"

static nvs_handle_t s_nvs_handle = 0;

esp_err_t nvs_storage_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: 0x%x", ret);
        return ret;
    }

    ESP_LOGI(TAG, "NVS storage initialized");
    return ESP_OK;
}

esp_err_t nvs_storage_get_slot(uint8_t *slot)
{
    if (s_nvs_handle == 0) {
        ESP_LOGE(TAG, "NVS not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = nvs_get_u8(s_nvs_handle, NVS_KEY_CURRENT_SLOT, slot);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        *slot = 0;
        return ESP_OK;
    }
    return ret;
}

esp_err_t nvs_storage_set_slot(uint8_t slot)
{
    if (s_nvs_handle == 0) {
        ESP_LOGE(TAG, "NVS not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = nvs_set_u8(s_nvs_handle, NVS_KEY_CURRENT_SLOT, slot);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set slot: 0x%x", ret);
        return ret;
    }

    ret = nvs_commit(s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: 0x%x", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Current slot set to %d", slot);
    return ESP_OK;
}
