#include "ble.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include <Arduino.h>

#include <JPEGDEC.h>
#include <PNGdec.h>

#include "buffer.h"
#include "draw.h"
#include "epaper.h"
#include "image.h"

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

enum ImageFormat {
  PNG_FORMAT,
  JPEG_FORMAT,
  BMP_FORMAT,
  MONO_BUFFER,
  COLOR_BUFFER,
};

bool raw_format(ImageFormat format) {
  return format == MONO_BUFFER || format == COLOR_BUFFER;
}

size_t ble_data_alloc = 256;
uint8_t *ble_data = (uint8_t *)malloc(ble_data_alloc);
size_t ble_data_size = 0;

size_t mono_buffer_size = 0;
size_t color_buffer_size = 0;

ImageFormat image_format;

PNG png;
JPEGDEC jpeg;

BLECharacteristic *pCharacteristic;

void ble_init() {
  BLEDevice::init("edown");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ |
                               BLECharacteristic::PROPERTY_WRITE |
                               BLECharacteristic::PROPERTY_INDICATE);

  BLEDescriptor descriptor = BLEUUID((uint16_t)0x2902);
  pCharacteristic->addDescriptor(&descriptor);
  pCharacteristic->setCallbacks(new ble_characteristics_callbacks());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(
      0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

std::string ble_get_value() { return pCharacteristic->getValue(); }

uint8_t *ble_get_data() { return pCharacteristic->getData(); }

size_t ble_get_len() { return pCharacteristic->getLength(); }

void ble_draw_png(PNGDRAW *pDraw) {
#ifdef DEBUG
  Serial.println("WRITING DRAW DATA");
  Serial.printf("Width: %d, Y: %d\n", pDraw->iWidth, pDraw->y);
#endif

  uint16_t *usPixels = (uint16_t *)malloc(2 * 128);
  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);

  rgb565_to_buffer((uint8_t *)usPixels, pDraw->iWidth, 1, 0, 0);

  epaper_write(output_mono_buffer[0], output_color_buffer[0], pDraw->iWidth, 1,
               0, pDraw->y);
}

int ble_draw_jpeg(JPEGDRAW *pDraw) {
#ifdef DEBUG
  Serial.println("WRITING DRAW DATA");
  Serial.printf("Width: %d, Y: %d\n", pDraw->iWidth, pDraw->y);
#endif

  rgb565_to_buffer((uint8_t *)pDraw->pPixels, pDraw->iWidth, pDraw->iHeight, 0,
                   0);

  for (uint16_t i = 0; i < pDraw->iHeight; i++) {
    epaper_write(output_mono_buffer[i], output_color_buffer[i], pDraw->iWidth,
                 1, pDraw->x, pDraw->y + i);
  }

  return 1;
}

void decode_image() {
  switch (image_format) {
  case PNG_FORMAT:
    if (png.openRAM(ble_data, ble_data_size, ble_draw_png) != PNG_SUCCESS) {
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
    if (jpeg.openRAM(ble_data, ble_data_size, ble_draw_jpeg) != JPEG_SUCCESS) {
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

  free(ble_data);
  ble_data_size = 0;
  ble_data_alloc = 256;
  ble_data = (uint8_t *)malloc(ble_data_alloc);
}

void alloc_memory() {
  if (pCharacteristic->getLength() + ble_data_size > ble_data_alloc) {
    ble_data_alloc = (pCharacteristic->getLength() + ble_data_size) * 2;
    ble_data = (uint8_t *)realloc(ble_data, ble_data_alloc);

#ifdef DEBUG
    Serial.println("REALLOC");
    Serial.printf("New alloc: %zu\n", ble_data_alloc);
#endif
  }

  memcpy(ble_data + ble_data_size, pCharacteristic->getData(),
         pCharacteristic->getLength());
  ble_data_size += pCharacteristic->getLength();

#ifdef DEBUG
  Serial.println("COPYING MEMORY");
  Serial.printf("Length: %zu\n", pCharacteristic->getLength());
  Serial.printf("Data size: %zu\n", ble_data_size);
#endif
}

void ble_characteristics_callbacks::onWrite(
    BLECharacteristic *pCharacteristic) {
  if (memcmp(pCharacteristic->getValue().c_str(), "BEGIN", strlen("BEGIN")) ==
      0) {
    Serial.println("BEGIN");
  } else if (memcmp(pCharacteristic->getValue().c_str(), "PNG",
                    strlen("PNG")) == 0) {
    Serial.println("PNG");
    image_format = PNG_FORMAT;
  } else if (memcmp(pCharacteristic->getValue().c_str(), "JPEG",
                    strlen("JPEG")) == 0) {
    Serial.println("JPEG");
    image_format = JPEG_FORMAT;
  } else if (memcmp(pCharacteristic->getValue().c_str(), "MONO BUFFER",
                    strlen("MONO BUFFER")) == 0) {
    Serial.println("MONO BUFFER");
    image_format = MONO_BUFFER;
  } else if (memcmp(pCharacteristic->getValue().c_str(), "COLOR BUFFER",
                    strlen("COLOR BUFFER")) == 0) {
    Serial.println("COLOR BUFFER");
    image_format = COLOR_BUFFER;
  } else if (memcmp(pCharacteristic->getValue().c_str(), "END",
                    strlen("END")) == 0) {
    Serial.println("END");
    if (!raw_format(image_format)) {
      decode_image();
    } else {
      mono_buffer_size = 0;
      color_buffer_size = 0;
    }
  } else if (memcmp(pCharacteristic->getValue().c_str(), "DRAW",
                    strlen("DRAW")) == 0) {
    Serial.println("DRAW");
    epaper_refresh();
  } else if (memcmp(pCharacteristic->getValue().c_str(), "CLEAR",
                    strlen("CLEAR")) == 0) {
    Serial.println("CLEAR");
    epaper_clear();
  } else {
    if (!raw_format(image_format)) {
      alloc_memory();
    } else {
      if (image_format == MONO_BUFFER) {
        Serial.printf("Mono length: %zu\n", pCharacteristic->getLength());
        Serial.printf("Mono size: %zu\n", mono_buffer_size);

        memcpy((uint8_t *)&output_mono_buffer[0][0] + mono_buffer_size,
               pCharacteristic->getData(), pCharacteristic->getLength());
        mono_buffer_size += pCharacteristic->getLength();
      }
      if (image_format == COLOR_BUFFER) {
        Serial.printf("Color length: %zu\n", pCharacteristic->getLength());
        Serial.printf("Color size: %zu\n", color_buffer_size);

        memcpy((uint8_t *)&output_color_buffer[0][0] + color_buffer_size,
               pCharacteristic->getData(), pCharacteristic->getLength());
        color_buffer_size += pCharacteristic->getLength();
      }
    }
  }
}
