#include "queues.h"

// Queues used for the inter-thread communication
QueueHandle_t dispatcher_queues[QUEUE_INDEX_LENGTH] = {0};

static StaticQueue_t queues_struct[QUEUE_INDEX_LENGTH];

#define create_static_queue(queue_idx, name, elt_size)        \
    static uint8_t uc_storage_area_##name[elt_size];          \
    dispatcher_queues[queue_idx] =                            \
        xQueueCreateStatic(1, sizeof(uc_storage_area_##name), \
                           uc_storage_area_##name, &queues_struct[queue_idx])

void queues_init(void) {
    create_static_queue(QUEUE_OTA, ota, 1024);
    create_static_queue(QUEUE_ANIM, anim, 1);
    create_static_queue(QUEUE_BRIG, brig, 1);
    create_static_queue(QUEUE_COLO, colo, 6);
    create_static_queue(QUEUE_LED_BRIG, led_brig, 1);
    create_static_queue(QUEUE_LED_COLO, led_colo, 6);
}
