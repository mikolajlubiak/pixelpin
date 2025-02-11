#pragma once

#include <stddef.h>
#include <stdint.h>

#include <JPEGDEC.h>
#include <PNGdec.h>

enum ImageFormat {
  PNG_FORMAT,
  JPEG_FORMAT,
  BMP_FORMAT,
  MONO_BUFFER,
  COLOR_BUFFER,
};

extern ImageFormat image_format;

bool raw_format(ImageFormat format);

void rgb565_to_buffer(uint8_t *rgb565, uint16_t width, uint16_t height,
                      uint16_t x = 0, uint16_t y = 0);

void draw_png(PNGDRAW *pDraw);

int draw_jpeg(JPEGDRAW *pDraw);

void decode_image();

void reset_image();

void alloc_memory(uint8_t *data, size_t length);
