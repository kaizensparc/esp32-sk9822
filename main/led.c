#include "led.h"

// FreeRTOS includes
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "apa102.h"
#include "mqtt.h"

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

// Simple routine to generate some patterns and send them to the LED Strip.
#define STACK_SIZE 4096
StaticTask_t animate_task_buffer;
StackType_t animate_task_stack[STACK_SIZE];
static void animate_task(void* pvParameter) {
    while (1) {
        animate_1();
    }
}

void led_init(apa102_spi_device_t* device) {
    apa102_init(device);

    xTaskCreateStatic(&animate_task, "animate_task", STACK_SIZE, NULL,
                      tskIDLE_PRIORITY + 1, animate_task_stack,
                      &animate_task_buffer);
}
