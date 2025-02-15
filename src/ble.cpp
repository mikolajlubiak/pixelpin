#include "ble.h"

#include <string>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include <Arduino.h>

#include "buffer.h"
#include "draw.h"
#include "image.h"

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

enum BleState {
  UPLOAD,
  SLEEP,
};

BleState ble_state;

size_t mono_buffer_size = 0;
size_t color_buffer_size = 0;

BLECharacteristic *pCharacteristic;

void ble_init() {
  BLEDevice::init("edown");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ble_server_callbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new ble_characteristics_callbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
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
    reset_image();
    mono_buffer_size = 0;
    color_buffer_size = 0;
  } else if (memcmp(pCharacteristic->getValue().c_str(), "MONO BUFFER",
                    strlen("MONO BUFFER")) == 0) {
    Serial.println("MONO BUFFER");
    buffer_type = MONO_BUFFER;
    ble_state = UPLOAD;
    memset(mono_buffer, 0xFF, BUFFER_SIZE);
  } else if (memcmp(pCharacteristic->getValue().c_str(), "COLOR BUFFER",
                    strlen("COLOR BUFFER")) == 0) {
    Serial.println("COLOR BUFFER");
    buffer_type = COLOR_BUFFER;
    ble_state = UPLOAD;
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
  } else if (memcmp(pCharacteristic->getValue().c_str(), "SLEEP",
                    strlen("SLEEP")) == 0) {
    Serial.println("SLEEP");
    ble_state = SLEEP;
  } else {
    if (ble_state == UPLOAD) {
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
    } else if (ble_state == SLEEP) {
      uint64_t time = stoi(pCharacteristic->getValue());
      esp_sleep_enable_timer_wakeup(time);
      Serial.flush();
      esp_deep_sleep_start();
    }
  }
  Serial.flush();
}

void ble_server_callbacks::onConnect(BLEServer *pServer) {
  BLEDevice::startAdvertising();
};

void ble_server_callbacks::onDisconnect(BLEServer *pServer) {
  BLEDevice::startAdvertising();
}
