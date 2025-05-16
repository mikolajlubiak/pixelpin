#ifdef EPD

#pragma once

#include <stddef.h>
#include <stdint.h>

#define MAX_ROW 296
#define MAX_COL 128
#define BUFFER_SIZE (MAX_ROW * MAX_COL / 8) // 1 bit per pixel

enum BufferType {
  MONO_BUFFER,
  COLOR_BUFFER,
  NONE,
};

extern BufferType buffer_type;

extern uint8_t *mono_buffer;
extern uint8_t *color_buffer;

extern size_t mono_buffer_size;
extern size_t color_buffer_size;

void rgb565_to_buffer(uint8_t *rgb565, uint16_t width, uint16_t height,
                      uint16_t x = 0, uint16_t y = 0);

void epaper_init();

void epaper_clean();

void epaper_write(uint8_t *mono, uint8_t *color, uint16_t width,
                  uint16_t height, uint16_t x, uint16_t y);

void epaper_clear();

void epaper_refresh();

#endif