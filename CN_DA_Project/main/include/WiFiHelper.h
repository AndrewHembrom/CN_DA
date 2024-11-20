#ifndef WIFIHELPER_H
#define WIFIHELPER_H

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"

#define EXAMPLE_ESP_WIFI_SSID      "Drewski"
#define EXAMPLE_ESP_WIFI_PASS      "hehehaha"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG_WIFI = "wifi station";

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
            ESP_LOGI(TAG_WIFI, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG_WIFI,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void initNVS(){
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void WiFiConnectHelper() {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
	     * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG_WIFI, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG_WIFI, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG_WIFI, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG_WIFI, "UNEXPECTED EVENT");
    }
}
#endif


// #ifndef WIFI_HELPER_H
// #define WIFI_HELPER_H

// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "nvs_flash.h"
// #include "freertos/event_groups.h"

// // Configuration Constants
// #define WIFI_SSID              "Drewski"
// #define WIFI_PASSWORD          "hehehaha"
// #define WIFI_MAXIMUM_RETRY     5

// // Event Group Bits
// #define WIFI_CONNECTED_BIT     BIT0
// #define WIFI_FAIL_BIT          BIT1

// // Logger Tag
// static const char *TAG_WIFI = "WiFiHelper";

// // Globals
// static EventGroupHandle_t wifi_event_group;
// static int retry_count = 0;

// // Function Prototypes
// void initNVS(void);
// void WiFiConnect(void);

// // Static Functions
// static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
// static void createWifiConfig(wifi_config_t* wifi_config);

// // Function Implementations
// void initNVS(void) {
//     // Initialize NVS
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);
//     ESP_LOGI(TAG_WIFI, "NVS initialized.");
// }

// void WiFiConnect(void) {
//     wifi_event_group = xEventGroupCreate();
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     esp_netif_create_default_wifi_sta();

//     wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&init_config));

//     esp_event_handler_instance_t instance_any_id;
//     esp_event_handler_instance_t instance_got_ip;

//     ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
//                                                         ESP_EVENT_ANY_ID,
//                                                         &eventHandler,
//                                                         NULL,
//                                                         &instance_any_id));
//     ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
//                                                         IP_EVENT_STA_GOT_IP,
//                                                         &eventHandler,
//                                                         NULL,
//                                                         &instance_got_ip));

//     wifi_config_t wifi_config;
//     createWifiConfig(&wifi_config);

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_start());

//     ESP_LOGI(TAG_WIFI, "WiFi initialization completed.");

//     // Wait for connection or failure
//     EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
//                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//                                            pdFALSE,
//                                            pdFALSE,
//                                            portMAX_DELAY);

//     if (bits & WIFI_CONNECTED_BIT) {
//         ESP_LOGI(TAG_WIFI, "Connected to AP: SSID=%s, Password=%s", WIFI_SSID, WIFI_PASSWORD);
//     } else if (bits & WIFI_FAIL_BIT) {
//         ESP_LOGI(TAG_WIFI, "Failed to connect to AP: SSID=%s, Password=%s", WIFI_SSID, WIFI_PASSWORD);
//     } else {
//         ESP_LOGE(TAG_WIFI, "Unexpected WiFi event occurred.");
//     }
// }

// static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//         esp_wifi_connect();
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         if (retry_count < WIFI_MAXIMUM_RETRY) {
//             esp_wifi_connect();
//             retry_count++;
//             ESP_LOGI(TAG_WIFI, "Retrying to connect to AP...");
//         } else {
//             xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
//             ESP_LOGE(TAG_WIFI, "Connection failed after maximum retries.");
//         }
//     } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//         ESP_LOGI(TAG_WIFI, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
//         retry_count = 0;
//         xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
//     }
// }

// static void createWifiConfig(wifi_config_t* wifi_config) {
//     memset(wifi_config, 0, sizeof(wifi_config_t));
//     strncpy((char*)wifi_config->sta.ssid, WIFI_SSID, sizeof(wifi_config->sta.ssid) - 1);
//     strncpy((char*)wifi_config->sta.password, WIFI_PASSWORD, sizeof(wifi_config->sta.password) - 1);
//     wifi_config->sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
//     wifi_config->sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
//     ESP_LOGI(TAG_WIFI, "WiFi configuration created.");
// }

// #endif // WIFI_HELPER_H
