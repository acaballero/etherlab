#include "ILI9341_fb.h"
#include <stdint-gcc.h>
#include <string.h>
#include <math.h>

uint16_t BACK_COLOR = 0x0000;

ILI9341::ILI9341(SPI_HandleTypeDef *spi_port) : Display(spi_port) {
}

int16_t ILI9341::begin(void) {

    select();
    reset();
    HAL_Delay(10);

    // SOFTWARE RESET
    writeCommand(0x01);
    HAL_Delay(1000);

    // POWER CONTROL A
    writeCommand(0xCB);
    {
        uint8_t data[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
        writeData(data, sizeof(data));
    }

    // POWER CONTROL B
    writeCommand(0xCF);
    {
        uint8_t data[] = {0x00, 0xC1, 0x30};
        writeData(data, sizeof(data));
    }

    // DRIVER TIMING CONTROL A
    writeCommand(0xE8);
    {
        uint8_t data[] = {0x85, 0x00, 0x78};
        writeData(data, sizeof(data));
    }

    // DRIVER TIMING CONTROL B
    writeCommand(0xEA);
    {
        uint8_t data[] = {0x00, 0x00};
        writeData(data, sizeof(data));
    }

    // POWER ON SEQUENCE CONTROL
    writeCommand(0xED);
    {
        uint8_t data[] = {0x64, 0x03, 0x12, 0x81};
        writeData(data, sizeof(data));
    }

    // PUMP RATIO CONTROL
    writeCommand(0xF7);
    {
        uint8_t data[] = {0x20};
        writeData(data, sizeof(data));
    }

    // POWER CONTROL,VRH[5:0]
    writeCommand(0xC0);
    {
        uint8_t data[] = {0x23};
        writeData(data, sizeof(data));
    }

    // POWER CONTROL,SAP[2:0];BT[3:0]
    writeCommand(0xC1);
    {
        uint8_t data[] = {0x10};
        writeData(data, sizeof(data));
    }

    // VCM CONTROL
    writeCommand(0xC5);
    {
        uint8_t data[] = {0x3E, 0x28};
        writeData(data, sizeof(data));
    }

    // VCM CONTROL 2
    writeCommand(0xC7);
    {
        uint8_t data[] = {0x86};
        writeData(data, sizeof(data));
    }

    // MEMORY ACCESS CONTROL
    writeCommand(0x36);
    {
        uint8_t data[] = {0x48};
        writeData(data, sizeof(data));
    }

    // PIXEL FORMAT
    writeCommand(0x3A);
    {
        uint8_t data[] = {0x55};
        writeData(data, sizeof(data));
    }

    // FRAME RATIO CONTROL, STANDARD RGB COLOR
    writeCommand(0xB1);
    {
        uint8_t data[] = {0x00, 0x18};
        writeData(data, sizeof(data));
    }

    // DISPLAY FUNCTION CONTROL
    writeCommand(0xB6);
    {
        uint8_t data[] = {0x08, 0x82, 0x27};
        writeData(data, sizeof(data));
    }

    // 3GAMMA FUNCTION DISABLE
    writeCommand(0xF2);
    {
        uint8_t data[] = {0x00};
        writeData(data, sizeof(data));
    }

    // GAMMA CURVE SELECTED
    writeCommand(0x26);
    {
        uint8_t data[] = {0x01};
        writeData(data, sizeof(data));
    }

    // POSITIVE GAMMA CORRECTION
    writeCommand(0xE0);
    {
        uint8_t data[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                          0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
        writeData(data, sizeof(data));
    }

    // NEGATIVE GAMMA CORRECTION
    writeCommand(0xE1);
    {
        uint8_t data[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                          0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
        writeData(data, sizeof(data));
    }

    // EXIT SLEEP
    writeCommand(0x11);
    HAL_Delay(120);

    // TURN ON DISPLAY
    writeCommand(0x29);
    HAL_Delay(100);

    // MADCTL
    writeCommand(0x36);
    {
        uint8_t data[] = {ILI9341_ROTATION};
        writeData(data, sizeof(data));
    }

    // Backlight on
    HAL_GPIO_WritePin(ILI9341_LED_PORT, ILI9341_LED_PIN, GPIO_PIN_RESET);

    // GRAM DATA
    //writeCommand(0x2C);

    // this->unselect();

    return 0;
}

int16_t ILI9341::stop() {
    writeCommand(0x10); // Enter sleep mode
    HAL_GPIO_WritePin(ILI9341_LED_PORT, ILI9341_LED_PIN, GPIO_PIN_SET); // Backlight off
}

void ILI9341::select() {
    HAL_GPIO_WritePin(ILI9341_CS_PORT, ILI9341_CS_PIN, GPIO_PIN_RESET);
}

void ILI9341::unselect() {
    HAL_GPIO_WritePin(ILI9341_CS_PORT, ILI9341_CS_PIN, GPIO_PIN_SET);
}


void ILI9341::reset() {
    HAL_GPIO_WritePin(ILI9341_RST_PORT, ILI9341_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(ILI9341_RST_PORT, ILI9341_RST_PIN, GPIO_PIN_SET);
}

void ILI9341::InitDisplayDataTransfer() {
    HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ILI9341_CS_PORT, ILI9341_CS_PIN, GPIO_PIN_RESET);
}

void ILI9341::EndDisplayDataTransfer() {
    HAL_GPIO_WritePin(ILI9341_CS_PORT, ILI9341_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_RESET);
}

uint16_t ILI9341::getPixel(uint16_t x, uint16_t y) {

    uint16_t c;

    // column address set
    writeCommand(0x2A); // CASET
    {
        uint8_t data[] = {static_cast<uint8_t>((x >> 8) & 0xFF), static_cast<uint8_t>(x & 0xFF), static_cast<uint8_t>((x >> 8) & 0xFF), static_cast<uint8_t>(x & 0xFF)};
        writeData(data, sizeof(data));
    }

    // row address set
    writeCommand(0x2B); // RASET
    {
        uint8_t data[] = {static_cast<uint8_t>((y >> 8) & 0xFF), static_cast<uint8_t>(y & 0xFF), static_cast<uint8_t>((y >> 8) & 0xFF), static_cast<uint8_t>(y & 0xFF)};
        writeData(data, sizeof(data));
    }

    // write to RAM
    writeCommand(0x2E); // Read data

    InitDisplayDataTransfer();
    //  uint8_t data[] = { 0};
    // writeData(data, sizeof(data));

    HAL_StatusTypeDef ret;

    ret = HAL_SPI_Receive(spi_port, (uint8_t *) &c, 4, 100);
    ret = HAL_SPI_Receive(spi_port, (uint8_t *) &c, 4, 100);
    ret = HAL_SPI_Receive(spi_port, (uint8_t *) &c, 4, 100);

    EndDisplayDataTransfer();

    return ret;
}


void ILI9341::writeCommand(uint8_t cmd) {

    HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_RESET);
    select();
    HAL_SPI_Transmit(spi_port, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    unselect();
}


void ILI9341::writeData(uint8_t *buff, size_t buff_size) {

    HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_SET);

    select();
    // split data in small chunks because HAL can't send more then 64K at once
    while (buff_size > 0) {

        uint16_t chunk_size = buff_size > 32768 ? 32768 : buff_size;
        HAL_SPI_Transmit(spi_port, buff, chunk_size, HAL_MAX_DELAY);
        buff += chunk_size;
        buff_size -= chunk_size;
    }
    unselect();
}

void ILI9341::setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {

    // column address set
    writeCommand(0x2A); // CASET
    {
        uint8_t data[] = {static_cast<uint8_t>((x0 >> 8) & 0xFF), static_cast<uint8_t>(x0 & 0xFF), static_cast<uint8_t>((x1 >> 8) & 0xFF), static_cast<uint8_t>(x1 & 0xFF)};
        writeData(data, sizeof(data));
    }

    // row address set
    writeCommand(0x2B); // RASET
    {
        uint8_t data[] = {static_cast<uint8_t>((y0 >> 8) & 0xFF), static_cast<uint8_t>(y0 & 0xFF), static_cast<uint8_t>((y1 >> 8) & 0xFF), static_cast<uint8_t>(y1 & 0xFF)};
        writeData(data, sizeof(data));
    }

    // write to RAM
    writeCommand(0x2C); // RAMWR
}






