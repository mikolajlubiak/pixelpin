#pragma once

#include <stdint.h>

#ifdef ARDUINO_ESP32C3_DEV
extern int64_t timer;
#endif

uint32_t clamp(uint32_t val, uint32_t min, uint32_t max);

uint64_t uint8_to_uint64(const uint8_t *buffer);

void common_init();

void common_clean();
