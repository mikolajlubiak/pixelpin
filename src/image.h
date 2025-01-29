#pragma once

#include <stdint.h>

void rgb565_to_buffer(uint8_t *rgb565, uint16_t width, uint16_t height,
                      uint16_t x = 0, uint16_t y = 0);
