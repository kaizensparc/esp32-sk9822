#pragma once

#include "apa102.h"

enum led_animation_name {
    ANIMATE_1,
    ANIMATE_2,
    ANIMATE_3,
    ANIMATE_4,
};

#define LED_ANIMATION_NAME_LENGTH (ANIMATE_4 + 1)

void led_init(apa102_spi_device_t *device);
