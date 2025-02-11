#include "image.h"

#include "buffer.h"
#include "common.h"
#include "draw.h"

#include <Arduino.h>

size_t data_size = 0;
size_t data_alloc = 0;
uint8_t *image_data = nullptr;

PNG png;
JPEGDEC jpeg;

ImageFormat image_format;

void rgb565_to_buffer(uint8_t *rgb565, uint16_t width, uint16_t height,
                      uint16_t x, uint16_t y) {
  uint16_t in_idx = 0;
  uint8_t red, green, blue;
  bool whitish = false;
  bool colored = false;
  bool with_color = true;
  uint8_t out_byte = 0xFF;       // white (for w%8!=0 border)
  uint8_t out_color_byte = 0xFF; // white (for w%8!=0 border)
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
        mono_buffer[(row + y) * MAX_COL / 8 + out_col_idx + x] = out_byte;
        color_buffer[(row + y) * MAX_COL / 8 + out_col_idx + x] =
            out_color_byte;
        out_col_idx++;
        out_byte = 0xFF;       // white (for w%8!=0 border)
        out_color_byte = 0xFF; // white (for w%8!=0 border)
      }
    }
  }
}

void draw_png(PNGDRAW *pDraw) {
  uint16_t *usPixels = (uint16_t *)malloc(2 * 128);
  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);

  rgb565_to_buffer((uint8_t *)usPixels, pDraw->iWidth, 1, 0, 0);

  draw_write(mono_buffer, color_buffer, pDraw->iWidth, 1, 0, pDraw->y);
}

int draw_jpeg(JPEGDRAW *pDraw) {
  rgb565_to_buffer((uint8_t *)pDraw->pPixels, pDraw->iWidth, pDraw->iHeight, 0,
                   0);

  for (uint16_t i = 0; i < pDraw->iHeight; i++) {
    draw_write(mono_buffer + (i * MAX_COL / 8),
               color_buffer + (i * MAX_COL / 8), pDraw->iWidth, 1, pDraw->x,
               pDraw->y + i);
  }

  return 1;
}

void decode_image() {
  switch (image_format) {
  case PNG_FORMAT:
    if (png.openRAM(image_data, data_size, draw_png) != PNG_SUCCESS) {
      Serial.println("[decode_image] - Error: "
                     "png.openRAM failed");
    }
    if (png.decode(NULL, 0) != PNG_SUCCESS) {
      Serial.println("[decode_image] - Error: "
                     "png.decode failed");
    }
    png.close();

#ifdef DEBUG
    Serial.println("DECODING PNG");
    Serial.printf("Image specs: (%d x %d), %d bpp, pixel type: %d\n",
                  png.getWidth(), png.getHeight(), png.getBpp(),
                  png.getPixelType());
#endif
    break;

  case JPEG_FORMAT:
    if (jpeg.openRAM(image_data, data_size, draw_jpeg) != JPEG_SUCCESS) {
      Serial.println("[decode_image] - Error: "
                     "jpeg.openRAM failed");
    }
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
    if (jpeg.decode(0, 0, 0) != JPEG_SUCCESS) {
      Serial.println("[decode_image] - Error: "
                     "jpeg.decode failed");
    }
    jpeg.close();

#ifdef DEBUG
    Serial.println("DECODING JPEG");
    Serial.printf("Image size: %d x %d, orientation: %d, bpp: %d\n",
                  jpeg.getWidth(), jpeg.getHeight(), jpeg.getOrientation(),
                  jpeg.getBpp());
#endif
    break;

  default:
    break;
  }
}

void reset_image() {
  free(image_data);
  data_size = 0;
  data_alloc = 0;
  image_data = nullptr;
}

void alloc_memory(uint8_t *data, size_t length) {
  if (length + data_size > data_alloc) {
    data_alloc = (length + data_size) * 2;
    image_data = (uint8_t *)realloc(image_data, data_alloc);
  }

  memcpy(image_data + data_size, data, length);
  data_size += length;
}

bool raw_format(ImageFormat format) {
  return format == MONO_BUFFER || format == COLOR_BUFFER;
}
