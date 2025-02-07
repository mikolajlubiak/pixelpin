#include "buffer.h"

#include "stdlib.h"

uint8_t *mono_buffer = (uint8_t *)malloc(BUFFER_SIZE);
uint8_t *color_buffer = (uint8_t *)malloc(BUFFER_SIZE);
