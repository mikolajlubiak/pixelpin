#include "Arduino.h"

#include "catbox_cert.h"
#include "draw.h"
#include "epaper.h"
#include "wifi.h"

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("init");

  epaper_init();
  wifi_init();

  Serial.println();

  // uint8_t *mono;
  // uint8_t *color;
  // uint16_t width;
  // uint16_t height;

  draw_https("files.catbox.moe", "/", "5su72d.bmp",
             "77:74:D8:7D:C1:10:2D:04:86:7C:24:2A:52:40:A6:FB:A4:73:DE:A8",
             catbox_cert);

  // draw_http("192.168.18.41", "/", "128px-Tux.svg.bmp", 8000);

  // downloadBitmapFrom_HTTPS("files.catbox.moe", "/", "5su72d.bmp",
  //                          "77:74:D8:7D:C1:10:2D:04:86:7C:24:2A:52:40:A6:FB:A4:73:DE:A8", true, catbox_cert, &mono, &color, &width, &height);

  Serial.println("done");
}

void loop(void) {}
