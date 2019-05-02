#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

void wifi_init(void);

extern EventGroupHandle_t wifi_event_group;
#define WIFI_STARTED_BIT BIT0
#define WIFI_CONNECTED_BIT BIT1
#define WIFI_ESPTOUCH_DONE_BIT BIT2
