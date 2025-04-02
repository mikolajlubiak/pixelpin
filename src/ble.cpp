#include "ble.h"

#include <string>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include <Arduino.h>

#include "common.h"
#include "draw.h"
#include "image.h"

#define SERVICE_UUID "3c9a8264-7d7e-41d3-963f-798e23f8b28f"
#define CHARACTERISTIC_UUID "59dee772-cb42-417b-82fe-3542909614bb"

size_t mono_buffer_size;
size_t color_buffer_size;

void ble_init() {
  ble_clean();

  BLEDevice::init("PixelPin");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new PixelPinBLEServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new PixelPinBLECharacteristicCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void PixelPinBLECharacteristicCallbacks::onWrite(
    BLECharacteristic *pCharacteristic) {
  // Restart the inactivity timer
  timer = esp_timer_get_time();

  if (memcmp(pCharacteristic->getValue().c_str(), "BEGIN", strlen("BEGIN")) ==
      0) {
    Serial.println("BEGIN");
    image_clean();
    mono_buffer_size = 0;
    color_buffer_size = 0;
  } else if (memcmp(pCharacteristic->getValue().c_str(), "MONO BUFFER",
                    strlen("MONO BUFFER")) == 0) {
    Serial.println("MONO BUFFER");
    buffer_type = MONO_BUFFER;
    memset(mono_buffer, 0xFF, BUFFER_SIZE);
  } else if (memcmp(pCharacteristic->getValue().c_str(), "COLOR BUFFER",
                    strlen("COLOR BUFFER")) == 0) {
    Serial.println("COLOR BUFFER");
    buffer_type = COLOR_BUFFER;
    memset(color_buffer, 0xFF, BUFFER_SIZE);
  } else if (memcmp(pCharacteristic->getValue().c_str(), "END",
                    strlen("END")) == 0) {
    Serial.println("END");
    draw_write(mono_buffer, color_buffer, MAX_COL, MAX_ROW, 0, 0);
  } else if (memcmp(pCharacteristic->getValue().c_str(), "DRAW",
                    strlen("DRAW")) == 0) {
    Serial.println("DRAW");
    draw_refresh();
  } else if (memcmp(pCharacteristic->getValue().c_str(), "CLEAR",
                    strlen("CLEAR")) == 0) {
    Serial.println("CLEAR");
    draw_clear();
  } else {
    if (buffer_type == MONO_BUFFER) {
      memcpy(mono_buffer + mono_buffer_size, pCharacteristic->getData(),
             pCharacteristic->getLength());
      mono_buffer_size += pCharacteristic->getLength();
    }
    if (buffer_type == COLOR_BUFFER) {
      memcpy(color_buffer + color_buffer_size, pCharacteristic->getData(),
             pCharacteristic->getLength());
      color_buffer_size += pCharacteristic->getLength();
    }
  }
}

// Advertise non stop
void PixelPinBLEServerCallbacks::onConnect(BLEServer *pServer) {
  BLEDevice::startAdvertising();
};

void PixelPinBLEServerCallbacks::onDisconnect(BLEServer *pServer) {
  BLEDevice::startAdvertising();
}

void ble_clean() {
  mono_buffer_size = 0;
  color_buffer_size = 0;
}
