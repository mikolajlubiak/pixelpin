#include "draw.h"

#include "common.h"

#ifdef EPD
#include "epaper.h"
#endif

#ifdef TFT
#include "tft.h"
#endif

#include <Arduino.h>

void draw_init() {
#ifdef EPD
  epaper_init();
#elif TFT
  tft_init();
#endif
}

#ifdef EPD
void draw_write(uint8_t *mono, uint8_t *color, uint16_t width, uint16_t height,
                uint16_t x, uint16_t y) {

  epaper_write(mono, color, width, height, x, y);
}
#endif

#ifdef TFT
void draw_write(uint8_t *buffer, uint16_t width, uint16_t height, uint16_t x,
                uint16_t y) {
  tft_write(buffer, width, height, x, y);
}
#endif

void draw_clear() {
#ifdef EPD
  epaper_clear();
#elif TFT
  tft_clear();
#endif
}

void draw_refresh() {
#ifdef EPD
  epaper_refresh();
#elif TFT
  tft_refresh();
#endif
}
