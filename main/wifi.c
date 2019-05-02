#include "wifi.h"

#include <string.h>

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

// ESP specific includes
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"

// WiFi
EventGroupHandle_t wifi_event_group;

static void sc_callback(smartconfig_status_t status, void *pdata) {
    switch (status) {
        case SC_STATUS_WAIT:
            ESP_LOGI("WIFI", "SC_STATUS_WAIT");
            break;
        case SC_STATUS_FIND_CHANNEL:
            ESP_LOGI("WIFI", "SC_STATUS_FINDING_CHANNEL");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            ESP_LOGI("WIFI", "SC_STATUS_GETTING_SSID_PSWD");
            break;
        case SC_STATUS_LINK:
            ESP_LOGI("WIFI", "SC_STATUS_LINK");
            wifi_config_t *wifi_config = pdata;
            ESP_LOGI("WIFI", "SSID:%s", wifi_config->sta.ssid);
            ESP_LOGI("WIFI", "PASSWORD:%s", wifi_config->sta.password);
            ESP_ERROR_CHECK(esp_wifi_disconnect());
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config));
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        case SC_STATUS_LINK_OVER:
            ESP_LOGI("WIFI", "SC_STATUS_LINK_OVER");
            if (pdata != NULL) {
                uint8_t phone_ip[4] = {0};
                memcpy(phone_ip, (uint8_t *)pdata, 4);
                ESP_LOGI("WIFI", "Phone ip: %d.%d.%d.%d\n", phone_ip[0],
                         phone_ip[1], phone_ip[2], phone_ip[3]);
            }
            xEventGroupSetBits(wifi_event_group, WIFI_ESPTOUCH_DONE_BIT);
            break;
        default:
            break;
    }
}

void smartconfig_task(void *pvParameter) {
    EventBits_t uxBits;
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
    ESP_ERROR_CHECK(esp_smartconfig_start(sc_callback));
    while (1) {
        uxBits = xEventGroupWaitBits(
            wifi_event_group, WIFI_CONNECTED_BIT | WIFI_ESPTOUCH_DONE_BIT, true,
            false, portMAX_DELAY);
        if (uxBits & WIFI_CONNECTED_BIT) {
            ESP_LOGI("WIFI", "WiFi Connected to ap");
        }
        if (uxBits & WIFI_ESPTOUCH_DONE_BIT) {
            ESP_LOGI("WIFI", "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}

// Permanently try to connect to WiFi when connection is lost
// Never surrender!
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3,
                        NULL);
            xEventGroupSetBits(wifi_event_group, WIFI_STARTED_BIT);
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void wifi_init(void) {
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}
