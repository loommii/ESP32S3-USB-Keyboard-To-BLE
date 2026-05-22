#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"

/* 设备名称配置 */
#define DEVICE_NAME               "Loommii-KB"
#define DEVICE_NAME_1             "Loommii-KB-01"
#define DEVICE_NAME_2             "Loommii-KB-02"   
#define DEVICE_NAME_3             "Loommii-KB-03"

/* 设备制造商 */
#define DEVICE_MANUFACTURER       "Loommii"

/* BLE 设备信息服务：VID/PID/版本 */
#define BLE_VENDOR_ID             0x16C0
#define BLE_PRODUCT_ID            0x05DF
#define BLE_VERSION               0x0100

/* 设备序列号 */
#define DEVICE_SERIAL_NUMBER      "1234567890"

/* 电池电量百分比 */
#define BATTERY_LEVEL             100

/* 设备槽位数量 */
#define NUM_DEVICE_SLOTS          3

/* 启用设备切换功能 (1=启用, 0=禁用) */
#define ENABLE_DEVICE_SWITCHING   1

/* 启用取消配对功能 (1=启用, 0=禁用) */
#define ENABLE_UNPAIR_COMBO       1

/* WS2812 LED GPIO 引脚 */
#define LED_GPIO                  GPIO_NUM_48
/* WS2812 LED 亮度 (0-255) */
#define LED_BRIGHTNESS            4

/* 按键事件队列大小 */
#define KEY_EVENT_QUEUE_SIZE      16

/* BLE 通知重试次数 */
#define BLE_NOTIFY_RETRY_COUNT    3

/* BLE 通知重试延迟 (毫秒) */
#define BLE_NOTIFY_RETRY_DELAY_MS 2

/* NVS 存储命名空间 */
#define NVS_NAMESPACE             "usb_ble_kb"

#endif
