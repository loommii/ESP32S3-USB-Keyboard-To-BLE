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

#ifndef BLE_HID_MANAGER_H
#define BLE_HID_MANAGER_H

#include "esp_err.h"
#include <stdint.h>

struct esp_hidd_dev_s;
typedef struct esp_hidd_dev_s esp_hidd_dev_t;

/* 初始化 BLE HID 协议栈（NimBLE）+ 安全配置 */
esp_err_t ble_hid_manager_init(void);

/* 反初始化，释放资源 */
esp_err_t ble_hid_manager_deinit(void);

/* 查询当前 BLE 是否已连接 */
bool ble_hid_is_connected(void);

/* 获取对端蓝牙地址 */
void ble_hid_get_remote_addr(uint8_t addr[6]);

/* 获取 HID 设备句柄 */
esp_hidd_dev_t *ble_hid_get_hid_dev(void);

/* 设置广播设备名 */
void ble_hid_set_device_name(const char *name);

/* 主动断开 BLE 连接 */
esp_err_t ble_hid_disconnect(void);

/* 删除对端绑定信息并断开连接 */
esp_err_t ble_hid_remove_bond(void);

/* 查询是否正在等待用户输入配对码 */
bool ble_hid_is_pairing(void);

/* 注入用户输入的 6 位配对码到 NimBLE 协议栈 */
void ble_hid_inject_passkey(uint32_t passkey);

/* 取消当前配对流程 */
void ble_hid_cancel_pairing(void);

#endif
