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

#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "usb/hid_host.h"

/**
 * @brief 获取当前 USB 键盘设备 handle
 *
 * 用于 bridge 层向 USB 键盘发送 SET_REPORT 请求（如 LED 状态回传）
 *
 * @return hid_host_device_handle_t  键盘 handle，未连接时返回 NULL
 */
hid_host_device_handle_t app_get_keyboard_handle(void);

/**
 * @brief 根据 USB / BLE 连接状态更新 LED 指示灯
 */
void update_led_status(void);

#endif
