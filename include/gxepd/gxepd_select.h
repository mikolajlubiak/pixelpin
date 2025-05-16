#pragma once

// base class GxEPD2_GFX can be used to pass references or pointers to the
// display instance as parameter, uses ~1.2k more code enable or disable
// GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

// uncomment next line to use class GFX of library GFX_Root instead of
// Adafruit_GFX #include <GFX.h> Note: if you use this with ENABLE_GxEPD2_GFX 1:
//       uncomment it in GxEPD2_GFX.h too, or add #include <GFX.h> before any
//       #include <GxEPD2_GFX.h>

#include <GxEPD2_3C.h>
// #include <GxEPD2_BW.h>

// NOTE: you may need to adapt or select for your wiring in the processor
// specific conditional compile sections below

// select the display class (only one), matching the kind of display panel
// #define GxEPD2_DISPLAY_CLASS GxEPD2_BW
#define GxEPD2_DISPLAY_CLASS GxEPD2_3C
// #define GxEPD2_DISPLAY_CLASS GxEPD2_4C
// #define GxEPD2_DISPLAY_CLASS GxEPD2_7C

// select the display driver class (only one) for your panel
#define GxEPD2_DRIVER_CLASS GxEPD2_290_C90c
#ifndef EPD_CS
#define EPD_CS SS
#endif

// somehow there should be an easier way to do this
#define GxEPD2_BW_IS_GxEPD2_BW true
#define GxEPD2_3C_IS_GxEPD2_3C true
#define GxEPD2_4C_IS_GxEPD2_4C true
#define GxEPD2_7C_IS_GxEPD2_7C true
#define GxEPD2_1248_IS_GxEPD2_1248 true
#define GxEPD2_1248c_IS_GxEPD2_1248c true
#define IS_GxEPD(c, x) (c##x)
#define IS_GxEPD2_BW(x) IS_GxEPD(GxEPD2_BW_IS_, x)
#define IS_GxEPD2_3C(x) IS_GxEPD(GxEPD2_3C_IS_, x)
#define IS_GxEPD2_4C(x) IS_GxEPD(GxEPD2_4C_IS_, x)
#define IS_GxEPD2_7C(x) IS_GxEPD(GxEPD2_7C_IS_, x)
#define IS_GxEPD2_1248(x) IS_GxEPD(GxEPD2_1248_IS_, x)
#define IS_GxEPD2_1248c(x) IS_GxEPD(GxEPD2_1248c_IS_, x)

#if defined(ESP32)
#define MAX_DISPLAY_BUFFER_SIZE 65536ul // e.g.
#if IS_GxEPD2_BW(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD)                                                        \
  (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8)                   \
       ? EPD::HEIGHT                                                           \
       : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
#elif IS_GxEPD2_3C(GxEPD2_DISPLAY_CLASS) || IS_GxEPD2_4C(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD)                                                        \
  (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8)             \
       ? EPD::HEIGHT                                                           \
       : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
#elif IS_GxEPD2_7C(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD)                                                        \
  (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2)                 \
       ? EPD::HEIGHT                                                           \
       : (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2))
#endif
// adapt the constructor parameters to your wiring
#if !IS_GxEPD2_1248(GxEPD2_DRIVER_CLASS) &&                                    \
    !IS_GxEPD2_1248c(GxEPD2_DRIVER_CLASS)
#if defined(ARDUINO_ESP32S3_DEV)
GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>
    display(GxEPD2_DRIVER_CLASS(/*CS*/ 10, /*DC=*/3, /*RST=*/8,
                                /*BUSY=*/18));
#else
GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>
    display(GxEPD2_DRIVER_CLASS(/*CS=*/EPD_CS, /*DC=*/1, /*RST=*/10,
                                /*BUSY=*/2));
#endif
#else // GxEPD2_1248 or GxEPD2_1248c
GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>
    display(GxEPD2_DRIVER_CLASS(/*sck=*/13, /*miso=*/12, /*mosi=*/14,
                                /*cs_m1=*/23, /*cs_s1=*/22, /*cs_m2=*/16,
                                /*cs_s2=*/19,
                                /*dc1=*/25, /*dc2=*/17, /*rst1=*/33, /*rst2=*/5,
                                /*busy_m1=*/32, /*busy_s1=*/26, /*busy_m2=*/18,
                                /*busy_s2=*/4));
#endif
#undef MAX_DISPLAY_BUFFER_SIZE
#undef MAX_HEIGHT
#endif
