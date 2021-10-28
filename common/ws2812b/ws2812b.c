/* RMT example -- RGB LED Strip

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include "ws2812b.h"

static const char *TAG = "RMT";
#define RMT_TX_CHANNEL RMT_CHANNEL_0

static led_strip_t *strip;

void WS2812B_Init(int tx_pin, int number_led)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(tx_pin, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(number_led, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }
    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(strip->clear(strip, 100));
}

void WS2812B_Set_Color(int index, int r, int g, int b)
{
    ESP_ERROR_CHECK(strip->set_pixel(strip, index, 0, 255, 0));
}

void WS2812B_Update_Color(void)
{
    ESP_ERROR_CHECK(strip->refresh(strip, 100));
}