#include "led_status.h"
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
        led_strip_set_pixel(s_strip, 0, 255, 0, 0);
        break;
    case LED_STATE_USB_CONNECTED_BLE_DISCONNECTED:
        led_strip_set_pixel(s_strip, 0, 255, 255, 0);
        break;
    case LED_STATE_ALL_CONNECTED:
        led_strip_set_pixel(s_strip, 0, 0, 255, 0);
        break;
    case LED_STATE_SWITCHING:
        led_strip_set_pixel(s_strip, 0, 0, 0, 255);
        break;
    case LED_STATE_ERROR:
        led_strip_set_pixel(s_strip, 0, 255, 0, 0);
        break;
    default:
        led_strip_clear(s_strip);
        return ESP_OK;
    }
    led_strip_refresh(s_strip);
    return ESP_OK;
}
