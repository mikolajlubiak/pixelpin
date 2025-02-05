#pragma once

#include <stdint.h>

void draw_http(const char *host, const char *path, const char *filename,
               uint16_t port = 80);

void draw_https(const char *host, const char *path, const char *filename,
                const char *fingerprint, const char *certificate);

void draw(uint8_t *mono, uint8_t *color, uint16_t width, uint16_t height);

void draw_write(uint8_t *mono, uint8_t *color, uint16_t width, uint16_t height,
                uint16_t x, uint16_t y);

void draw_bitmap(uint8_t *bitmap, uint16_t width, uint16_t height);

void draw_clear();

void draw_refresh();
