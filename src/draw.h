#pragma once

#include <stdint.h>

void draw(uint8_t *mono, uint8_t *color, uint16_t width, uint16_t height);

void draw_write(uint8_t *mono, uint8_t *color, uint16_t width, uint16_t height,
                uint16_t x, uint16_t y);

void draw_bitmap(uint8_t *bitmap, uint16_t width, uint16_t height);

void draw_clear();

void draw_refresh();
