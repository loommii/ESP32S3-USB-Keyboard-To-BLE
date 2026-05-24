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
