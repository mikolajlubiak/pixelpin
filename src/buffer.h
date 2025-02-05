#pragma once

#include "common.h"
#include <stdint.h>
#include <stddef.h>

extern uint8_t output_mono_buffer[][MAX_COL / 8];
extern uint8_t output_color_buffer[][MAX_COL / 8];

extern size_t mono_buffer_size;
extern size_t color_buffer_size;
