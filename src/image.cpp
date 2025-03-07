#include "image.h"

#include "common.h"
#include "draw.h"

#include <Arduino.h>

size_t data_size;
size_t data_alloc;
uint8_t *image_data;

BufferType buffer_type;

// Convert RGB565 pixel data and write it to buffers
void rgb565_to_buffer(uint8_t *rgb565, uint16_t width, uint16_t height,
                      uint16_t x, uint16_t y) {
  constexpr bool with_color = true;

  uint16_t in_idx = 0;
  uint8_t red, green, blue;
  bool whitish = false;
  bool colored = false;
  uint8_t out_byte = 0xFF;       // white
  uint8_t out_color_byte = 0xFF; // no color
  uint16_t out_col_idx = 0;

  for (uint16_t row = 0; row < height; row++) {
    out_col_idx = 0;
    for (uint16_t col = 0; col < width; col++) {
      uint8_t lsb = ((uint8_t *)rgb565)[row * width + in_idx++];
      uint8_t msb = ((uint8_t *)rgb565)[row * width + in_idx++];
      blue = (lsb & 0x1F) << 3;
      green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
      red = (msb & 0xF8);

      whitish =
          (red * 0.299f + green * 0.587f + blue * 0.114f) > 0x80; // whitish
      colored =
          ((red > 0x80) && (((red > green + 0x40) && (red > blue + 0x40)) ||
                            (red + 0x10 > green + blue))) ||
          (green > 0xC8 && red > 0xC8 && blue < 0x40); // reddish or yellowish?

      // 1 bit per pixel
      // Set the bit to 0 if colored or black
      if (whitish) {
        // keep white
      } else if (colored && with_color) {
        out_color_byte &= ~(0x80 >> col % 8); // colored
      } else {
        out_byte &= ~(0x80 >> col % 8); // black
      }

      // Write to buffers only after the whole byte is filled
      if ((7 == col % 8) ||
          (col == width - 1)) // write that last byte! (for w%8!=0 border)
      {
        mono_buffer[(row + y) * MAX_COL / 8 + out_col_idx + x] = out_byte;
        color_buffer[(row + y) * MAX_COL / 8 + out_col_idx + x] =
            out_color_byte;
        out_col_idx++;
        out_byte = 0xFF;       // reset color to white
        out_color_byte = 0xFF; // reset to no color
      }
    }
  }
}

void image_clean() {
  if (image_data) {
    free(image_data);
  }
  image_data = nullptr;

  data_size = 0;
  data_alloc = 0;
  buffer_type = NONE;
}

void alloc_memory(uint8_t *data, size_t length) {
  if (length + data_size > data_alloc) {
    data_alloc = (length + data_size) * 2;
    image_data = (uint8_t *)realloc(image_data, data_alloc);
  }

  memcpy(image_data + data_size, data, length);
  data_size += length;
}
