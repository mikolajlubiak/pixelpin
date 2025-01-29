#include "ble.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include <Arduino.h>

#include <PNGdec.h>

#include "buffer.h"
#include "draw.h"
#include "epaper.h"

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

size_t ble_data_alloc = 256;
uint8_t *ble_data = (uint8_t *)malloc(ble_data_alloc);
size_t ble_data_size = 0;

bool began = false;
bool png_decoder = false;

PNG png;

BLECharacteristic *pCharacteristic;

void ble_init()
{
    BLEDevice::init("edown");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ |
                                 BLECharacteristic::PROPERTY_WRITE |
                                 BLECharacteristic::PROPERTY_INDICATE);

    BLEDescriptor descriptor = BLEUUID((uint16_t)0x2902);
    pCharacteristic->addDescriptor(&descriptor);
    pCharacteristic->setCallbacks(new ble_characteristics_callbacks());
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(
        0x06); // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
}

std::string ble_get_value() { return pCharacteristic->getValue(); }

uint8_t *ble_get_data() { return pCharacteristic->getData(); }

size_t ble_get_len() { return pCharacteristic->getLength(); }

void ble_draw(PNGDRAW *pDraw)
{
#ifdef DEBUG
    Serial.println("WRITING DRAW DATA");
    Serial.printf("Width: %d, Y: %d\n", pDraw->iWidth, pDraw->y);
#endif

    uint16_t *usPixels = (uint16_t *)malloc(2 * 128);

    png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);

    uint16_t in_idx = 0;
    uint16_t red, green, blue;
    bool whitish = false;
    bool colored = false;
    bool with_color = true;
    uint8_t out_byte = 0xFF;       // white (for w%8!=0 border)
    uint8_t out_color_byte = 0xFF; // white (for w%8!=0 border)
    uint16_t out_idx_col = 0;

    for (uint16_t col = 0; col < pDraw->iWidth; col++)
    {

        uint8_t lsb = ((uint8_t *)usPixels)[in_idx++];
        uint8_t msb = ((uint8_t *)usPixels)[in_idx++];
        blue = (lsb & 0x1F) << 3;
        green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
        red = (msb & 0xF8);
        whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80))
                             : ((red + green + blue) > 3 * 0x80); // whitish
        colored = (red > 0xF0) ||
                  ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
        if (whitish)
        {
            // keep white
        }
        else if (colored && with_color)
        {
            out_color_byte &= ~(0x80 >> col % 8); // colored
        }
        else
        {
            out_byte &= ~(0x80 >> col % 8); // black
        }
        if ((7 == col % 8) ||
            (col == pDraw->iWidth - 1)) // write that last byte! (for w%8!=0 border)
        {
            output_color_buffer[0][out_idx_col] = out_color_byte;
            output_mono_buffer[0][out_idx_col++] = out_byte;
            out_byte = 0xFF;       // white (for w%8!=0 border)
            out_color_byte = 0xFF; // white (for w%8!=0 border)
        }
    }

    epaper_write(output_mono_buffer[0], output_color_buffer[0], pDraw->iWidth, 1,
                 0, pDraw->y);
}

void ble_characteristics_callbacks::onWrite(
    BLECharacteristic *pCharacteristic)
{
    if (pCharacteristic->getLength() < 10)
    {
        if (memcmp(pCharacteristic->getValue().c_str(), "BEGIN", strlen("BEGIN")) ==
            0)
        {
            Serial.println("BEGIN");
            began = true;
        }
        else if (memcmp(pCharacteristic->getValue().c_str(), "PNG",
                        strlen("PNG")) == 0 &&
                 began)
        {
            Serial.println("PNG");
            png_decoder = true;
        }
        else if (memcmp(pCharacteristic->getValue().c_str(), "DRAW",
                        strlen("DRAW")) == 0)
        {
            Serial.println("DRAW");
            epaper_refresh();
        }
        else if (memcmp(pCharacteristic->getValue().c_str(), "CLEAR",
                        strlen("CLEAR")) == 0)
        {
            Serial.println("CLEAR");
            epaper_clear();
        }
        else if (memcmp(pCharacteristic->getValue().c_str(), "END",
                        strlen("END")) == 0)
        {
            Serial.println("END");
            began = false;
            if (png_decoder)
            {
                if (png.openRAM(ble_data, ble_data_size, ble_draw) != PNG_SUCCESS)
                {
                    Serial.println("[ble_characteristics_callbacks::onWrite] - Error: "
                                   "png.openRAM failed");
                    return;
                }
                png.decode(NULL, 0);
                png.close();
                ble_data_alloc = 256;
                free(ble_data);
                ble_data = (uint8_t *)malloc(ble_data_alloc);

#ifdef DEBUG
                Serial.println("DECODING PNG");
                Serial.printf("Image specs: (%d x %d), %d bpp, pixel type: %d\n",
                              png.getWidth(), png.getHeight(), png.getBpp(),
                              png.getPixelType());
#endif
            }
        }
    }
    else
    {
        if (pCharacteristic->getLength() + ble_data_size > ble_data_alloc)
        {
            ble_data_alloc = (pCharacteristic->getLength() + ble_data_size) * 2;
            ble_data = (uint8_t *)realloc(ble_data, ble_data_alloc);

#ifdef DEBUG
            Serial.println("REALLOC");
            Serial.printf("New alloc: %zu\n", ble_data_alloc);
#endif
        }

        memcpy(ble_data + ble_data_size, pCharacteristic->getData(),
               pCharacteristic->getLength());
        ble_data_size += pCharacteristic->getLength();

#ifdef DEBUG
        Serial.println("COPYING MEMORY");
        Serial.printf("Length: %zu\n", pCharacteristic->getLength());
        Serial.printf("Data size: %zu\n", ble_data_size);
#endif
    }
}
