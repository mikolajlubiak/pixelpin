#pragma once

#include <stdint.h>

void draw_init();

#ifdef EPD
void draw_write(uint8_t *mono, uint8_t *color, uint16_t width, uint16_t height,
                uint16_t x, uint16_t y);
#endif

#ifdef TFT
void draw_write(uint8_t *buffer, uint16_t width, uint16_t height, uint16_t x,
                uint16_t y);
#endif

void draw_clear();

void draw_refresh();
