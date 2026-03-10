# Image Processing Pipeline

This document describes how PixelPin converts a full-color RGB565 image into the two 1-bit framebuffers required by the 2.9-inch tri-color e-paper display.

The relevant source is `src/epaper.cpp` → `rgb565_to_buffer()`.

---

## Overview

The e-paper display supports exactly three colors: **black**, **white**, and **red**. The firmware takes RGB565 pixel data (as received from the companion app), determines the best match for each pixel among these three options, and packs the results into two separate 1-bit buffers.

```
  RGB565 input
  (2 bytes/pixel)
       |
       v
  +-----------+
  | Decode    |  Extract 8-bit R, G, B from 16-bit RGB565
  +-----------+
       |
       v
  +-----------+
  | Luminance |  Is the pixel bright enough to be white?
  |   check   |  luma = R*0.299 + G*0.587 + B*0.114 > 128
  +-----------+
       |
     whitish?
      /    \
    yes     no
     |       |
  white   +--v--------+
           | Hue check |  Is the pixel red or yellow?
           +-----------+
                |
           colored?
            /      \
          yes        no
           |          |
          red        black

       |               |               |
       v               v               v
     WHITE            RED            BLACK
  mono bit=1       mono bit=0      mono bit=0
 color bit=1      color bit=0     color bit=1

    8 pixels packed into 1 byte, MSB first
```

---

## RGB565 Decoding

RGB565 stores one pixel in 16 bits: 5 bits red, 6 bits green, 5 bits blue. The firmware receives the pixel as two bytes and decodes them:

```
  Byte layout (little-endian, as received over BLE):

  LSB (first byte):  [B4 B3 B2 B1 B0 | G2 G1 G0]
  MSB (second byte): [R4 R3 R2 R1 R0 | G5 G4 G3]

  Blue  = (LSB & 0x1F) << 3          // 5 bits → upper 5 bits of uint8_t
  Green = ((MSB & 0x07) << 5)        // G5-G3 → bits 7-5
        | ((LSB & 0xE0) >> 3)        // G2-G0 → bits 4-2  (combined = 6 bits)
  Red   = (MSB & 0xF8)               // 5 bits in positions 7-3 of uint8_t
```

After decoding, each channel is in the range `[0, 248]` (the lower 3 bits of each 8-bit value are zero due to the left-shift, which is an acceptable loss of precision for a 3-color display).

---

## 3-Color Quantization

### Step 1: Luminance (White Detection)

The firmware computes a standard luminance value using the BT.601 coefficients and compares it to a threshold of 128:

```
luma = R * 0.299 + G * 0.587 + B * 0.114

whitish = (luma > 128)
```

If `whitish` is true, the pixel is mapped to **white** and no further checks are performed.

### Step 2: Hue (Red/Yellow Detection)

If the pixel is not white, the firmware checks whether it is reddish or yellowish:

```
colored = (R > 128)
          AND (
               (R > G + 64  AND  R > B + 64)   -- clearly red-dominant
               OR
               (R + 16 > G + B)                 -- red dominates sum
          )
          OR
          (G > 200  AND  R > 200  AND  B < 64)  -- yellow (high R and G, low B)
```

If `colored` is true, the pixel is mapped to **red** (the e-paper's third color). The yellow detection exists because warm yellows look better rendered as red than as white or black on the display.

If neither `whitish` nor `colored` is true, the pixel is mapped to **black**.

---

## Bit Packing

The e-paper display uses 1 bit per pixel in each buffer:

- **Mono buffer**: `0` = black or red (not white), `1` = white. Pre-filled with `0xFF`.
- **Color buffer**: `0` = red (colored), `1` = not colored. Pre-filled with `0xFF`.

The mapping:

| Pixel color | mono bit | color bit |
|---|---|---|
| White | 1 (keep 0xFF default) | 1 (keep 0xFF default) |
| Red   | 0 (clear bit) | 0 (clear bit) |
| Black | 0 (clear bit) | 1 (keep 0xFF default) |

Bits are packed 8 per byte, MSB first. For column `col` within a row, the bit position within the current byte is `7 - (col % 8)`, which corresponds to the mask `0x80 >> (col % 8)`.

```c
// Set mono bit to 0 for black pixels
out_byte &= ~(0x80 >> col % 8);

// Set color bit to 0 for red pixels
out_color_byte &= ~(0x80 >> col % 8);
```

After every 8 columns (or at the last column of the row if `width % 8 != 0`), the accumulated byte is written to the buffer at the correct position:

```c
mono_buffer[(row + y) * MAX_COL / 8 + out_col_idx + x] = out_byte;
color_buffer[(row + y) * MAX_COL / 8 + out_col_idx + x] = out_color_byte;
```

The buffers use row-major order: row `r`, column group `c` maps to byte index `r * (MAX_COL / 8) + c`.

---

## Floyd-Steinberg Dithering

The companion app (Flutter, `pixelpin-app`) applies Floyd-Steinberg dithering to the image before encoding it as RGB565 and sending it over BLE. The firmware's `rgb565_to_buffer()` function then performs the 3-color quantization described above.

Floyd-Steinberg dithering is an error-diffusion algorithm that distributes the quantization error of each pixel to its neighbors, producing a perceptual impression of more colors than are actually available.

```
  For each pixel (left to right, top to bottom):
    1. Quantize pixel to nearest of {black, white, red}
    2. Compute error = original_value - quantized_value
    3. Distribute error to neighbors:

          [ current ]  -->  [ + 7/16 error ]
    [ + 3/16 error ]  [ + 5/16 error ]  [ + 1/16 error ]

  Pixel traversal order:

    +---+---+---+---+---+
    | * | 7 |   |   |   |     * = current pixel
    +---+---+---+---+---+     7 = receives 7/16 of error
    | 3 | 5 | 1 |   |   |
    +---+---+---+---+---+
```

The error is computed and distributed separately for each color channel (R, G, B) before the final RGB565 encoding, so the dithering operates in full-color space rather than after quantization.

---

## Buffer Sizes

| Parameter | Value |
|---|---|
| Display width | 128 pixels |
| Display height | 296 pixels |
| Bits per pixel (each buffer) | 1 |
| Mono buffer size | 128 × 296 / 8 = **4,736 bytes** |
| Color buffer size | 128 × 296 / 8 = **4,736 bytes** |
| Total framebuffer memory | **9,472 bytes** |

---

## Performance on ESP32

`rgb565_to_buffer()` processes each pixel with:

- 2 byte reads from the input buffer
- 3 bitwise arithmetic operations for channel extraction
- 1 floating-point multiply-accumulate (luminance)
- 2–3 integer comparisons (color detection)
- 1–2 bitwise AND/NOT operations (bit packing)
- 1 byte write per 8 pixels

For a full 128×296 image (37,888 pixels), this is approximately 37,888 iterations with lightweight operations per iteration. On the ESP32-C3 at 160 MHz, the conversion completes well within the BLE connection timeout window.

The buffers are pre-cleared with `memset` (a single `0xFF` write across 4,736 bytes) at the start of each `MONO BUFFER` / `COLOR BUFFER` command, which is faster than clearing individual pixels during conversion.
