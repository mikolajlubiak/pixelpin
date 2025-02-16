#include <Arduino.h>

#include "ble.h"
#include "common.h"
#include "draw.h"
#include "epaper.h"

void setup() {
  Serial.begin(115200);
  Serial.println();

  Serial.println("Initializing");
  epaper_init();
  ble_init();

  // Wake up after pressing button
  esp_deep_sleep_enable_gpio_wakeup(1ULL << GPIO_NUM_3,
                                    ESP_GPIO_WAKEUP_GPIO_HIGH);

  Serial.println("Initializing done");
}

void loop() {
  // Sleep after 1 minute of inactivity
  if (esp_timer_get_time() - timer >= static_cast<uint64_t>(6e+7)) {
    Serial.println("Going into deep sleep mode");
    esp_deep_sleep_start();
  }
}
