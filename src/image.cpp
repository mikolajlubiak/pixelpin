#include "image.h"

#include "buffer.h"
#include "common.h"

#include <Arduino.h>

void rgb565_to_buffer(uint8_t *rgb565, uint16_t width, uint16_t height,
                      uint16_t x, uint16_t y) {
#ifdef DEBUG
  static uint16_t height_times = 0;
  height_times += height;
  Serial.printf("H: %u, W: %u\n", height_times, width);
#endif

  for (uint16_t row = 0; row < height; row++) {
    uint16_t in_idx = 0;
    uint16_t red, green, blue;
    bool whitish = false;
    bool colored = false;
    bool with_color = true;
    uint8_t out_byte = 0xFF;       // white (for w%8!=0 border)
    uint8_t out_color_byte = 0xFF; // white (for w%8!=0 border)
    uint16_t out_col_idx = 0;
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

      if (whitish) {
        // keep white
      } else if (colored && with_color) {
        out_color_byte &= ~(0x80 >> col % 8); // colored
      } else {
        out_byte &= ~(0x80 >> col % 8); // black
      }
      if ((7 == col % 8) ||
          (col == width - 1)) // write that last byte! (for w%8!=0 border)
      {
        output_color_buffer[row + y][out_col_idx + x] = out_color_byte;
        output_mono_buffer[row + y][x + out_col_idx++] = out_byte;
        out_byte = 0xFF;       // white (for w%8!=0 border)
        out_color_byte = 0xFF; // white (for w%8!=0 border)
      }
    }
  }
}
