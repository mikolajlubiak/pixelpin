#include <Arduino.h>
#include <esp_sleep.h>

#define MINUTE_TO_MICROSEC 60000000
#define TIMER (5*MINUTE_TO_MICROSEC)

#include "ble.h"
#include "common.h"
#include "draw.h"
#include "epaper.h"
#include "image.h"

void setup() {
  Serial.begin(9600);
  Serial.println();

  Serial.println("Initializing");

  image_clean();
  common_init();
  epaper_init();
  ble_init();

  // Wake up after pressing button
  esp_deep_sleep_enable_gpio_wakeup(1ULL << GPIO_NUM_3,
                                    ESP_GPIO_WAKEUP_GPIO_HIGH);

  Serial.println("Initializing done");
}

void loop() {
  // Sleep after 5 minutes of inactivity
  if (esp_timer_get_time() - timer >= TIMER) {
    common_clean();
    image_clean();
    ble_clean();

    Serial.println("Going into deep sleep mode");
    esp_deep_sleep_start();
  }
}
