#include "common.h"

#include <stdlib.h>

#ifdef ARDUINO_ESP32C3_DEV
#include <esp_timer.h>
#endif

#ifdef ARDUINO_ESP32C3_DEV
int64_t timer;
#endif

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

#ifdef ARDUINO_ESP32C3_DEV
  timer = esp_timer_get_time();
#endif
}

void common_clean() {
#ifdef ARDUINO_ESP32C3_DEV
  timer = 0;
#endif
}
