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
};

size_t ble_data_alloc = 256;
uint8_t *ble_data = (uint8_t *)malloc(ble_data_alloc);
size_t ble_data_size = 0;

bool began = false;
bool decoded = false;
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

  rgb565_to_buffer((uint8_t *)usPixels, pDraw->iWidth, 1);

  epaper_write(output_mono_buffer[0], output_color_buffer[0], pDraw->iWidth, 1,
               0, pDraw->y);
}

int ble_draw_jpeg(JPEGDRAW *pDraw) {
#ifdef DEBUG
  Serial.println("WRITING DRAW DATA");
  Serial.printf("Width: %d, Y: %d\n", pDraw->iWidth, pDraw->y);
#endif

  rgb565_to_buffer((uint8_t *)pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);

  for (uint16_t i = 0; i < pDraw->iHeight; i++) {
    epaper_write(output_mono_buffer[i], output_color_buffer[i], pDraw->iWidth,
                 1, pDraw->x, pDraw->y + i);
  }

  return 1;
}

void ble_characteristics_callbacks::onWrite(
    BLECharacteristic *pCharacteristic) {
  if (memcmp(pCharacteristic->getValue().c_str(), "BEGIN", strlen("BEGIN")) ==
      0) {
    Serial.println("BEGIN");
    began = true;
    decoded = false;
  } else if (memcmp(pCharacteristic->getValue().c_str(), "PNG",
                    strlen("PNG")) == 0 &&
             began) {
    Serial.println("PNG");
    image_format = PNG_FORMAT;
  } else if (memcmp(pCharacteristic->getValue().c_str(), "JPEG",
                    strlen("JPEG")) == 0 &&
             began) {
    Serial.println("JPEG");
    image_format = JPEG_FORMAT;
  } else if (memcmp(pCharacteristic->getValue().c_str(), "END",
                    strlen("END")) == 0 &&
             began) {
    Serial.println("END");
    switch (image_format) {
    case PNG_FORMAT:
      if (png.openRAM(ble_data, ble_data_size, ble_draw_png) != PNG_SUCCESS) {
        Serial.println("[ble_characteristics_callbacks::onWrite] - Error: "
                       "png.openRAM failed");
      }
      if (png.decode(NULL, 0) != PNG_SUCCESS) {
        Serial.println("[ble_characteristics_callbacks::onWrite] - Error: "
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
      if (jpeg.openRAM(ble_data, ble_data_size, ble_draw_jpeg) !=
          JPEG_SUCCESS) {
        Serial.println("[ble_characteristics_callbacks::onWrite] - Error: "
                       "jpeg.openRAM failed");
      }
      jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
      if (jpeg.decode(0, 0, 0) != JPEG_SUCCESS) {
        Serial.println("[ble_characteristics_callbacks::onWrite] - Error: "
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

    decoded = true;
    began = false;

    ble_data_alloc = 256;
    ble_data_size = 0;
    free(ble_data);
    ble_data = (uint8_t *)malloc(ble_data_alloc);
  } else if (memcmp(pCharacteristic->getValue().c_str(), "DRAW",
                    strlen("DRAW")) == 0 &&
             decoded) {
    Serial.println("DRAW");
    epaper_refresh();
  } else if (memcmp(pCharacteristic->getValue().c_str(), "CLEAR",
                    strlen("CLEAR")) == 0) {
    Serial.println("CLEAR");
    epaper_clear();
  } else {
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
}
