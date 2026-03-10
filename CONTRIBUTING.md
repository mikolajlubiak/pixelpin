# Contributing to PixelPin

Thank you for your interest in contributing to PixelPin! This guide covers everything you need to build the firmware, understand the codebase, and extend it.

## Hardware Requirements

### Microcontroller

The firmware is tested on:

| Board | Chip | Notes |
|---|---|---|
| ESP32-C3-DevKitM-1 | ESP32-C3 | Recommended; has deep sleep + GPIO wake-up support |
| ESP32-S3-DevKitM-1 | ESP32-S3 | Supported; deep sleep not wired in firmware |

Any ESP32-C3 or ESP32-S3 board should work if you adjust the GPIO pin assignments in `include/gxepd/gxepd_select.h` and `src/tft.cpp`.

### E-Paper Display (primary)

| Parameter | Value |
|---|---|
| Model | GxEPD2_290_C90c (Waveshare 2.9" V2 tri-color) |
| Resolution | 128 × 296 pixels |
| Colors | Black, White, Red |
| Interface | SPI |

Wiring to **ESP32-C3**:

| Display Pin | ESP32-C3 Pin |
|---|---|
| CS | SS (default SPI CS pin) |
| DC | GPIO 1 |
| RST | GPIO 10 |
| BUSY | GPIO 2 |
| CLK | hardware SPI CLK |
| MOSI | hardware SPI MOSI |
| GND | GND |
| VCC | 3.3V |

Wiring to **ESP32-S3**:

| Display Pin | ESP32-S3 Pin |
|---|---|
| CS | GPIO 10 |
| DC | GPIO 3 |
| RST | GPIO 8 |
| BUSY | GPIO 18 |

### TFT Display (alternative)

| Parameter | Value |
|---|---|
| Controller | ST7789 |
| Resolution | 320 × 240 pixels |
| Interface | SPI |

Wiring:

| Display Pin | ESP32 Pin |
|---|---|
| CS | GPIO 10 |
| DC | GPIO 8 |
| CLK | hardware SPI CLK |
| MOSI | hardware SPI MOSI |
| GND | GND |
| VCC | 3.3V |

### Wake-up Button (ESP32-C3 only)

Connect a momentary push-button between **GPIO 3** and **3.3V**. The firmware configures GPIO 3 as a high-level wake-up source.

---

## Build Setup

### Prerequisites

Install [PlatformIO](https://platformio.org/). The CLI is recommended:

```bash
pip install platformio
```

Or install the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode) for VS Code.

### Build Environments

The `platformio.ini` defines two build environments:

| Environment | Board |
|---|---|
| `esp32-c3-devkitm-1` | ESP32-C3-DevKitM-1 |
| `esp32-s3-devkitm-1` | ESP32-S3-DevKitM-1 |

Both use the Arduino framework. The default build flag is `-DEPD` (e-paper display). Change this to `-DTFT` in `platformio.ini` to build for the TFT backend.

### Build, Flash, and Monitor

```bash
# Build for ESP32-C3 (e-paper)
pio run -e esp32-c3-devkitm-1

# Flash to ESP32-C3
pio run -e esp32-c3-devkitm-1 --target upload

# Open serial monitor (9600 baud)
pio device monitor

# Build for ESP32-S3
pio run -e esp32-s3-devkitm-1 --target upload
```

### Dependencies

Dependencies are declared in `platformio.ini` and downloaded automatically by PlatformIO:

| Library | Version | Purpose |
|---|---|---|
| `zinggjm/GxEPD2` | ^1.6.2 | E-paper display driver |
| `adafruit/Adafruit ST7735 and ST7789 Library` | ^1.11.0 | TFT display driver |

---

## Project Structure

```
pixelpin/
├── platformio.ini          # PlatformIO build configuration
├── src/
│   ├── main.cpp            # Entry point, deep sleep loop
│   ├── ble.cpp             # BLE GATT server, protocol parser
│   ├── common.cpp          # Shared utilities, inactivity timer
│   ├── draw.cpp            # Display abstraction layer
│   ├── epaper.cpp          # E-paper driver, RGB565→buffer conversion
│   └── tft.cpp             # TFT driver
├── include/
│   ├── ble.h
│   ├── common.h
│   ├── draw.h
│   ├── epaper.h
│   ├── tft.h
│   └── gxepd/
│       └── gxepd_select.h  # GxEPD2 display selection and GPIO config
└── docs/
    ├── ARCHITECTURE.md
    ├── BLE_PROTOCOL.md
    └── IMAGE_PROCESSING.md
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for a full description of each file.

---

## How to Modify the BLE Protocol

The BLE command parser lives in `PixelPinBLECharacteristicCallbacks::onWrite()` in `src/ble.cpp`.

### Adding a New Command

1. Add a new `else if` branch in `onWrite()` that compares the incoming value against your new command string:

```cpp
else if (memcmp(pCharacteristic->getValue().c_str(), "MY_COMMAND",
                strlen("MY_COMMAND")) == 0) {
  Serial.println("MY_COMMAND");
  // your logic here
}
```

2. If the command is display-type-specific, wrap it in the appropriate `#ifdef EPD` / `#elif TFT` guard.

3. Update [docs/BLE_PROTOCOL.md](docs/BLE_PROTOCOL.md) with the new command's payload format, effect, and how it fits into the transfer flow.

4. Update the companion app (`pixelpin-app`) to send the new command at the right point in the transfer sequence.

### Changing Buffer Behavior

Buffer sizes are defined in `include/epaper.h` (`MAX_ROW`, `MAX_COL`, `BUFFER_SIZE`) and `include/tft.h`. If you change the display resolution, update these constants and ensure the companion app sends the correct amount of data.

---

## How to Add Support for a New Display

### 1. E-Paper Display (GxEPD2-compatible)

GxEPD2 supports dozens of e-paper panels. To add a new one:

1. Find the driver class name in the [GxEPD2 repository](https://github.com/ZinggJM/GxEPD2/tree/master/src).
2. Update `include/gxepd/gxepd_select.h`:
   - Change `#define GxEPD2_DRIVER_CLASS` to your driver.
   - Update `GxEPD2_DISPLAY_CLASS` if the color depth changes (e.g., `GxEPD2_BW` for black/white, `GxEPD2_3C` for tri-color).
   - Add a new `#if defined(ARDUINO_...)` block with the correct GPIO assignments.
3. Update `MAX_ROW` and `MAX_COL` in `include/epaper.h` to match the new display resolution.
4. If the color model changes (e.g., 2-color vs 3-color), update `rgb565_to_buffer()` in `src/epaper.cpp` and the buffer management logic in `src/ble.cpp`.

### 2. TFT Display (Adafruit GFX-compatible)

1. Add the required library to `lib_deps` in `platformio.ini`.
2. Replace the `Adafruit_ST7789` instance in `src/tft.cpp` with your display's class.
3. Update `MAX_ROW`, `MAX_COL`, and `BUFFER_SIZE` in `include/tft.h`.
4. Adjust the constructor's GPIO pin arguments to match your wiring.

---

## Code Style

The codebase follows standard embedded C++ conventions:

- C++17 or later features (where the Arduino ESP32 toolchain supports them)
- `snake_case` for functions and variables
- `UPPER_CASE` for macros and constants
- `PascalCase` for class names
- Conditional compilation via `#ifdef EPD` / `#ifdef TFT` for display-specific code
- Conditional compilation via `#ifdef ARDUINO_ESP32C3_DEV` for board-specific code
- No dynamic memory allocation after `init()` — all heap buffers are allocated once in `epaper_init()` / `tft_init()` and reused for the device's lifetime

---

## Reporting Issues

If you find a bug or have a feature request, please open a GitHub issue with:

- Your hardware (board model, display model)
- Build environment (`esp32-c3-devkitm-1` or `esp32-s3-devkitm-1`)
- The display mode (`-DEPD` or `-DTFT`)
- Serial monitor output (if applicable)
- Steps to reproduce
