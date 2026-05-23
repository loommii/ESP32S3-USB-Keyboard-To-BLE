#include <stdio.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "usb/usb_host.h"

#include "config.h"
#include "usb_host_manager.h"
#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"
#include "ble_hid_manager.h"
#include "ble_hid_report.h"
#include "bridge.h"
#include "led_status.h"
#include "nvs_storage.h"
#include "app_main.h"

static const char *TAG = "USB-BLE-KB";

typedef enum {
    APP_EVENT_HID_HOST = 0,
} app_event_group_t;

typedef struct {
    app_event_group_t event_group;
    struct {
        hid_host_device_handle_t handle;
        hid_host_driver_event_t event;
        void *arg;
    } hid_host_device;
} app_event_queue_t;

static QueueHandle_t s_app_event_queue = NULL;

static const char *hid_proto_name_str[] = {
    "NONE",
    "KEYBOARD",
    "MOUSE"
};

/* USB/BLE 连接状态标记，用于驱动 LED 状态指示 */
static bool s_usb_connected = false;

/* 保存当前 USB 键盘 handle，供 bridge 层发送 LED Report 回传 */
static hid_host_device_handle_t s_keyboard_handle = NULL;

/**
 * @brief 获取当前 USB 键盘 handle
 *
 * bridge_handle_led_report() 通过此接口获取键盘 handle，
 * 调用 hid_class_request_set_report() 将 BLE 端的 LED 状态
 * （CapsLock/NumLock/ScrollLock）转发到 USB 键盘
 */
hid_host_device_handle_t app_get_keyboard_handle(void)
{
    return s_keyboard_handle;
}

/* 根据 USB/BLE/配对状态刷新 LED */
void update_led_status(void)
{
    if (!s_usb_connected) {
        led_status_set(LED_STATE_USB_DISCONNECTD);
    } else if (!ble_hid_is_connected()) {
        led_status_set(LED_STATE_USB_CONNECTED_BLE_DISCONNECTED);
    } else if (ble_hid_is_pairing()) {
        /* 已连接但正在配对中，显示紫色常亮 */
        led_status_set(LED_STATE_PAIRING);
    } else {
        led_status_set(LED_STATE_ALL_CONNECTED);
    }
}

static void hid_host_keyboard_report_callback(const uint8_t *const data, const int length)
{
    hid_keyboard_input_report_boot_t *kb_report = (hid_keyboard_input_report_boot_t *)data;

    if (length < sizeof(hid_keyboard_input_report_boot_t)) {
        return;
    }

    ESP_LOGD(TAG, "KB Report: mod=0x%02x keys=[0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]",
        kb_report->modifier.val,
        kb_report->key[0], kb_report->key[1], kb_report->key[2],
        kb_report->key[3], kb_report->key[4], kb_report->key[5]);

    bridge_handle_keyboard_report(data, length);
}

static void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle,
                                        const hid_host_interface_event_t event,
                                        void *arg)
{
    uint8_t data[64] = {0};
    size_t data_length = 0;
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
        ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(hid_device_handle,
                                                                  data,
                                                                  64,
                                                                  &data_length));

        if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
            if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                hid_host_keyboard_report_callback(data, data_length);
            }
        }
        break;
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HID Device, protocol '%s' DISCONNECTED",
                 hid_proto_name_str[dev_params.proto]);
        if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
            s_usb_connected = false;
            s_keyboard_handle = NULL;  /* 键盘断开，清空 handle */
            update_led_status();
        }
        ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
        break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        ESP_LOGW(TAG, "HID Device, protocol '%s' TRANSFER_ERROR",
                 hid_proto_name_str[dev_params.proto]);
        break;
    default:
        break;
    }
}

static void hid_host_device_event(hid_host_device_handle_t hid_device_handle,
                                  const hid_host_driver_event_t event,
                                  void *arg)
{
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
    case HID_HOST_DRIVER_EVENT_CONNECTED:
        ESP_LOGI(TAG, "HID Device, protocol '%s' CONNECTED", hid_proto_name_str[dev_params.proto]);

        if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
            s_usb_connected = true;
            s_keyboard_handle = hid_device_handle;  /* 保存键盘 handle 供 LED 回传使用 */
            update_led_status();
        }

        const hid_host_device_config_t dev_config = {
            .callback = hid_host_interface_callback,
            .callback_arg = NULL
        };

        ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));
        if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
            ESP_ERROR_CHECK(hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT));
            if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                ESP_ERROR_CHECK(hid_class_request_set_idle(hid_device_handle, 0, 0));
            }
        }
        ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));
        break;
    default:
        break;
    }
}

static void hid_host_device_callback(hid_host_device_handle_t hid_device_handle,
                                     const hid_host_driver_event_t event,
                                     void *arg)
{
    const app_event_queue_t evt_queue = {
        .event_group = APP_EVENT_HID_HOST,
        .hid_host_device.handle = hid_device_handle,
        .hid_host_device.event = event,
        .hid_host_device.arg = arg
    };

    if (s_app_event_queue) {
        xQueueSend(s_app_event_queue, &evt_queue, 0);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "USB-BLE-KB: Hello (version %s, build %s)",
             PROJECT_VERSION, PROJECT_BUILD);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(nvs_storage_init());

    /* 根据 NVS 中的槽位索引设置 MAC 地址和设备名
     * 每个槽位使用不同的 MAC 后缀，使主机识别为不同设备
     * 槽位 0 → MAC 后缀 0 → "USB-BLE-KB-1"
     * 槽位 1 → MAC 后缀 1 → "USB-BLE-KB-2"
     * 槽位 2 → MAC 后缀 2 → "USB-BLE-KB-3"
     * 注意：切换槽位后需重新配对，切回已配对槽位时主机可能自动重连
     */
    uint8_t slot = 0;
    nvs_storage_get_slot(&slot);
    if (slot >= MAX_DEVICE_SLOTS) {
        slot = 0;
    }

    /* 修改 MAC 地址最后一字节（必须在 BLE 初始化之前调用）*/
    uint8_t base_mac[6];
    esp_base_mac_addr_get(base_mac);
    base_mac[5] = (base_mac[5] & 0xF0) | (slot & 0x0F);
    esp_base_mac_addr_set(base_mac);
    ESP_LOGI(TAG, "MAC address: %02x:%02x:%02x:%02x:%02x:%02x",
             base_mac[0], base_mac[1], base_mac[2],
             base_mac[3], base_mac[4], base_mac[5]);

    /* 设置设备名 */
    const char *slot_names[] = {DEVICE_NAME_1, DEVICE_NAME_2, DEVICE_NAME_3};
    ble_hid_set_device_name(slot_names[slot]);
    ESP_LOGI(TAG, "Device slot: %d, name: %s", slot, slot_names[slot]);

    ESP_ERROR_CHECK(led_status_init());

    ESP_ERROR_CHECK(ble_hid_manager_init());

    ESP_ERROR_CHECK(ble_hid_report_init());
    ESP_ERROR_CHECK(bridge_init());

    ESP_ERROR_CHECK(usb_host_manager_init());

    const hid_host_driver_config_t hid_host_driver_config = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 0,
        .callback = hid_host_device_callback,
        .callback_arg = NULL
    };

    ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));

    s_app_event_queue = xQueueCreate(10, sizeof(app_event_queue_t));

    update_led_status();  // 初始化完成后刷新 LED 状态（无 USB 无 BLE 连接时亮红灯）

    ESP_LOGI(TAG, "Waiting for HID Device to be connected");

    app_event_queue_t evt_queue;
    while (1) {
        if (xQueueReceive(s_app_event_queue, &evt_queue, portMAX_DELAY)) {
            if (APP_EVENT_HID_HOST == evt_queue.event_group) {
                hid_host_device_event(evt_queue.hid_host_device.handle,
                                      evt_queue.hid_host_device.event,
                                      evt_queue.hid_host_device.arg);
            }
        }
    }
}
