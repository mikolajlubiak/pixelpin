#pragma once

#include "common.h"

#include <stdint.h>

void wifi_init();

void downloadBitmapFrom_HTTP(const char *host, const char *path,
                             const char *filename, uint16_t port,
                             bool with_color, uint8_t **out_mono,
                             uint8_t **out_color, uint16_t *out_width,
                             uint16_t *out_height);
void downloadBitmapFrom_HTTPS(const char *host, const char *path,
                              const char *filename, const char *fingerprint,
                              bool with_color, const char *certificate,
                              uint8_t **out_mono, uint8_t **out_color,
                              uint16_t *out_width, uint16_t *out_height);

void showBitmapFrom_HTTP_Buffered(const char *host, const char *path,
                                  const char *filename, int16_t x, int16_t y,
                                  bool with_color);
void showBitmapFrom_HTTPS_Buffered(const char *host, const char *path,
                                   const char *filename,
                                   const char *fingerprint, int16_t x,
                                   int16_t y, bool with_color,
                                   const char *certificate);

void drawBitmapFrom_HTTP_ToBuffer(const char *host, const char *path,
                                  const char *filename, int16_t x, int16_t y,
                                  bool with_color);
void drawBitmapFrom_HTTPS_ToBuffer(const char *host, const char *path,
                                   const char *filename,
                                   const char *fingerprint, int16_t x,
                                   int16_t y, bool with_color,
                                   const char *certificate);
