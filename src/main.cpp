#include <Arduino.h>
#include <esp_sleep.h>

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
  // Sleep after 5 minutes of inactivity
  if (esp_timer_get_time() - timer >= static_cast<uint64_t>(3e+8)) {
    Serial.println("Going into deep sleep mode");
    esp_deep_sleep_start();
  }
}
