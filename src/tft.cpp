#ifdef TFT

#include "tft.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define TFT_CS 10
#define TFT_DC 8
#define TFT_RST -1

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

uint8_t *tft_buffer;

size_t tft_buffer_size;

void tft_init() {
  tft_clean();

  tft.init(240, 320);
  tft.fillScreen(ST77XX_BLACK);

  tft_buffer = (uint8_t *)malloc(BUFFER_SIZE);
}

void tft_clean() {
  if (tft_buffer) {
    free(tft_buffer);
  }

  tft_buffer = nullptr;
}

void tft_write(uint8_t *buffer, uint16_t width, uint16_t height, uint16_t x,
               uint16_t y) {
  tft.drawRGBBitmap(x, y, (uint16_t *)buffer, width, height);
}

void tft_clear() { tft.fillScreen(ST77XX_BLACK); }

void tft_refresh() {}

#endif
