#include "draw.h"

#include "common.h"
#include "epaper.h"
#include "wifi.h"

#include <Arduino.h>

void draw_http(const char *host, const char *path, const char *filename,
               uint16_t port) {
  uint8_t *mono;
  uint8_t *color;
  uint16_t width;
  uint16_t height;
  bool flip = true;

  downloadBitmapFrom_HTTP(host, path, filename, port, true, (uint8_t **)&mono,
                          (uint8_t **)&color, &width, &height);

  draw(mono, color, width, height);
}

void draw_https(const char *host, const char *path, const char *filename,
                const char *fingerprint, const char *certificate) {
  uint8_t *mono;
  uint8_t *color;
  uint16_t width;
  uint16_t height;
  bool flip = true;

  downloadBitmapFrom_HTTPS(host, path, filename, fingerprint, true, certificate,
                           (uint8_t **)&mono, (uint8_t **)&color, &width,
                           &height);

  draw(mono, color, width, height);
}

void draw(uint8_t *mono, uint8_t *color, uint16_t width, uint16_t height) {
  bool flip = true;

  epaper_clear();
  for (uint16_t i = 0; i < height; i++) {
    uint16_t yrow = flip ? height - i - 1 : i;
    epaper_write(mono + (i * MAX_COL / 8), color + (i * MAX_COL / 8), width, 1,
                 0, yrow);
  }
  epaper_refresh();
}

void draw_bitmap(uint8_t *bitmap, uint16_t width, uint16_t height) {
  bool flip = true;

  epaper_clear();
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
