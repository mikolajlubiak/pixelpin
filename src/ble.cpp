#include "ble.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include <Arduino.h>

#include "buffer.h"
#include "draw.h"
#include "image.h"

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

size_t mono_buffer_size = 0;
size_t color_buffer_size = 0;

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
      draw_write((uint8_t *)output_mono_buffer, (uint8_t *)output_color_buffer,
                 MAX_COL, MAX_ROW, 0, 0);
      mono_buffer_size = 0;
      color_buffer_size = 0;
    }
  } else if (memcmp(pCharacteristic->getValue().c_str(), "DRAW",
                    strlen("DRAW")) == 0) {
    Serial.println("DRAW");
    draw_refresh();
  } else if (memcmp(pCharacteristic->getValue().c_str(), "CLEAR",
                    strlen("CLEAR")) == 0) {
    Serial.println("CLEAR");
    draw_clear();
  } else {
    if (!raw_format(image_format)) {
      alloc_memory(pCharacteristic->getData(), pCharacteristic->getLength());
    } else {
      if (image_format == MONO_BUFFER) {
        memcpy(output_mono_buffer + mono_buffer_size,
               pCharacteristic->getData(), pCharacteristic->getLength());
        mono_buffer_size += pCharacteristic->getLength();
      }
      if (image_format == COLOR_BUFFER) {
        memcpy(output_color_buffer + color_buffer_size,
               pCharacteristic->getData(), pCharacteristic->getLength());
        color_buffer_size += pCharacteristic->getLength();
      }
    }
  }
}
