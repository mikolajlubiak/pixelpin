#pragma once

#include <stdint.h>
#include <string>

#include <BLECharacteristic.h>

void ble_init();

std::string ble_get_value();

uint8_t *ble_get_data();

size_t ble_get_len();

class ble_characteristics_callbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic);
};
