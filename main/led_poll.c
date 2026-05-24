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
 * @file led_poll.c
 * @brief LED 轮询模块实现
 *
 * 本文件实现 HID Output Report 的发现与轮询读取，解决 NimBLE 后端
 * 不上抛 ESP_HIDD_OUTPUT_EVENT 的问题。通过 ble_att_svr_read_local()
 * 读取本机 GATT 属性值，无蓝牙空中通信开销。
 *
 * @see led_poll.h 模块头文件
 * @see bridge_handle_led_report() 最终将 LED 状态写入 USB 键盘
 */

#include <string.h>
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/ble_att.h"
#include "host/ble_gatt.h"
#include "os/os_mbuf.h"
#include "bridge.h"
#include "ble_hid_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*--------------------------------------------------------------------*/
/*  日志标签                                                            */
/*--------------------------------------------------------------------*/

static const char *TAG = "LED_POLL";

/*--------------------------------------------------------------------*/
/*  静态变量                                                            */
/*--------------------------------------------------------------------*/

static uint16_t s_output_report_handle = 0;    /* Output Report 特征值值句柄 */
static uint8_t  s_prev_led_bits = 0xFF;        /* 上一次 LED 位图，0xFF 确保首次必更新 */
static bool     s_output_handle_found = false;  /* 句柄是否已成功发现 */

/*--------------------------------------------------------------------*/
/*  Output Report 句柄发现                                              */
/*--------------------------------------------------------------------*/

/**
 * @brief 扫描 HID 服务中的 Report Reference Descriptor 定位 Output Report
 *
 * 算法：先用 ble_gatts_find_chr() 找第一个 Report 特征值 (UUID 0x2A4D)，
 * 然后从 val_handle 起递增扫描 +30 个句柄范围。对每个句柄读其内容，
 * 若返回 2 字节且 buf[1] == 0x02（Report Ref type = Output Report），
 * 则 Output Report 的值句柄 = 当前句柄 - 1（因 Output 无 CCCD，
 * Report Ref 紧邻 Value 之后）。
 */
static void find_output_report_handle(void)
{
    /* HID Service = 0x1812, Report = 0x2A4D */
    ble_uuid16_t svc_uuid = BLE_UUID16_INIT(0x1812);
    ble_uuid16_t chr_uuid = BLE_UUID16_INIT(0x2A4D);

    uint16_t def_handle, val_handle;
    int rc = ble_gatts_find_chr(&svc_uuid.u, &chr_uuid.u, &def_handle, &val_handle);
    if (rc != 0) {
        ESP_LOGW(TAG, "Report characteristic not found");
        return;
    }

    /* 从 val_handle 起扫描 +30 范围，逐 handle 读 Report Reference Descriptor */
    for (uint16_t h = val_handle; h < val_handle + 30; h++) {
        struct os_mbuf *om = NULL;
        rc = ble_att_svr_read_local(h, &om);
        if (rc != 0 || om == NULL) {
            if (om) os_mbuf_free_chain(om);
            continue;
        }

        /* Output Report 的 Report Ref 固定返回 2 字节 */
        if (OS_MBUF_PKTLEN(om) == 2) {
            uint8_t buf[2] = {0};
            rc = ble_hs_mbuf_to_flat(om, buf, 2, NULL);
            if (rc == 0 && buf[1] == 0x02) {
                /* Output Report 的值句柄 = Ref 句柄 - 1（无 CCCD） */
                s_output_report_handle = h - 1;
                s_output_handle_found = true;
                ESP_LOGI(TAG, "Output Report: val_handle=%d, rpt_ref_handle=%d, id=%d",
                         s_output_report_handle, h, buf[0]);
                os_mbuf_free_chain(om);
                return;
            }
        }
        os_mbuf_free_chain(om);
    }

    ESP_LOGW(TAG, "OUTPUT Report handle not found");
}

/*--------------------------------------------------------------------*/
/*  LED 轮询任务                                                        */
/*--------------------------------------------------------------------*/

/**
 * @brief LED 轮询任务入口
 *
 * 启动后先延迟 3 秒等待 GATT 服务注册，然后执行句柄发现。
 * 句柄发现失败则打印错误并退出任务。
 *
 * 主循环：
 * - BLE 未连接时重置 s_prev_led_bits 并 500ms 轮询等待连接
 * - 连接后每 100ms 读一次 Output Report，与缓存值比较
 * - 变化时调用 bridge_handle_led_report() 发送给 USB 键盘
 *
 * 任务配置：core 1, 优先级 2（低于 bridge_task 的 prio 5）
 *
 * @param arg 未使用
 */
static void led_poll_task(void *arg)
{
    /* 等待 GATT 服务注册完成 */
    vTaskDelay(pdMS_TO_TICKS(3000));

    find_output_report_handle();
    if (!s_output_handle_found) {
        ESP_LOGE(TAG, "LED poll: handle not found, exiting");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        /* BLE 未连接时跳过轮询，重置缓存确保重连后立即更新 */
        if (!ble_hid_is_connected()) {
            s_prev_led_bits = 0xFF;
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        /* 读 Output Report 特征值（本机内存，无蓝牙通信） */
        struct os_mbuf *om = NULL;
        int rc = ble_att_svr_read_local(s_output_report_handle, &om);
        if (rc == 0 && om != NULL) {
            if (OS_MBUF_PKTLEN(om) >= 1) {
                uint8_t led_bits = 0;
                rc = ble_hs_mbuf_to_flat(om, &led_bits, 1, NULL);
                if (rc == 0 && led_bits != s_prev_led_bits) {
                    ESP_LOGI(TAG, "LED: 0x%02x -> 0x%02x", s_prev_led_bits, led_bits);
                    s_prev_led_bits = led_bits;
                    bridge_handle_led_report(led_bits);
                }
            }
            os_mbuf_free_chain(om);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/*--------------------------------------------------------------------*/
/*  公开 API                                                           */
/*--------------------------------------------------------------------*/

esp_err_t led_poll_init(void)
{
    xTaskCreatePinnedToCore(led_poll_task, "led_poll", 2048, NULL, 2, NULL, 1);
    return ESP_OK;
}
