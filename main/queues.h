#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

enum queue_index {
    QUEUE_OTA,
    QUEUE_ANIM,
    QUEUE_BRIG,
    QUEUE_COLO,
    QUEUE_LED_BRIG,
    QUEUE_LED_COLO,
};
#define QUEUE_INDEX_LENGTH (QUEUE_LED_COLO + 1)

extern QueueHandle_t dispatcher_queues[QUEUE_INDEX_LENGTH];

void queues_init(void);
