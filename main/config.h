#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"

#define DEVICE_NAME               "Loommii-KB"
#define DEVICE_NAME_1             "Loommii-KB-01"
#define DEVICE_NAME_2             "Loommii-KB-02"
#define DEVICE_NAME_3             "Loommii-KB-03"

#define DEVICE_MANUFACTURER       "Loommii"

#define BATTERY_LEVEL             100

#define NUM_DEVICE_SLOTS          3

#define ENABLE_DEVICE_SWITCHING   1

#define LED_GPIO                  GPIO_NUM_48

#define KEY_EVENT_QUEUE_SIZE      16

#define BLE_NOTIFY_RETRY_COUNT    3

#define BLE_NOTIFY_RETRY_DELAY_MS 2

#define NVS_NAMESPACE             "usb_ble_kb"

#endif
