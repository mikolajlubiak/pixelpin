#pragma once

#include <stdint.h>
#include <string>

#include <BLECharacteristic.h>
#include <BLEServer.h>

void ble_init();

class EDownBLECharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic);
};

class EDownBLEServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer);

  void onDisconnect(BLEServer *pServer);
};
