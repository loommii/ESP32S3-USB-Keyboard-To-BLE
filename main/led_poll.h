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

/**
 * @author loommii
 * @file led_poll.h
 * @brief LED 轮询模块 — 通过轮询本地 GATT 属性读取 BLE 主机下发的 CapsLock
 *        /NumLock/ScrollLock 输出报告，转发至 USB 键盘 LED。
 *
 * 设计背景：NimBLE 后端 (esp_hidd) 的 ble_svc_hid 不主动上抛
 * ESP_HIDD_OUTPUT_EVENT，故采用 100ms 周期轮询替代事件驱动。
 * 轮询使用 ble_att_svr_read_local() 读本机内存，不涉及空中通信，
 * 开销微秒级，对 CapsLock/NumLock/ScrollLock 灯效人眼无感。
 */

#ifndef LED_POLL_H
#define LED_POLL_H

#include "esp_err.h"

/**
 * @brief 启动 LED 轮询任务
 *
 * 在 core 1 创建优先级 2 的轮询任务。先延迟 3 秒等待 GATT 服务注册完成，
 * 然后通过扫描 Report Reference Descriptor 查找 Output Report 特征值句柄。
 * 连接后每 100ms 读取一次输出报告，检测到 LED 位图变化时调用
 * bridge_handle_led_report() 发送给 USB 键盘。
 *
 * 任务句柄发现失败时自动退出，不影响系统其余功能正常运行。
 *
 * @return ESP_OK 任务创建成功
 */
esp_err_t led_poll_init(void);

#endif
