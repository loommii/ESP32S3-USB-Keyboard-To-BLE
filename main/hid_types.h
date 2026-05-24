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
