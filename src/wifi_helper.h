#pragma once

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

uint16_t read16(WiFiClient &client) {
  // BMP data is stored little-endian, same as Arduino.
  uint16_t result;
  ((uint8_t *)&result)[0] = client.read(); // LSB
  ((uint8_t *)&result)[1] = client.read(); // MSB
  return result;
}

uint32_t read32(WiFiClient &client) {
  // BMP data is stored little-endian, same as Arduino.
  uint32_t result;
  ((uint8_t *)&result)[0] = client.read(); // LSB
  ((uint8_t *)&result)[1] = client.read();
  ((uint8_t *)&result)[2] = client.read();
  ((uint8_t *)&result)[3] = client.read(); // MSB
  return result;
}

#if USE_BearSSL

uint32_t skip(BearSSL::WiFiClientSecure &client, int32_t bytes) {
  int32_t remain = bytes;
  uint32_t start = millis();
  while ((client.connected() || client.available()) && (remain > 0)) {
    if (client.available()) {
      client.read();
      remain--;
    } else
      delay(10);
    if (millis() - start > 2000)
      break; // don't hang forever
  }
  return bytes - remain;
}

uint32_t read8n(BearSSL::WiFiClientSecure &client, uint8_t *buffer,
                int32_t bytes) {
  int32_t remain = bytes;
  uint32_t start = millis();
  while ((client.connected() || client.available()) && (remain > 0)) {
    if (client.available()) {
      int16_t v = client.read();
      *buffer++ = uint8_t(v);
      remain--;
    } else
      delay(10);
    if (millis() - start > 2000)
      break; // don't hang forever
  }
  return bytes - remain;
}

#endif

uint32_t skip(WiFiClient &client, int32_t bytes) {
  int32_t remain = bytes;
  uint32_t start = millis();
  while ((client.connected() || client.available()) && (remain > 0)) {
    if (client.available()) {
      client.read();
      remain--;
    } else
      delay(1);
    if (millis() - start > 2000)
      break; // don't hang forever
  }
  return bytes - remain;
}

uint32_t read8n(WiFiClient &client, uint8_t *buffer, int32_t bytes) {
  int32_t remain = bytes;
  uint32_t start = millis();
  while ((client.connected() || client.available()) && (remain > 0)) {
    if (client.available()) {
      int16_t v = client.read();
      *buffer++ = uint8_t(v);
      remain--;
    } else
      delay(1);
    if (millis() - start > 2000)
      break; // don't hang forever
  }
  return bytes - remain;
}

// Set time via NTP, as required for x.509 validation
void setClock() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}
