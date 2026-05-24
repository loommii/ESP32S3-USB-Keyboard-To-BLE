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

#include "led_status.h"
#include "config.h"
#include "led_strip.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

static const char *TAG = "LED";

#define WS2812_GPIO 48
#define BLINK_INTERVAL_MS 500

static led_strip_handle_t s_strip = NULL;
static TimerHandle_t s_blink_timer = NULL;

static uint8_t s_current_r = 0;
static uint8_t s_current_g = 0;
static uint8_t s_current_b = 0;
static bool s_blink_on = true;

/* 闪烁定时器回调：交替亮/灭 LED */
static void blink_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    s_blink_on = !s_blink_on;
    if (s_blink_on) {
        led_strip_set_pixel(s_strip, 0, s_current_r, s_current_g, s_current_b);
    } else {
        led_strip_clear(s_strip);
    }
    led_strip_refresh(s_strip);
}

esp_err_t led_status_init(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = WS2812_GPIO,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip));
    led_strip_clear(s_strip);

    /* 创建 500ms 周期性闪烁定时器 */
    s_blink_timer = xTimerCreate("led_blink", pdMS_TO_TICKS(BLINK_INTERVAL_MS),
                                 pdTRUE, NULL, blink_timer_cb);

    ESP_LOGI(TAG, "LED initialized on GPIO%d", WS2812_GPIO);
    return ESP_OK;
}

/* 停止闪烁，恢复常亮 */
static void stop_blink(void)
{
    if (s_blink_timer && xTimerIsTimerActive(s_blink_timer)) {
        xTimerStop(s_blink_timer, 0);
    }
    s_blink_on = true;
}

/* 启动闪烁定时器 */
static void start_blink(void)
{
    s_blink_on = true;
    if (s_blink_timer) {
        xTimerStart(s_blink_timer, 0);
    }
}

esp_err_t led_status_set(led_state_t state)
{
    uint8_t r = 0, g = 0, b = 0;
    bool blink = false;

    switch (state) {
    case LED_STATE_USB_DISCONNECTD:
        /* USB 未连接：红色常亮 */
        r = LED_BRIGHTNESS;
        break;
    case LED_STATE_USB_CONNECTED_BLE_DISCONNECTED:
        /* USB 已连、BLE 未连：紫色闪烁 */
        r = LED_BRIGHTNESS;
        b = LED_BRIGHTNESS;
        blink = true;
        break;
    case LED_STATE_PAIRING:
        /* 配对中：紫色常亮 */
        r = LED_BRIGHTNESS;
        b = LED_BRIGHTNESS;
        blink = false;
        break;
    case LED_STATE_ALL_CONNECTED:
        /* USB + BLE 全连接：绿色常亮 */
        g = LED_BRIGHTNESS;
        break;
    case LED_STATE_SWITCHING:
        /* 设备切换中：黄色 */
        r = LED_BRIGHTNESS;
        g = LED_BRIGHTNESS / 2;
        break;
    case LED_STATE_ERROR:
        /* 错误状态：红色 */
        r = LED_BRIGHTNESS;
        break;
    default:
        stop_blink();
        led_strip_clear(s_strip);
        led_strip_refresh(s_strip);
        return ESP_OK;
    }

    s_current_r = r;
    s_current_g = g;
    s_current_b = b;

    if (blink) {
        start_blink();
        s_blink_on = true;
    } else {
        stop_blink();
    }

    led_strip_set_pixel(s_strip, 0, r, g, b);
    led_strip_refresh(s_strip);
    return ESP_OK;
}
