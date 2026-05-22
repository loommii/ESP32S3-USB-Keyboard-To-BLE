#ifndef HID_TYPES_H
#define HID_TYPES_H

#include <stdint.h>
#include "usb/hid.h"
#include "usb/hid_usage_keyboard.h"

typedef struct {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t key[6];
} hid_keyboard_report_t;

typedef struct {
    uint8_t buttons;
    int8_t  x;
    int8_t  y;
    int8_t  wheel;
} hid_mouse_report_t;

typedef struct {
    uint8_t modifier;
    uint8_t key[6];
    hid_protocol_t protocol;
} key_event_t;
            
typedef struct {
    bool usb_keyboard_connected;
    bool ble_connected;
    uint8_t current_slot;
    uint16_t ble_conn_id;
} system_state_t;

#endif
