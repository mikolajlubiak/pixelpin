#include "draw.h"

#include "common.h"
#include "epaper.h"

#include <Arduino.h>

void draw(uint8_t *mono, uint8_t *color, uint16_t width, uint16_t height) {
  constexpr bool flip = true;

  epaper_clear();

  // Write the buffers row by row
  for (uint16_t i = 0; i < height; i++) {
    uint16_t yrow = flip ? height - i - 1 : i;
    epaper_write(mono + (i * MAX_COL / 8), color + (i * MAX_COL / 8), width, 1,
                 0, yrow);
  }

  epaper_refresh();
}

void draw_bitmap(uint8_t *bitmap, uint16_t width, uint16_t height) {
  constexpr bool flip = true;

  epaper_clear();

  // Write the buffers row by row
  for (uint16_t i = 0; i < height; i++) {
    uint16_t yrow = flip ? height - i - 1 : i;
    epaper_write_bitmap(bitmap + (i * MAX_COL / 8), width, 1, 0, yrow);
  }

  epaper_refresh();
}

void draw_write(uint8_t *mono, uint8_t *color, uint16_t width, uint16_t height,
                uint16_t x, uint16_t y) {
  epaper_write(mono, color, width, height, x, y);
}

void draw_clear() { epaper_clear(); }

void draw_refresh() { epaper_refresh(); }
