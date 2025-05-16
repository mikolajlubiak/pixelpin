#ifdef TFT

#pragma once

#include "tft/tft_select.h"

#include <stddef.h>
#include <stdint.h>

#define MAX_ROW 240
#define MAX_COL 320
#define BUFFER_SIZE (MAX_ROW * MAX_COL * 2) // 2 bytes per pixel

extern uint8_t *tft_buffer;

extern size_t tft_buffer_size;

void tft_init();

void tft_clean();

void tft_write(uint8_t *buffer, uint16_t width, uint16_t height, uint16_t x,
               uint16_t y);

void tft_clear();

void tft_refresh();

#endif