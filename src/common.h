#pragma once

#include <stdint.h>

#define MAX_ROW 296
#define MAX_COL 128
#define BUFFER_SIZE (MAX_ROW * MAX_COL / 8) // 1 bit per pixel

extern uint8_t *mono_buffer;
extern uint8_t *color_buffer;

extern uint64_t timer;

uint32_t clamp(uint32_t val, uint32_t min, uint32_t max);

uint64_t uint8_to_uint64(const uint8_t *buffer);
