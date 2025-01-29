#include "image.h"

#include "buffer.h"
#include "common.h"

void rgb565_to_buffer(uint8_t *rgb565, uint16_t width, uint16_t height,
                      uint16_t x, uint16_t y) {
  uint16_t out_row_idx = 0;
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

      whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80))
                           : ((red + green + blue) > 3 * 0x80); // whitish
      colored = (red > 0xF0) ||
                ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
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
        output_color_buffer[out_row_idx + y][out_col_idx + x] = out_color_byte;
        output_mono_buffer[out_row_idx + y][x + out_col_idx++] = out_byte;
        out_byte = 0xFF;       // white (for w%8!=0 border)
        out_color_byte = 0xFF; // white (for w%8!=0 border)
      }
    }
    out_row_idx++;
  }
}
