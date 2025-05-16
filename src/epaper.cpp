#ifdef EPD

#include "epaper.h"

#include "gxepd/gxepd_select.h"

BufferType buffer_type;

uint8_t *mono_buffer;
uint8_t *color_buffer;

size_t mono_buffer_size;
size_t color_buffer_size;

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

void epaper_init() {
  epaper_clean();

  display.init(115200, true, 2, false);

  mono_buffer = (uint8_t *)malloc(BUFFER_SIZE);
  color_buffer = (uint8_t *)malloc(BUFFER_SIZE);
}

void epaper_clean() {
  if (mono_buffer) {
    free(mono_buffer);
  }

  if (color_buffer) {
    free(color_buffer);
  }

  mono_buffer = nullptr;
  color_buffer = nullptr;
}

void epaper_write(uint8_t *mono, uint8_t *color, uint16_t width,
                  uint16_t height, uint16_t x, uint16_t y) {
  display.writeImage(mono, color, x, y, width, height);
}

void epaper_clear() { display.clearScreen(); }

void epaper_refresh() { display.refresh(); }

#endif
