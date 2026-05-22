#include "bridge.h"
#include "ble_hid_manager.h"
#include "ble_hid_report.h"
#include "hid_types.h"
#include "usb/hid_usage_keyboard.h"
#include "usb/hid_host.h"
#include "nvs_storage.h"
#include "led_status.h"
#include "app_main.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

void update_led_status(void);

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
static uint8_t s_current_slot = 0;
static bool s_switch_combo_active = false;

/* 取消配对状态 */
static bool s_unpair_pending = false;
static esp_bd_addr_t s_unpair_addr = {0};

/**
 * @brief 切换设备槽位
 *
 * 流程：保存槽位到 NVS → 断开 BLE 连接 → 重启
 * 重启后 app_main() 根据新槽位设置 MAC 地址和设备名
 * 每个槽位使用不同 MAC 后缀，主机识别为不同设备
 * 注意：首次切换到新槽位需重新配对，切回已配对槽位时主机可能自动重连
 *
 * @param new_slot  目标槽位 (0-2)
 */
static void switch_device_slot(uint8_t new_slot)
{
    if (new_slot >= MAX_DEVICE_SLOTS) {
        return;
    }

    ESP_LOGI(TAG, "Switching device slot from %d to %d", s_current_slot, new_slot);

    led_status_set(LED_STATE_SWITCHING);

    nvs_storage_set_slot(new_slot);

    if (ble_hid_is_connected()) {
        esp_bd_addr_t remote_addr;
        memcpy(remote_addr, *ble_hid_get_remote_addr(), sizeof(esp_bd_addr_t));
        esp_ble_gap_disconnect(remote_addr);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "Rebooting to apply new slot...");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

/**
 * @brief 请求清除当前连接设备的配对信息并断开连接
 *
 * 流程：调用 esp_ble_remove_bond_device() → 等待 GAP 回调
 *       → 回调中调用 esp_ble_gap_disconnect() → 自动重新广播
 *
 * 符合官方最佳实践：等待 ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT 后再断开连接
 */
void bridge_request_unpair(void)
{
    if (!ble_hid_is_connected()) {
        ESP_LOGW(TAG, "No BLE connection, skip unpair");
        return;
    }

    s_unpair_pending = true;
    memcpy(s_unpair_addr, *ble_hid_get_remote_addr(), sizeof(esp_bd_addr_t));

    ESP_LOGI(TAG, "Unpair requested for %02x:%02x:%02x:%02x:%02x:%02x",
             s_unpair_addr[0], s_unpair_addr[1], s_unpair_addr[2],
             s_unpair_addr[3], s_unpair_addr[4], s_unpair_addr[5]);

    esp_ble_remove_bond_device(s_unpair_addr);
}

static void check_unpair_combo(const key_report_t *report)
{
    bool scroll_lock_pressed = false;
    bool has_esc_key = false;

    for (int i = 0; i < 6; i++) {
        if (report->keys[i] == HID_KEY_SCROLL_LOCK) {
            scroll_lock_pressed = true;
        }
        if (report->keys[i] == HID_KEY_ESC) {
            has_esc_key = true;
        }
    }

    if (scroll_lock_pressed && has_esc_key && !s_unpair_pending) {
        bridge_request_unpair();
    }
}

static void check_device_switch_combo(const key_report_t *report)
{
    bool scroll_lock_pressed = false;
    uint8_t target_slot = 0;
    bool has_number_key = false;

    for (int i = 0; i < 6; i++) {
        if (report->keys[i] == HID_KEY_SCROLL_LOCK) {
            scroll_lock_pressed = true;
        }
        if (report->keys[i] == HID_KEY_1) {
            target_slot = 0;
            has_number_key = true;
        } else if (report->keys[i] == HID_KEY_2) {
            target_slot = 1;
            has_number_key = true;
        } else if (report->keys[i] == HID_KEY_3) {
            target_slot = 2;
            has_number_key = true;
        }
    }

    if (scroll_lock_pressed && has_number_key && !s_switch_combo_active) {
        if (target_slot != s_current_slot) {
            s_switch_combo_active = true;
            switch_device_slot(target_slot);
        }
    }

    if (!scroll_lock_pressed) {
        s_switch_combo_active = false;
    }
}

static void bridge_task(void *arg)
{
    key_report_t report;

    while (1) {
        if (xQueueReceive(s_key_queue, &report, portMAX_DELAY)) {
            check_device_switch_combo(&report);
            check_unpair_combo(&report);

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
    nvs_storage_get_slot(&s_current_slot);
    ESP_LOGI(TAG, "Current device slot: %d", s_current_slot);

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

/**
 * @brief 处理 BLE 主机发来的 LED Report（CapsLock/NumLock/ScrollLock 状态）
 *
 * 数据流：PC 按 CapsLock → BLE GATT Write → ESP_HIDD_EVENT_BLE_LED_REPORT_WRITE_EVT
 *        → 此函数 → hid_class_request_set_report → USB 键盘 LED 亮灭
 *
 * LED Report 字节定义（bit 位掩码）：
 *   bit 0 = Num Lock
 *   bit 1 = Caps Lock
 *   bit 2 = Scroll Lock
 *
 * @param led_bitmap  LED 状态字节，来自 BLE 主机的 Output Report
 */
void bridge_handle_led_report(uint8_t led_bitmap)
{
    hid_host_device_handle_t kb_handle = app_get_keyboard_handle();
    if (kb_handle == NULL) {
        ESP_LOGW(TAG, "No keyboard connected, skip LED report");
        return;
    }

    /* 通过 USB HID SET_REPORT 将 LED 状态发送给 USB 键盘
     * report_type = 0x02 (Output Report)
     * report_id   = 0    (Boot Protocol 无 Report ID)
     */
    uint8_t data = led_bitmap;
    esp_err_t ret = hid_class_request_set_report(kb_handle, 0x02, 0, &data, 1);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send LED report to USB keyboard: 0x%x", ret);
    } else {
        ESP_LOGD(TAG, "LED report sent: 0x%02x", led_bitmap);
    }
}

uint8_t bridge_get_current_slot(void)
{
    return s_current_slot;
}

void bridge_on_ble_connected(void)
{
    update_led_status();
}

void bridge_on_ble_disconnected(void)
{
    update_led_status();
}
