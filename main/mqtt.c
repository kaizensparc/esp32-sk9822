#include "mqtt.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "mqtt_client.h"
#include "queues.h"

static const char* TAG = "MQTT";
#define MQTT_PAYLOAD_MAX_SIZE_BYTES 256
#define TOPIC_OTA CONFIG_MQTT_PREFIX "/ota"
#define TOPIC_LOGS CONFIG_MQTT_PREFIX "/logs"
#define TOPIC_ANIM CONFIG_MQTT_PREFIX "/animate"
#define TOPIC_BRIG CONFIG_MQTT_PREFIX "/brightness"
#define TOPIC_COLO CONFIG_MQTT_PREFIX "/color"
#define TOPIC_LED_BRIG CONFIG_MQTT_PREFIX "/led/+/brightness"
#define TOPIC_LED_COLO CONFIG_MQTT_PREFIX "/led/+/color"

// MQTT Client
static esp_mqtt_client_handle_t client;

// MQTT event group
EventGroupHandle_t mqtt_event_group;

static void mqtt_subscribe() {
    int msg_id;
    msg_id = esp_mqtt_client_subscribe(client, TOPIC_OTA, 0);
    ESP_LOGI(TAG, "Subscribed to (%s), msg_id=%d", TOPIC_OTA, msg_id);

    msg_id = esp_mqtt_client_subscribe(client, TOPIC_ANIM, 0);
    ESP_LOGI(TAG, "Subscribed to (%s), msg_id=%d", TOPIC_ANIM, msg_id);

    msg_id = esp_mqtt_client_subscribe(client, TOPIC_BRIG, 0);
    ESP_LOGI(TAG, "Subscribed to (%s), msg_id=%d", TOPIC_BRIG, msg_id);

    msg_id = esp_mqtt_client_subscribe(client, TOPIC_COLO, 0);
    ESP_LOGI(TAG, "Subscribed to (%s), msg_id=%d", TOPIC_COLO, msg_id);

    msg_id = esp_mqtt_client_subscribe(client, TOPIC_LED_BRIG, 0);
    ESP_LOGI(TAG, "Subscribed to (%s), msg_id=%d", TOPIC_LED_BRIG, msg_id);

    msg_id = esp_mqtt_client_subscribe(client, TOPIC_LED_COLO, 0);
    ESP_LOGI(TAG, "Subscribed to (%s), msg_id=%d", TOPIC_LED_COLO, msg_id);
}

static void mqtt_parse_payload(esp_mqtt_event_handle_t event) {
    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
    // Sanity check
    if (event->data_len >= MQTT_PAYLOAD_MAX_SIZE_BYTES - 1) {
        ESP_LOGI(TAG, "Payload is larger than buffer!");
        return;
    }

    if (memcmp(event->topic, TOPIC_OTA, event->topic_len) == 0) {
        ESP_LOGI(TAG, "OTA update!");
        if (xQueueSend(dispatcher_queues[QUEUE_OTA], (void*)&event->data,
                       500 / portTICK_PERIOD_MS) != pdTRUE) {
            ESP_LOGI(TAG, "Queue is not available, ignoring message");
        }
    } else if (memcmp(event->topic, TOPIC_ANIM, event->topic_len) == 0) {
        ESP_LOGI(TAG, "Animation call");
        uint8_t payload;
        if (event->data_len >= 3) {
            ESP_LOGI(TAG, "Message is too long for an uint8");
            return;
        }
        // XXX To continue
        // Sanity check, payload should be a single uint8_t
        if (xQueueSend(dispatcher_queues[QUEUE_ANIM], (void*)&event->data,
                       500 / portTICK_PERIOD_MS) != pdTRUE) {
            ESP_LOGI(TAG, "Queue is not available, ignoring message");
        }
    } else if (memcmp(event->topic, TOPIC_BRIG, event->topic_len) == 0) {
        ESP_LOGI(TAG, "Global brightness change");
        // Sanity check, payload should be a single uint8_t
        if (xQueueSend(dispatcher_queues[QUEUE_BRIG], (void*)&event->data,
                       500 / portTICK_PERIOD_MS) != pdTRUE) {
            ESP_LOGI(TAG, "Queue is not available, ignoring message");
        }
    } else if (memcmp(event->topic, TOPIC_COLO, event->topic_len) == 0) {
        ESP_LOGI(TAG, "GLobal color change");
        // Sanity check, payload should be a string like RRGGBB
        if (xQueueSend(dispatcher_queues[QUEUE_COLO], (void*)&event->data,
                       500 / portTICK_PERIOD_MS) != pdTRUE) {
            ESP_LOGI(TAG, "Queue is not available, ignoring message");
        }
    } else if (memcmp(event->topic, TOPIC_LED_BRIG, event->topic_len) == 0) {
        ESP_LOGI(TAG, "Led-specific brightness change");
        // Sanity check, payload should be a single uint8_t
        if (xQueueSend(dispatcher_queues[QUEUE_LED_BRIG], (void*)&event->data,
                       500 / portTICK_PERIOD_MS) != pdTRUE) {
            ESP_LOGI(TAG, "Queue is not available, ignoring message");
        }
    } else if (memcmp(event->topic, TOPIC_LED_COLO, event->topic_len) == 0) {
        ESP_LOGI(TAG, "Led-specific color change");
        // Sanity check, payload should be a string like RRGGBB
        if (xQueueSend(dispatcher_queues[QUEUE_LED_COLO], (void*)&event->data,
                       500 / portTICK_PERIOD_MS) != pdTRUE) {
            ESP_LOGI(TAG, "Queue is not available, ignoring message");
        }
    } else {
        ESP_LOGE(TAG, "Error, unhandled message from topic \"%s\"",
                 event->topic);
    }
}

static int mqtt_vprintf(const char* fmt, va_list ap) {
    // Get the formatted string
    char buf[256];
    int bufsz = vsprintf(buf, fmt, ap);
    esp_mqtt_client_publish(client, CONFIG_MQTT_PREFIX "logs", buf, bufsz, 1,
                            0);
    return bufsz;
}

// MQTT event handler
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
    switch (event->event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "Event before connecting");
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected");

            mqtt_subscribe();

            xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_BIT);

            ESP_LOGI(
                TAG,
                "Connected to MQTT broker, redirecting logs to topic %s. Bye!",
                TOPIC_LOGS);
            esp_log_set_vprintf(mqtt_vprintf);
            ESP_LOGI(TAG,
                     "Connected to MQTT broker, logs redirected to topic %s",
                     TOPIC_LOGS);

            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected");
            xEventGroupClearBits(mqtt_event_group, MQTT_CONNECTED_BIT);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGD(TAG, "Subscribed, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGD(TAG, "Unsubscribed, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "Published, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGD(TAG, "Data incoming");
            mqtt_parse_payload(event);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "Event error");
            break;
    }
    return ESP_OK;
}

void mqtt_init() {
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_MQTT_URL,
        .client_id = CONFIG_MQTT_CLIENT_ID,
        .username = CONFIG_MQTT_CLIENT_ID,
        .password = CONFIG_MQTT_CLIENT_ID,
        .event_handle = mqtt_event_handler};

    mqtt_event_group = xEventGroupCreate();

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}
