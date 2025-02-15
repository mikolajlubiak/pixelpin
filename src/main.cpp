#include <Arduino.h>

#include "ble.h"
#include "draw.h"
#include "epaper.h"

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("init");

  epaper_init();
  ble_init();

  Serial.println("done");
  Serial.flush();
}

void loop() {}
