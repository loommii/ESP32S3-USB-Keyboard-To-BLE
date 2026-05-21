#ifndef LED_STATUS_H
#define LED_STATUS_H

#include "esp_err.h"

typedef enum {
    LED_STATE_USB_DISCONNECTD = 0,
    LED_STATE_USB_CONNECTED_BLE_DISCONNECTED,
    LED_STATE_ALL_CONNECTED,
    LED_STATE_SWITCHING,
    LED_STATE_ERROR,
} led_state_t;

esp_err_t led_status_init(void);

esp_err_t led_status_set(led_state_t state);

#endif
