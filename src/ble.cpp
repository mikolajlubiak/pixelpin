#include "ble.h"

#include <string>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include <Arduino.h>

#include "common.h"
#include "draw.h"

#ifdef EPD
#include "epaper.h"
#elif TFT
#include "tft.h"
#endif

#define SERVICE_UUID "3c9a8264-7d7e-41d3-963f-798e23f8b28f"
#define CHARACTERISTIC_UUID "59dee772-cb42-417b-82fe-3542909614bb"

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
#ifdef ARDUINO_ESP32C3_DEV
  // Restart the inactivity timer
  timer = esp_timer_get_time();
#endif

  if (memcmp(pCharacteristic->getValue().c_str(), "BEGIN", strlen("BEGIN")) ==
      0) {
    Serial.println("BEGIN");
#ifdef EPD
    mono_buffer_size = 0;
    color_buffer_size = 0;
#elif TFT
    tft_buffer_size = 0;
#endif
  }
#ifdef EPD
  else if (memcmp(pCharacteristic->getValue().c_str(), "MONO BUFFER",
                  strlen("MONO BUFFER")) == 0) {
    Serial.println("MONO BUFFER");
    buffer_type = MONO_BUFFER;
    memset(mono_buffer, 0xFF, BUFFER_SIZE);
  } else if (memcmp(pCharacteristic->getValue().c_str(), "COLOR BUFFER",
                    strlen("COLOR BUFFER")) == 0) {
    Serial.println("COLOR BUFFER");
    buffer_type = COLOR_BUFFER;
    memset(color_buffer, 0xFF, BUFFER_SIZE);
  }
#elif TFT
  else if (memcmp(pCharacteristic->getValue().c_str(), "TFT BUFFER",
                  strlen("TFT BUFFER")) == 0) {
    Serial.println("TFT BUFFER");
    memset(tft_buffer, 0x00, BUFFER_SIZE);
  }
#endif
  else if (memcmp(pCharacteristic->getValue().c_str(), "END", strlen("END")) ==
           0) {
    Serial.println("END");
#ifdef EPD
    draw_write(mono_buffer, color_buffer, MAX_COL, MAX_ROW, 0, 0);
#elif TFT
    draw_write(tft_buffer, MAX_COL, MAX_ROW, 0, 0);
#endif
  } else if (memcmp(pCharacteristic->getValue().c_str(), "DRAW",
                    strlen("DRAW")) == 0) {
    Serial.println("DRAW");
    draw_refresh();
  } else if (memcmp(pCharacteristic->getValue().c_str(), "CLEAR",
                    strlen("CLEAR")) == 0) {
    Serial.println("CLEAR");
    draw_clear();
  } else {
#ifdef EPD
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
#elif TFT
    memcpy(tft_buffer + tft_buffer_size, pCharacteristic->getData(),
           pCharacteristic->getLength());
    tft_buffer_size += pCharacteristic->getLength();
#endif
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
#ifdef EPD
  mono_buffer_size = 0;
  color_buffer_size = 0;
#elif TFT
  tft_buffer_size = 0;
#endif
}
