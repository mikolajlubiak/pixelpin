# PixelPin Architecture

## System Overview

PixelPin is a wearable e-paper display pin that streams images from a smartphone over Bluetooth Low Energy. The system has two components: the companion Flutter app (image selection, encoding, BLE transmission) and the ESP32 firmware (BLE reception, image processing, display rendering).

```
+------------------+        BLE         +-------------------+
|   Flutter App    |  ──────────────►  |    ESP32 MCU      |
|  (pixelpin-app)  |  Custom protocol  |  (this firmware)  |
|                  |                   |                   |
|  Image select    |  Commands:        |  BLE Server       |
|  RGB565 encode   |  BEGIN            |  Command parser   |
|  Chunk & send    |  MONO BUFFER      |  Buffer manager   |
|                  |  COLOR BUFFER     |  Image processor  |
|                  |  <binary data>    |  E-Paper driver   |
|                  |  END              |                   |
|                  |  DRAW             +--------+----------+
|                  |  CLEAR                     |
+------------------+                            | SPI
                                                |
                                    +-----------v-----------+
                                    |   E-Paper Display     |
                                    |   GxEPD2_290_C90c     |
                                    |   2.9" tri-color      |
                                    |   128 x 296 pixels    |
                                    |   Black / White / Red |
                                    +-----------------------+
```

---

## Component Descriptions

### `src/main.cpp` — Entry Point & Power Management

Initialises all subsystems and implements the inactivity-based deep sleep loop.

- Calls `common_init()`, `draw_init()`, and `ble_init()` in sequence.
- On **ESP32-C3**: configures GPIO 3 as a wake-up source and enters deep sleep after `TIMER` (5 minutes) of inactivity.
- `TIMER` is computed as `5 * 60,000,000` microseconds, evaluated against `esp_timer_get_time()`.

### `src/ble.cpp` + `include/ble.h` — BLE Server & Protocol Parser

Implements a BLE GATT server with a single writable characteristic. All protocol logic lives in the `onWrite` callback.

- Device name: `PixelPin`
- Service UUID: `3c9a8264-7d7e-41d3-963f-798e23f8b28f`
- Characteristic UUID: `59dee772-cb42-417b-82fe-3542909614bb`
- Characteristic property: `WRITE`

See [BLE_PROTOCOL.md](BLE_PROTOCOL.md) for the full protocol specification.

### `src/epaper.cpp` + `include/epaper.h` — E-Paper Display Driver

Wraps the GxEPD2 library. Manages two heap-allocated 1-bit framebuffers and provides the `rgb565_to_buffer()` image conversion function.

- **Mono buffer**: 1 bit per pixel — `0` = black, `1` = white
- **Color buffer**: 1 bit per pixel — `0` = colored (red), `1` = no color
- Buffer size: `128 × 296 / 8 = 4,736 bytes` each
- Display initialised at 115200 baud via SPI

See [IMAGE_PROCESSING.md](IMAGE_PROCESSING.md) for the full image processing pipeline.

### `src/tft.cpp` + `include/tft.h` — TFT LCD Driver

Alternative display backend using the Adafruit ST7789 library (enabled with `-DTFT` build flag).

- Buffer size: `320 × 240 × 2 = 153,600 bytes` (RGB565, 2 bytes per pixel)
- `tft_write()` blits the full RGB565 framebuffer directly to the display

### `src/draw.cpp` + `include/draw.h` — Display Abstraction Layer

Provides a unified API so that `ble.cpp` and `main.cpp` are display-agnostic. Routes calls to `epaper_*` or `tft_*` based on the compile-time flag.

| Function | Description |
|---|---|
| `draw_init()` | Initialise the active display |
| `draw_write(...)` | Write framebuffer(s) to the display |
| `draw_clear()` | Clear the display |
| `draw_refresh()` | Trigger a display refresh cycle |

### `src/common.cpp` + `include/common.h` — Shared Utilities

Small utilities used across the firmware.

| Function/Variable | Description |
|---|---|
| `timer` | Inactivity timestamp (ESP32-C3 only) |
| `common_init()` | Initialise and reset the timer |
| `common_clean()` | Reset the timer to 0 |
| `clamp(val, min, max)` | Clamp a `uint32_t` value |
| `uint8_to_uint64(buf)` | Read 8 bytes into a `uint64_t` (little-endian) |

### `include/gxepd/gxepd_select.h` — Display Driver Configuration

Selects the GxEPD2 display class and driver at compile time and wires the GPIO pin assignments for each supported board.

---

## BLE Protocol

The firmware implements a custom text-command + binary-data protocol over a single BLE GATT characteristic. The companion app sends text commands to control the transfer state and raw bytes for pixel data.

See [BLE_PROTOCOL.md](BLE_PROTOCOL.md) for the complete specification including packet format, all command types, and the state machine.

### Quick Reference

```
App                                   ESP32
 |                                      |
 |-------- "BEGIN" ------------------>  | Reset buffer offsets
 |-------- "MONO BUFFER" ------------>  | Clear mono buffer (0xFF), activate mono mode
 |-------- <binary pixel data> ------>  | Accumulate into mono_buffer
 |-------- "COLOR BUFFER" ----------->  | Clear color buffer (0xFF), activate color mode
 |-------- <binary pixel data> ------>  | Accumulate into color_buffer
 |-------- "END" -------------------->  | Call draw_write() — write buffers to display
 |-------- "DRAW" ------------------->  | Call draw_refresh() — trigger e-paper refresh
 |                                      |
```

---

## Image Processing Pipeline

The full image conversion happens in `rgb565_to_buffer()` in `src/epaper.cpp`. The input is an RGB565-encoded image; the output is two 1-bit framebuffers ready for the e-paper display.

See [IMAGE_PROCESSING.md](IMAGE_PROCESSING.md) for the detailed algorithm description.

### Summary

```
RGB565 input (2 bytes/pixel)
        |
        v
  Extract R, G, B channels
        |
        v
  Luminance check  ──► whitish?  ──► keep white  ──► mono_buffer bit = 1
        |                                              color_buffer bit = 1
        |
        v
  Hue check  ──► reddish/yellowish?  ──► colored  ──► mono_buffer bit = 0
        |                                               color_buffer bit = 0
        |
        v
  Otherwise  ──► black  ──► mono_buffer bit = 0
                              color_buffer bit = 1

  8 pixels packed into 1 byte, MSB first
```

---

## State Machine

The firmware's BLE protocol handler is an implicit state machine driven by the content of each BLE write operation.

```
                    +-------+
         power on   |       |
         ---------> | IDLE  |
                    |  (BLE |
                    | adv.) |
                    +---+---+
                        |
                  "BEGIN" received
                        |
                        v
                 +------+------+
                 | TRANSFER    |
                 |  STARTED    |
                 | (buf reset) |
                 +------+------+
                        |
             +-----------+-----------+
             |                       |
    "MONO BUFFER"            "COLOR BUFFER"
             |                       |
             v                       v
    +---------+----+        +---------+----+
    | RECEIVING    |        | RECEIVING    |
    | MONO DATA    |        | COLOR DATA   |
    |  (chunks)    |        |  (chunks)    |
    +------+-------+        +------+-------+
             |                       |
             +----------+------------+
                        |
                     "END" received
                        |
                        v
                +-------+-------+
                | WRITING TO    |
                | DISPLAY       |
                | (draw_write)  |
                +-------+-------+
                        |
                     "DRAW" received
                        |
                        v
                +-------+-------+
                | REFRESHING    |
                | DISPLAY       |
                | (epaper_ref.) |
                +-------+-------+
                        |
                        v
                    +---+---+
                    | IDLE  |
                    +-------+

  At any point: "CLEAR" → draw_clear() → IDLE
  On inactivity (5 min, ESP32-C3 only): deep sleep
  On button press (GPIO 3): wake from deep sleep → power on
```

---

## Memory Management

The ESP32 has limited RAM (~300 KB on ESP32-C3, ~512 KB on ESP32-S3). The firmware allocates the display framebuffers on the heap and manages them carefully.

### E-Paper Mode (EPD)

| Buffer | Size | Allocation |
|---|---|---|
| `mono_buffer` | 4,736 bytes (128 × 296 / 8) | `malloc` in `epaper_init()` |
| `color_buffer` | 4,736 bytes (128 × 296 / 8) | `malloc` in `epaper_init()` |
| **Total** | **9,472 bytes** | freed in `epaper_clean()` |

BLE data is written directly into these pre-allocated buffers using `memcpy`. The `mono_buffer_size` and `color_buffer_size` counters track the current fill offset. Buffers are pre-cleared with `memset(..., 0xFF, BUFFER_SIZE)` before each transfer (0xFF = all white / no color).

### TFT Mode

| Buffer | Size | Allocation |
|---|---|---|
| `tft_buffer` | 153,600 bytes (320 × 240 × 2) | `malloc` in `tft_init()` |

The full RGB565 image is accumulated in the single buffer and blitted to the display in one operation.

### Deep Sleep

On deep sleep entry (`esp_deep_sleep_start()`), `ble_clean()` resets the buffer offset counters and `common_clean()` resets the inactivity timer. The heap-allocated buffers are released on the next wake-up cycle during `epaper_init()`.
