/*
 * SPDX-FileCopyrightText: 2026 loommii
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string.h>
#include "nvs_storage.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "config.h"

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

    /* 检查构建版本——不匹配或为 "auto" 时擦除整个 NVS */
    char stored_build[32] = {0};
    size_t len = sizeof(stored_build);
    esp_err_t err = nvs_get_str(s_nvs_handle, "build", stored_build, &len);

    bool need_erase = (err != ESP_OK)
                      || (strcmp(stored_build, PROJECT_BUILD) != 0)
                      || (strcmp(PROJECT_BUILD, "auto") == 0);

    if (need_erase) {
        ESP_LOGW(TAG, "Build mismatch or forced: [%s] → [%s], erasing NVS...",
                 (err == ESP_OK) ? stored_build : "(none)", PROJECT_BUILD);
        nvs_close(s_nvs_handle);
        nvs_flash_erase();
        nvs_flash_init();
        nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_nvs_handle);
    }

    nvs_set_str(s_nvs_handle, "build", PROJECT_BUILD);
    nvs_commit(s_nvs_handle);

    ESP_LOGI(TAG, "NVS storage initialized (build %s)", PROJECT_BUILD);
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
