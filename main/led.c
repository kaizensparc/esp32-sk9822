#include "led.h"

// FreeRTOS includes
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mqtt.h"
#include "queues.h"

static const char* TAG = "LED";

static void animate_4() {
    for (int i = 0; i < 1; i++) {
        for (int led = 0; led < CONFIG_LEDS_COUNT; led++)
            apa102_set_pixel(led, 21, 255, 255, 255);
        apa102_flush();
    }
    for (int i = 0; i < 1000; i++) {
        for (int led = 0; led < CONFIG_LEDS_COUNT; led++)
            apa102_set_pixel(led, 0, 255, 255, 255);
        apa102_flush();
    }
}

static void animate_3() {
    for (int i = 0; i < 21; i++) {
        for (int led = 0; led < CONFIG_LEDS_COUNT; led++)
            apa102_set_pixel(led, i, 255, 255, 255);
        apa102_flush();
    }
    for (int i = 21; i > 0; i--) {
        for (int led = 0; led < CONFIG_LEDS_COUNT; led++)
            apa102_set_pixel(led, i, 255, 255, 255);
        apa102_flush();
    }
}

static void animate_2() {
    for (int i = 0; i < CONFIG_LEDS_COUNT; i++) {
        if (i != 0) apa102_set_pixel(i - 1, 2, 0, 0, 0);
        apa102_set_pixel(i, 2, 255, 255, 255);
        apa102_flush();
    }
    for (int i = CONFIG_LEDS_COUNT - 1; i > 0; i--) {
        apa102_set_pixel(i, 2, 0, 0, 0);
        if (i != 0) apa102_set_pixel(i - 1, 2, 255, 255, 255);
        apa102_flush();
    }
}

static void animate_1() {
    int bn = 31;
    for (int r = 0; r < 255; r++) {
        for (int led = 0; led < CONFIG_LEDS_COUNT; led++)
            apa102_set_pixel(led, bn, r, 0, 0);
        apa102_flush();
    }
    for (int r = 255; r >= 0; r--) {
        for (int led = 0; led < CONFIG_LEDS_COUNT; led++)
            apa102_set_pixel(led, bn, r, 0, 0);
        apa102_flush();
    }
    for (int r = 0; r < 255; r++) {
        for (int led = 0; led < CONFIG_LEDS_COUNT; led++)
            apa102_set_pixel(led, bn, 0, r, 0);
        apa102_flush();
    }
    for (int r = 255; r >= 0; r--) {
        for (int led = 0; led < CONFIG_LEDS_COUNT; led++)
            apa102_set_pixel(led, bn, 0, r, 0);
        apa102_flush();
    }
    for (int r = 0; r < 255; r++) {
        for (int led = 0; led < CONFIG_LEDS_COUNT; led++)
            apa102_set_pixel(led, bn, 0, 0, r);
        apa102_flush();
    }
    for (int r = 255; r >= 0; r--) {
        for (int led = 0; led < CONFIG_LEDS_COUNT; led++)
            apa102_set_pixel(led, bn, 0, 0, r);
        apa102_flush();
    }
}

static void (*animate[])(void) = {animate_1, animate_2, animate_3, animate_4};

static enum led_animation_name current_animation = ANIMATE_1;

static uint8_t anim_idx = 0;
// Simple routine to change animation index
#define ANIMATION_CHANGER_STACK_SIZE 2048
StaticTask_t animation_changer_buffer;
StackType_t animation_changer_stack[ANIMATION_CHANGER_STACK_SIZE];
static void animation_changer(void* pvParameter) {
    while (1) {
        if (xQueueReceive(dispatcher_queues[QUEUE_ANIM], (void*)&anim_idx,
                          portMAX_DELAY) != pdTRUE) {
            ESP_LOGI(TAG, "Queue is not available, ignoring message");
            continue;
        }
        if (anim_idx >= LED_ANIMATION_NAME_LENGTH) {
            continue;
        }
        ESP_LOGI(TAG, "Switching to animation %d", anim_idx);
        current_animation = anim_idx;
    }
}

// Simple routine to generate some patterns and send them to the LED Strip.
#define ANIMATE_TASK_STACK_SIZE 4096
StaticTask_t animate_task_buffer;
StackType_t animate_task_stack[ANIMATE_TASK_STACK_SIZE];
static void animate_task(void* pvParameter) {
    while (1) {
        (*animate[current_animation])();
    }
}

void led_init(apa102_spi_device_t* device) {
    apa102_init(device);

    xTaskCreateStatic(&animate_task, "animate_task", ANIMATE_TASK_STACK_SIZE,
                      NULL, tskIDLE_PRIORITY + 1, animate_task_stack,
                      &animate_task_buffer);

    xTaskCreateStatic(&animation_changer, "animation_changer",
                      ANIMATION_CHANGER_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1,
                      animation_changer_stack, &animation_changer_buffer);
}
