#include "led_status.h"
#include "config.h"
#include "led_strip.h"
#include "esp_log.h"

static const char *TAG = "LED";

#define WS2812_GPIO 48

static led_strip_handle_t s_strip = NULL;

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
    ESP_LOGI(TAG, "LED initialized on GPIO%d", WS2812_GPIO);
    return ESP_OK;
}

esp_err_t led_status_set(led_state_t state)
{
    switch (state) {
    case LED_STATE_USB_DISCONNECTD:
        /* 红色: USB未连接 */
        led_strip_set_pixel(s_strip, 0, LED_BRIGHTNESS, 0, 0);
        break;
    case LED_STATE_USB_CONNECTED_BLE_DISCONNECTED:
        /* 紫色: USB已连接，BLE未连接 */
        led_strip_set_pixel(s_strip, 0, LED_BRIGHTNESS, 0, LED_BRIGHTNESS);
        break;
    case LED_STATE_ALL_CONNECTED:
        /* 绿色: USB和BLE均已连接 */
        led_strip_set_pixel(s_strip, 0, 0, LED_BRIGHTNESS, 0);
        break;
    case LED_STATE_SWITCHING:
        /* 橙色: 正在切换设备槽位 */
        led_strip_set_pixel(s_strip, 0, LED_BRIGHTNESS, LED_BRIGHTNESS / 2, 0);
        break;
    case LED_STATE_ERROR:
        /* 红色: 错误状态 */
        led_strip_set_pixel(s_strip, 0, LED_BRIGHTNESS, 0, 0);
        break;
    default:
        led_strip_clear(s_strip);
        return ESP_OK;
    }
    led_strip_refresh(s_strip);
    return ESP_OK;
}
