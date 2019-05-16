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

#define QUEUE_SIZE_OTA 1024
#define QUEUE_SIZE_ANIM 1
#define QUEUE_SIZE_BRIG 1
#define QUEUE_SIZE_COLO 6
#define QUEUE_SIZE_LED_BRIG 1
#define QUEUE_SIZE_LED_COLO 6

extern QueueHandle_t dispatcher_queues[QUEUE_INDEX_LENGTH];

void queues_init(void);
