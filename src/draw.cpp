#include "draw.h"

#include "common.h"
#include "epaper.h"
#include "wifi.h"

#include "Arduino.h"

void draw_http(const char *host, const char *path, const char *filename,
               uint16_t port) {
  uint8_t mono[MAX_ROW / 8][MAX_COL / 8];
  uint8_t color[MAX_ROW / 8][MAX_COL / 8];
  uint16_t width;
  uint16_t height;

  downloadBitmapFrom_HTTP(host, path, filename, port, true, (uint8_t **)&mono,
                          (uint8_t **)&color, &width, &height);

  epaper_clear();
  for (uint16_t i = 0; i < height; i++) {
    epaper_write(mono[i], color[i], width, 1, 0, i);
  }
  epaper_refresh();
}

void draw_https(const char *host, const char *path, const char *filename,
                const char *fingerprint, const char *certificate) {
  uint8_t mono[MAX_ROW / 8][MAX_COL / 8];
  uint8_t color[MAX_ROW / 8][MAX_COL / 8];
  uint16_t width;
  uint16_t height;

  downloadBitmapFrom_HTTPS(host, path, filename, fingerprint, true, certificate,
                           (uint8_t **)&mono, (uint8_t **)&color, &width,
                           &height);

  epaper_clear();
  for (uint16_t i = 0; i < height; i++) {
    epaper_write(mono[i], color[i], width, 1, 0, i);
  }
  epaper_refresh();
}