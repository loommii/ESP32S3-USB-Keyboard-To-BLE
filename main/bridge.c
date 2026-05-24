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
static uint8_t s_current_slot = 0;
static bool s_switch_combo_active = false;

/* 取消配对状态 */
static bool s_unpair_pending = false;

/* 配对模式: 通过 USB 键盘输入 BLE 配对码 */
static bool s_pairing_mode = false;          /* 是否处于配对输入模式 */
static uint8_t s_pairing_fifo[6] = {0};      /* 6 位配码 FIFO 滑动窗口 */
static int s_pairing_fifo_count = 0;         /* FIFO 中有效位数 */
/* 上一帧按键状态，用于去抖检测（防止同一按键重复录入） */
static uint8_t s_prev_pairing_keys[6] = {0};

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
        ble_hid_disconnect();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "Rebooting to apply new slot...");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

/**
 * @brief 请求清除当前连接设备的配对信息并断开连接
 *
 * 调用 ble_hid_remove_bond() 断开连接并删除绑定信息
 */
void bridge_request_unpair(void)
{
    if (!ble_hid_is_connected()) {
        ESP_LOGW(TAG, "No BLE connection, skip unpair");
        return;
    }

    s_unpair_pending = true;

    ESP_LOGI(TAG, "Unpair requested");

    ble_hid_remove_bond();
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

/* 检查按键是否已在上一帧中出现（用于去抖） */
static bool is_key_in_set(uint8_t key, const uint8_t *set, int count)
{
    for (int i = 0; i < count; i++) {
        if (set[i] == key) return true;
    }
    return false;
}

/* 配对模式按键处理：FIFO 滑动窗口收集数字，Enter 提交，Esc 取消 */
static void handle_pairing_key(const key_report_t *report)
{
    for (int i = 0; i < 6; i++) {
        uint8_t key = report->keys[i];
        if (key == 0) continue;  /* 空键忽略 */

        /* 去抖：按键已在上一帧中，跳过避免重复 */
        if (is_key_in_set(key, s_prev_pairing_keys, 6)) {
            continue;
        }

        /* 数字键 1~9：HID_KEY_1(0x1E) ~ HID_KEY_9(0x26) 映射为 1~9 */
        if (key >= HID_KEY_1 && key <= HID_KEY_9) {
            int digit = key - HID_KEY_1 + 1;
            if (s_pairing_fifo_count >= 6) {
                memmove(s_pairing_fifo, s_pairing_fifo + 1, 5);
                s_pairing_fifo[5] = digit;
            } else {
                s_pairing_fifo[s_pairing_fifo_count] = digit;
                s_pairing_fifo_count++;
            }
            ESP_LOGI(TAG, "Pairing digit: %d (FIFO: %d/%d)",
                     digit, s_pairing_fifo_count, 6);
        } else if (key == HID_KEY_0) {  /* 数字键 0：单独处理，因为 HID_KEY_0(0x27) 不在 1~9 范围内 */
            int digit = 0;
            if (s_pairing_fifo_count >= 6) {
                memmove(s_pairing_fifo, s_pairing_fifo + 1, 5);
                s_pairing_fifo[5] = digit;
            } else {
                s_pairing_fifo[s_pairing_fifo_count] = digit;
                s_pairing_fifo_count++;
            }
            ESP_LOGI(TAG, "Pairing digit: %d (FIFO: %d/%d)",
                     digit, s_pairing_fifo_count, 6);
        } else if (key == HID_KEY_ENTER || key == HID_KEY_KEYPAD_ENTER) {
            /* Enter：提交 6 位配对码 */
            if (s_pairing_fifo_count == 6) {
                uint32_t passkey = 0;
                for (int j = 0; j < 6; j++) {
                    passkey = passkey * 10 + s_pairing_fifo[j];
                }
                ESP_LOGI(TAG, "Passkey %06lu submitted", (unsigned long)passkey);
                ble_hid_inject_passkey(passkey);
            } else {
                ESP_LOGW(TAG, "Enter pressed with only %d/6 digits", s_pairing_fifo_count);
            }
        } else if (key == HID_KEY_ESC) {
            /* Esc：取消配对 */
            ESP_LOGI(TAG, "Pairing cancelled by user");
            ble_hid_cancel_pairing();
        }
    }

    /* 保存当前帧按键供下一帧去抖使用 */
    memcpy(s_prev_pairing_keys, report->keys, 6);
}

/* BLE 被动发起配对请求：初始化 FIFO，LED 变为紫色常亮，等待键盘输入 */
void bridge_on_pairing_start(void)
{
    s_pairing_mode = true;
    s_pairing_fifo_count = 0;
    memset(s_pairing_fifo, 0, sizeof(s_pairing_fifo));
    memset(s_prev_pairing_keys, 0, sizeof(s_prev_pairing_keys));
    led_status_set(LED_STATE_PAIRING);
    ESP_LOGI(TAG, "PAIRING: type the 6-digit passkey from your screen, then press Enter");
}

/* 配对完成（无论成功或失败）：清除配对状态，更新 LED */
void bridge_on_pairing_done(bool success)
{
    s_pairing_mode = false;
    s_pairing_fifo_count = 0;
    memset(s_pairing_fifo, 0, sizeof(s_pairing_fifo));
    memset(s_prev_pairing_keys, 0, sizeof(s_prev_pairing_keys));

    if (success) {
        ESP_LOGI(TAG, "Pairing succeeded");
        update_led_status();
    } else {
        ESP_LOGI(TAG, "Pairing failed");
        led_status_set(LED_STATE_USB_CONNECTED_BLE_DISCONNECTED);
    }
}

/* 桥接任务：从队列接收 USB 键盘报告，优先处理配对/切槽/解绑等特殊模式 */
static void bridge_task(void *arg)
{
    key_report_t report;

    while (1) {
        if (xQueueReceive(s_key_queue, &report, portMAX_DELAY)) {
            /* 优先检查设备切换组合键（ScrollLock+数字） */
            check_device_switch_combo(&report);

            /* 配对模式下拦截所有按键，不转发到 BLE */
            if (s_pairing_mode) {
                handle_pairing_key(&report);
                continue;
            }

            /* 检查解绑组合键（ScrollLock+Esc） */
            check_unpair_combo(&report);

            /* 未连接时不发 BLE */
            if (!ble_hid_is_connected()) {
                continue;
            }

            /* 通过 BLE HID 转发按键报告（带重试） */
            bool send_ok = false;

            for (int retry = 0; retry < BRIDGE_SEND_RETRY_COUNT; retry++) {
                esp_err_t ret = ble_hid_send_keyboard_value(
                    report.modifier, report.keys, 6);

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

/* USB HOST 回调：将原始键盘报告送入桥接队列 */
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

/* BLE 连接建立回调：重置解绑标志，更新 LED 状态 */
void bridge_on_ble_connected(void)
{
    s_unpair_pending = false;
    update_led_status();
}

/* BLE 断开回调：更新 LED 状态 */
void bridge_on_ble_disconnected(void)
{
    update_led_status();
}
