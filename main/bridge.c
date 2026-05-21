#include "bridge.h"
#include "ble_hid_manager.h"
#include "ble_hid_report.h"
#include "hid_types.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "BRIDGE";

#define BRIDGE_QUEUE_LENGTH 32
#define BRIDGE_TASK_STACK_SIZE 2048
#define BRIDGE_TASK_PRIORITY 5
#define BRIDGE_SEND_RETRY_COUNT 3
#define BRIDGE_SEND_RETRY_DELAY_MS 2

typedef struct {
    uint8_t modifier;
    uint8_t keys[6];
} key_report_t;

static QueueHandle_t s_key_queue = NULL;
static TaskHandle_t s_bridge_task_handle = NULL;

static void bridge_task(void *arg)
{
    key_report_t report;

    while (1) {
        if (xQueueReceive(s_key_queue, &report, portMAX_DELAY)) {
            if (!ble_hid_is_connected()) {
                continue;
            }

            uint16_t conn_id = ble_hid_get_conn_id();
            bool send_ok = false;

            for (int retry = 0; retry < BRIDGE_SEND_RETRY_COUNT; retry++) {
                esp_err_t ret = ble_hid_send_keyboard_value(
                    conn_id, report.modifier, report.keys, 6);

                if (ret == ESP_OK) {
                    send_ok = true;
                    break;
                }

                ESP_LOGW(TAG, "BLE send failed, retry %d/%d, err=0x%x",
                         retry + 1, BRIDGE_SEND_RETRY_COUNT, ret);
                vTaskDelay(pdMS_TO_TICKS(BRIDGE_SEND_RETRY_DELAY_MS));
            }

            if (!send_ok) {
                ESP_LOGE(TAG, "BLE send failed after %d retries", BRIDGE_SEND_RETRY_COUNT);
            }
        }
    }
}

esp_err_t bridge_init(void)
{
    s_key_queue = xQueueCreate(BRIDGE_QUEUE_LENGTH, sizeof(key_report_t));
    if (s_key_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create key queue");
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ret = xTaskCreatePinnedToCore(
        bridge_task,
        "bridge_task",
        BRIDGE_TASK_STACK_SIZE,
        NULL,
        BRIDGE_TASK_PRIORITY,
        &s_bridge_task_handle,
        0);

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create bridge task");
        vQueueDelete(s_key_queue);
        s_key_queue = NULL;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Bridge initialized");
    return ESP_OK;
}

void bridge_handle_keyboard_report(const uint8_t *data, size_t length)
{
    if (length < sizeof(hid_keyboard_input_report_boot_t)) {
        return;
    }

    hid_keyboard_input_report_boot_t *kb_report = (hid_keyboard_input_report_boot_t *)data;

    key_report_t report;
    report.modifier = kb_report->modifier.val;
    for (int i = 0; i < 6; i++) {
        report.keys[i] = kb_report->key[i];
    }

    BaseType_t ret = xQueueSend(s_key_queue, &report, 0);
    if (ret != pdPASS) {
        ESP_LOGW(TAG, "Key queue full, dropping report");
    }
}
