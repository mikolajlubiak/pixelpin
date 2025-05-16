#ifdef TFT

#include "tft.h"

#include <TFT_eSPI.h>

#include "tft/tft_select.h"

TFT_eSPI tft = TFT_eSPI();

uint8_t *tft_buffer;

size_t tft_buffer_size;

void tft_init() {
  tft_clean();

  tft.begin();
  tft.fillScreen(TFT_BLUE);

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
  tft.startWrite();
  tft.pushImage(x, y, height, width, buffer);
}

void tft_clear() { tft.fillScreen(TFT_BLACK); }

void tft_refresh() { tft.endWrite(); }

#endif
