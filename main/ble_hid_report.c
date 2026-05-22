#include "ble_hid_report.h"
#include "ble_hid_manager.h"  /* ble_hid_get_hid_dev */
#include "esp_hidd.h"        /* esp_hidd_dev_input_set */

esp_err_t ble_hid_report_init(void)
{
    return ESP_OK;
}

esp_err_t ble_hid_send_keyboard_value(uint8_t modifier, const uint8_t *keys, size_t key_count)
{
    esp_hidd_dev_t *dev = ble_hid_get_hid_dev();  /* 获取 BLE HID 设备句柄 */
    if (dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (key_count > 6) {
        key_count = 6;
    }

    /* 构建 8 字节键盘报告：[modifier][保留][key0..key5]（Boot Protocol 格式） */
    uint8_t report[8] = {modifier, 0, 0, 0, 0, 0, 0, 0};
    for (size_t i = 0; i < key_count; i++) {
        report[2 + i] = keys[i];
    }

    /* 报告 ID 2 = 键盘（1 = 鼠标，3 = 消费控制）；通过 Notification 发送到主机 */
    return esp_hidd_dev_input_set(dev, 0, 2, report, sizeof(report));
}
