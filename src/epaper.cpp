#include "epaper.h"

#include "gxepd/gxepd_select.h"

void epaper_init() { display.init(115200, true, 2, false); }

void epaper_draw(uint8_t *mono, uint8_t *color, uint16_t width, uint16_t height,
                 uint16_t x, uint16_t y) {
  display.clearScreen();
  display.writeImage(mono, color, x, y, width, height);
  display.refresh();
}

void epaper_write(uint8_t *mono, uint8_t *color, uint16_t width,
                  uint16_t height, uint16_t x, uint16_t y) {
  display.writeImage(mono, color, x, y, width, height);
}

void epaper_clear() { display.clearScreen(); }

void epaper_refresh() { display.refresh(); }

void epaper_write_bitmap(uint8_t *bitmap, uint16_t width, uint16_t height,
                         uint16_t x, uint16_t y) {
  display.writeImage(bitmap, x, y, width, height);
}
