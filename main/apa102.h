#pragma once

typedef struct {
    int clock_speed_hz;
    int mosi;
    int clk;
    int cs;
} apa102_spi_device_t;

void apa102_init(apa102_spi_device_t *device);
void apa102_set_pixel(int index, int brightness, int red, int green, int blue);
void apa102_flush();
