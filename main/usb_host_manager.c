#include "usb_host_manager.h"
#include "usb/usb_host.h"
#include "esp_log.h"

static const char *TAG = "USB_HOST";

static TaskHandle_t s_usb_lib_task_handle = NULL;

static void usb_lib_task(void *arg)
{
    TaskHandle_t main_task_handle = (TaskHandle_t)arg;

    ESP_LOGI(TAG, "Installing USB Host Library");
    usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    ESP_ERROR_CHECK(usb_host_install(&host_config));
    ESP_LOGI(TAG, "USB Host installed");

    xTaskNotifyGive(main_task_handle);

    bool has_clients = true;
    bool has_devices = false;
    while (has_clients) {
        uint32_t event_flags;
        ESP_ERROR_CHECK(usb_host_lib_handle_events(portMAX_DELAY, &event_flags));
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_LOGI(TAG, "No more clients");
            if (ESP_OK == usb_host_device_free_all()) {
                ESP_LOGI(TAG, "All devices marked as free");
                has_clients = false;
            } else {
                ESP_LOGI(TAG, "Wait for FLAGS_ALL_FREE");
                has_devices = true;
            }
        }
        if (has_devices && event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            ESP_LOGI(TAG, "All devices free");
            has_clients = false;
        }
    }

    ESP_LOGI(TAG, "Uninstalling USB Host Library");
    ESP_ERROR_CHECK(usb_host_uninstall());
    s_usb_lib_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t usb_host_manager_init(void)
{
    BaseType_t task_created = xTaskCreatePinnedToCore(
        usb_lib_task,
        "usb_host",
        4096,
        xTaskGetCurrentTaskHandle(),
        2,
        &s_usb_lib_task_handle,
        0
    );

    if (task_created != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create USB Host task");
        return ESP_FAIL;
    }

    ulTaskNotifyTake(false, 1000);

    return ESP_OK;
}

esp_err_t usb_host_manager_deinit(void)
{
    if (s_usb_lib_task_handle != NULL) {
        vTaskDelete(s_usb_lib_task_handle);
        s_usb_lib_task_handle = NULL;
    }
    return ESP_OK;
}

TaskHandle_t usb_host_manager_get_task_handle(void)
{
    return s_usb_lib_task_handle;
}
