#ifndef LED_STATUS_H
#define LED_STATUS_H

#include "esp_err.h"

/* LED 状态枚举 */
typedef enum {
    LED_STATE_USB_DISCONNECTD = 0,             /* USB 未连接，红色常亮 */
    LED_STATE_USB_CONNECTED_BLE_DISCONNECTED,  /* USB 已连 BLE 未连，紫色闪烁 */
    LED_STATE_ALL_CONNECTED,                   /* USB + BLE 全连接，绿色常亮 */
    LED_STATE_PAIRING,                         /* 配对中，紫色常亮 */
    LED_STATE_SWITCHING,                       /* 设备切换中，黄色 */
    LED_STATE_ERROR,                            /* 错误，红色 */
} led_state_t;

esp_err_t led_status_init(void);

esp_err_t led_status_set(led_state_t state);

#endif
