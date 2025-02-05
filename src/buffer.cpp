#include "buffer.h"

uint8_t output_mono_buffer[MAX_ROW][MAX_COL / 8];
uint8_t output_color_buffer[MAX_ROW][MAX_COL / 8];

size_t mono_buffer_size = 0;
size_t color_buffer_size = 0;
