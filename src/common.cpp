#include "common.h"

uint32_t clamp(uint32_t val, uint32_t min, uint32_t max) {
  if (val < min) {
    return min;
  } else if (val > max) {
    return max;
  } else {
    return val;
  }
}