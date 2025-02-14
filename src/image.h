#pragma once

#include <stddef.h>
#include <stdint.h>

enum BufferType {
  MONO_BUFFER,
  COLOR_BUFFER,
};

extern BufferType buffer_type;

void rgb565_to_buffer(uint8_t *rgb565, uint16_t width, uint16_t height,
                      uint16_t x = 0, uint16_t y = 0);

void decode_image();

void reset_image();

void alloc_memory(uint8_t *data, size_t length);
