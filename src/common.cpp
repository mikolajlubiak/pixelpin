#include "common.h"

#include <stdlib.h>

uint8_t *mono_buffer;
uint8_t *color_buffer;

uint64_t timer;

uint32_t clamp(uint32_t val, uint32_t min, uint32_t max) {
  if (val < min) {
    return min;
  } else if (val > max) {
    return max;
  } else {
    return val;
  }
}

uint64_t uint8_to_uint64(const uint8_t *buffer) {
  return ((uint64_t)buffer[0] << 0) | ((uint64_t)buffer[1] << 8) |
         ((uint64_t)buffer[2] << 16) | ((uint64_t)buffer[3] << 24) |
         ((uint64_t)buffer[4] << 32) | ((uint64_t)buffer[5] << 40) |
         ((uint64_t)buffer[6] << 48) | ((uint64_t)buffer[7] << 56);
}

void common_init() {
  common_clean();

  mono_buffer = (uint8_t *)malloc(BUFFER_SIZE);
  color_buffer = (uint8_t *)malloc(BUFFER_SIZE);
  
  timer = 0;
}

void common_clean() {
  if (mono_buffer) {
    free(mono_buffer);
  }

  if (color_buffer) {
    free(color_buffer);
  }

  mono_buffer = nullptr;
  color_buffer = nullptr;

  timer = 0;
}
