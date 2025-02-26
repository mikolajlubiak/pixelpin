#pragma once

#include <stdint.h>
#include <string>

#include <BLECharacteristic.h>
#include <BLEServer.h>

void ble_init();

class PixelPinBLECharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic);
};

class PixelPinBLEServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer);

  void onDisconnect(BLEServer *pServer);
};
