#pragma once

#include "stdint.h"

void epaper_init();

void epaper_draw(uint8_t *mono, uint8_t *color, uint16_t width, uint16_t height,
                 uint16_t x, uint16_t y);

void epaper_write(uint8_t *mono, uint8_t *color, uint16_t width,
                  uint16_t height, uint16_t x, uint16_t y);

void epaper_clear();

void epaper_refresh();