#pragma once

#include <stdint.h>

// for up to 7.8" display 1872x1404
// #define MAX_ROW 1872
// #define MAX_COL 1404
#define MAX_ROW 296
#define MAX_COL 128
#define BUFFER_SIZE (MAX_ROW * MAX_COL / 8)

uint32_t clamp(uint32_t val, uint32_t min, uint32_t max);
