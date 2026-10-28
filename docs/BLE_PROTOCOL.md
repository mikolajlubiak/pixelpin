# BLE Protocol Specification

PixelPin uses a custom text-command + binary-data protocol over a single BLE GATT characteristic. The companion Flutter app is the client (central); the ESP32 firmware is the server (peripheral).

## Service and Characteristic

| Attribute | Value |
|---|---|
| Device Name | `PixelPin` |
| Service UUID | `3c9a8264-7d7e-41d3-963f-798e23f8b28f` |
| Characteristic UUID | `59dee772-cb42-417b-82fe-3542909614bb` |
| Characteristic Property | `WRITE` |

The firmware advertises continuously, including after a client connects, so multiple sequential connections are supported without restarting.

---

## Packet Format

Each BLE write operation carries either a **text command** or a **binary data chunk**.

```
+---------------------------+
|  BLE Characteristic Write |
+---------------------------+
|  Payload (<= 512 bytes)   |
|  either:                  |
|    - ASCII command string |
|    - raw binary bytes     |
+---------------------------+
```

Command recognition works by comparing the raw bytes of the write payload against known command strings using `memcmp`. Any write that does not match a known command string is treated as binary pixel data and appended to the active buffer.

---

## Command Types

### `BEGIN`

Marks the start of a new image transfer. Resets all buffer write-position counters.

```
Payload:  "BEGIN"  (5 bytes, ASCII)
Effect:   mono_buffer_size  = 0   (EPD mode)
          color_buffer_size = 0   (EPD mode)
          tft_buffer_size   = 0   (TFT mode)
```

### `MONO BUFFER` *(EPD mode only)*

Activates the monochrome buffer as the destination for subsequent binary data writes. Pre-fills the entire mono buffer with `0xFF` (all white).

```
Payload:  "MONO BUFFER"  (11 bytes, ASCII)
Effect:   buffer_type = MONO_BUFFER
          memset(mono_buffer, 0xFF, BUFFER_SIZE)
```

### `COLOR BUFFER` *(EPD mode only)*

Activates the color buffer as the destination for subsequent binary data writes. Pre-fills the entire color buffer with `0xFF` (no color).

```
Payload:  "COLOR BUFFER"  (12 bytes, ASCII)
Effect:   buffer_type = COLOR_BUFFER
          memset(color_buffer, 0xFF, BUFFER_SIZE)
```

### `TFT BUFFER` *(TFT mode only)*

Activates the TFT buffer and pre-fills it with `0x00` (black).

```
Payload:  "TFT BUFFER"  (10 bytes, ASCII)
Effect:   memset(tft_buffer, 0x00, BUFFER_SIZE)
```

### Binary Data (no command keyword)

Any write payload that does not match a command string is treated as raw pixel data and appended to the currently active buffer. Each chunk is clamped to the space left in that buffer, so data past the end of a plane is dropped.

```
Payload:  <raw bytes>
Effect (EPD, MONO_BUFFER):
    n = min(length, BUFFER_SIZE - mono_buffer_size)
    memcpy(mono_buffer + mono_buffer_size, data, n)
    mono_buffer_size += n

Effect (EPD, COLOR_BUFFER):
    n = min(length, BUFFER_SIZE - color_buffer_size)
    memcpy(color_buffer + color_buffer_size, data, n)
    color_buffer_size += n

Effect (TFT):
    n = min(length, BUFFER_SIZE - tft_buffer_size)
    memcpy(tft_buffer + tft_buffer_size, data, n)
    tft_buffer_size += n
```

The data format depends on the display mode:
- **EPD**: 1-bit packed pixel data (1 bit per pixel, 8 pixels per byte, MSB first), produced by the app's [`convertImage()`](https://github.com/mikolajlubiak/pixelpin-app/blob/main/lib/src/img/img.dart).
- **TFT** (experimental): RGB565 pixel data, 2 bytes per pixel, copied as-is into a `uint16_t` buffer, so the firmware expects native little-endian order.

### `END`

Signals that all pixel data has been sent. Triggers the write of the accumulated buffer(s) to the display.

```
Payload:  "END"  (3 bytes, ASCII)
Effect (EPD):
    draw_write(mono_buffer, color_buffer, MAX_COL, MAX_ROW, 0, 0)
    which calls epaper_write(), which calls display.writeImage()
Effect (TFT):
    draw_write(tft_buffer, MAX_COL, MAX_ROW, 0, 0)
    which calls tft_write(), which calls tft.drawRGBBitmap()
```

### `DRAW`

Triggers an e-paper display refresh cycle. On TFT this is a no-op because TFT updates are immediate.

```
Payload:  "DRAW"  (4 bytes, ASCII)
Effect (EPD):  display.refresh(true)
Effect (TFT):  no operation
```

### `CLEAR`

Clears the display to its default state.

```
Payload:  "CLEAR"  (5 bytes, ASCII)
Effect (EPD):  display.clearScreen()
Effect (TFT):  fill screen with black
```

---

## Transfer Flow (E-Paper Image)

The typical sequence for sending a complete image to the e-paper display:

```
App (Flutter)                                  ESP32 Firmware
     |                                               |
     |  write("BEGIN")                               |
     | --------------------------------------------> |  Reset buffer offsets
     |                                               |
     |  write("MONO BUFFER")                         |
     | --------------------------------------------> |  Clear mono_buffer (0xFF)
     |                                               |  buffer_type = MONO_BUFFER
     |                                               |
     |  write(<mono pixel chunk 1>)                  |
     | --------------------------------------------> |  Append to mono_buffer
     |  write(<mono pixel chunk 2>)                  |
     | --------------------------------------------> |  Append to mono_buffer
     |  ...  (repeat until all 4,736 bytes sent)     |
     |                                               |
     |  write("COLOR BUFFER")                        |
     | --------------------------------------------> |  Clear color_buffer (0xFF)
     |                                               |  buffer_type = COLOR_BUFFER
     |                                               |
     |  write(<color pixel chunk 1>)                 |
     | --------------------------------------------> |  Append to color_buffer
     |  ...  (repeat until all 4,736 bytes sent)     |
     |                                               |
     |  write("END")                                 |
     | --------------------------------------------> |  epaper_write(mono, color, 128, 296)
     |                                               |
     |  write("DRAW")                                |
     | --------------------------------------------> |  display.refresh(true)
     |                                               |
```

The total pixel data per image:
- Mono buffer: `128 × 296 / 8 = 4,736 bytes`
- Color buffer: `128 × 296 / 8 = 4,736 bytes`
- **Total: 9,472 bytes**

The app requests a 512-byte ATT MTU and sends each buffer in 512-byte writes with response (10 writes per plane: 9 full chunks and a final 128-byte chunk). Values longer than MTU − 3 bytes go out as BLE long writes, which the ESP32 BLE stack reassembles before calling `onWrite`.

---

## State Machine Diagram

```
                     +----------+
      power on /     |          |
      wake-up -----> |   IDLE   |  <----+
                     | (advert) |       |
                     +----+-----+       |
                          |             |
                     "BEGIN"            |
                          |             |
                     +----v------+      |
                     | TRANSFER  |      |
                     |  STARTED  |      |
                     +----+------+      |
                          |             |
            +-------------+----------+  |
            |                        |  |
     "MONO BUFFER"           "COLOR BUFFER"
            |                        |
      +-----v------+          +------v-----+
      | RX MONO    |          | RX COLOR   |
      | (chunks)   |          | (chunks)   |
      +-----+------+          +------+-----+
            |                        |
            +-------------+----------+
                          |
                        "END"
                          |
                     +----v------+
                     | WRITING   |
                     | TO DISP.  |
                     +----+------+
                          |
                        "DRAW"
                          |
                     +----v------+
                     | REFRESH   |  ------> IDLE
                     +-----------+

  "CLEAR" at any time ---> draw_clear() ---> IDLE
  5 min inactivity (ESP32-C3) ---> deep sleep
  Button press (GPIO 3) ---> wake-up ---> IDLE
```

---

## Error Handling

The protocol has no acknowledgements, length fields or checksums: the characteristic is write-only, the firmware has no way to send status back to the app, and it expects the client to send exactly one image's worth of data per plane (anything beyond the buffer is dropped). Delivery itself is reliable because every write is a BLE write with response.

What the firmware does guarantee:

- **Buffer pre-clearing**: `MONO BUFFER` and `COLOR BUFFER` commands reset the entire buffer to the default value (`0xFF`) before new data arrives, preventing stale pixels from a previous image.
- **Inactivity timer**: If BLE writes stop for 5 minutes (on ESP32-C3), the device enters deep sleep and resets all state on wake-up.
- **Continuous advertising**: The device restarts advertising on both connect and disconnect, so the app can reconnect without restarting the badge.
- **Offset tracking**: `mono_buffer_size` and `color_buffer_size` track the write position, so an image split across many writes is reassembled in order.
- **Bounds checking**: every chunk is clamped to the space left in the active buffer, so an oversized or malformed transfer cannot write past the end of a framebuffer.
