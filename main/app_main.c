/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "http_server_app.h"
#include "output_iot.h"
#include "dht11.h"
#include "ledc_app.h"
#include "ws2812b.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "TP-LINK_T3"
#define EXAMPLE_ESP_WIFI_PASS      "Hoithangtoan"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
static struct dht11_reading dht11_last_data, dht11_cur_data;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT ,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);

    // esp_wifi_disconnect();
    // esp_wifi_stop();
    // strcpy((char*)wifi_config.sta.ssid,"Van Phong Mentory");
    // strcpy((char*)wifi_config.sta.password,"onthineu");
    // ESP_ERROR_CHECK(esp_wifi_init(&cfg));    
    // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    // ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    // wifi_config_t wifi_config_get;
    // esp_wifi_get_config(WIFI_IF_STA, &wifi_config_get);
    // printf("--> ssid: %s\n", wifi_config_get.sta.ssid);
    // printf("--> pass: %s\n", wifi_config_get.sta.password);
    // esp_wifi_start();
    // bits = xEventGroupWaitBits(s_wifi_event_group,
    //         WIFI_CONNECTED_BIT ,
    //         pdTRUE,
    //         pdFALSE,
    //         portMAX_DELAY);
}

void switch_data_callback(char *data, int len)
{
    if(*data == '1')
    {
        output_io_set_level(GPIO_NUM_2,1);
    }
    else if(*data == '0')
    {
        output_io_set_level(GPIO_NUM_2,0);
    }
}

void slider_data_callback(char *data, int len)
{
    char number_str[10];
    memcpy(number_str, data, len + 1);
    int duty = atoi(number_str);
    printf("%d\n", duty);
    LedC_Set_Duty(0,duty);
}

void rgb_data_callback(char *data, int len)
{
    printf("RGB: %s\n", data);
}


void dht11_data_callback(void)
{
    char resp[100];
    sprintf(resp, "{\"temperature\": \"%.1f\",\"humidity\": \"%.1f\"}", dht11_last_data.temperature, dht11_last_data.humidity);
    dht11_response(resp, strlen(resp));
}

void wifi_data_callback(char *data, int len)
{
    //"Deviot2@12345679@"
    char ssid[30] = "";
    char pass[30] = "";
    printf("->data: %s\n", data);
    char *pt = strtok(data, "@");
    if(pt)
        strcpy(ssid, pt);
    pt = strtok(NULL, "@");
    if(pt)    
        strcpy(pass, pt);

    printf("->ssid: %s\n", ssid);
    printf("->pass: %s\n", pass);

    //stop_webserver();
    esp_wifi_disconnect();
    esp_wifi_stop();
    wifi_config_t wifi_config = {
        .sta = {
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    strcpy((char*)wifi_config.sta.ssid,ssid);
    strcpy((char*)wifi_config.sta.password,pass);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    // wifi_config_t wifi_config_get;
    // esp_wifi_get_config(WIFI_IF_STA, &wifi_config_get);
    // printf("--> ssid: %s\n", wifi_config_get.sta.ssid);
    // printf("--> pass: %s\n", wifi_config_get.sta.password);
    esp_wifi_start();
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT ,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);
    //start_webserver();        
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    http_set_callback_switch(switch_data_callback);
    http_set_callback_dht11(dht11_data_callback);
    http_set_callback_slider(slider_data_callback);
    http_set_callback_wifiinfo(wifi_data_callback);
    http_set_rgb_callback(rgb_data_callback);

    // output_io_create(GPIO_NUM_2);
    DHT11_init(GPIO_NUM_4);
    LedC_Init();
    LedC_Add_Pin(GPIO_NUM_2,0);

    WS2812B_Init(GPIO_NUM_15, 8);
    
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    start_webserver();

    while(1)
    {
        dht11_cur_data = DHT11_read();
        if(dht11_cur_data.status == 0)// READ OK
        {
            dht11_last_data = dht11_cur_data;
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
