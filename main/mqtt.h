#pragma once

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

void mqtt_init(void);

extern EventGroupHandle_t mqtt_event_group;
#define MQTT_CONNECTED_BIT BIT0
#define MQTT_OTA_BIT BIT1
