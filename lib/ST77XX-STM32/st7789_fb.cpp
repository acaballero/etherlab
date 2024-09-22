#include "st7789_fb.h"
#include <stdint-gcc.h>
#include <string.h>
#include <math.h>
#include "hw/stm32_hal.h"

uint16_t BACK_COLOR = 0x0000;

ST7789::ST7789(SPI_HandleTypeDef *spi_port) : Display(spi_port) {


}

int16_t ST7789::begin(void) {

    select();

    ST7789_RST_Clr();
    HAL_Delay(10);
    ST7789_RST_Set();
    HAL_Delay(100);

    // Software reset
    writeCommand(ST7796S_SWRESET);
    HAL_Delay(100);

    writeCommand(ST7796S_MADCTL);
    {
        uint8_t data[] = {0x68};
        writeData(data, sizeof(data));
    }

    writeCommand(ST7796S_PIXFMT);
    {
        uint8_t data[] = {0x05}; // 16 bit/pixel
        writeData(data, sizeof(data));
    }

    // Interface mode control
    writeCommand(0xB0);
    {
        uint8_t data[] = {0x80}; // SPI
        writeData(data, sizeof(data));
    }

    // Display function
    writeCommand(0xB6);
    {
        uint8_t data[] = {0x20, 0x02, 0x3B};
        writeData(data, sizeof(data));
    }

    // Blanking Porch
    writeCommand(0xB5);
    {
        uint8_t data[] = {0x02, 0x03, 0x00, 0x04};
        writeData(data, sizeof(data));
    }

    writeCommand(ST7796S_FRMCTR1);
    {
        uint8_t data[] = {0x80, 0x10};
        writeData(data, sizeof(data));
    }

    writeCommand(ST7796S_INVCTR);
    {
        uint8_t data[] = {0x00};
        writeData(data, sizeof(data));
    }

    // Entry mode
    writeCommand(0xB7);
    {
        uint8_t data[] = {0xC6};
        writeData(data, sizeof(data));
    }

    writeCommand(ST7796S_VMCTR1);
    {
        uint8_t data[] = {0x24};
        writeData(data, sizeof(data));
    }

    // VCOM
    writeCommand(0xE4);
    {
        uint8_t data[] = {0x31};
        writeData(data, sizeof(data));
    }

    // Display output
    writeCommand(0xE0);
    {
        uint8_t data[] = {0x40, 0x8A, 0x00, 0x00, 0x24, 0x19, 0xA5, 0x33};
        writeData(data, sizeof(data));
    }

    // Power control 3
    writeCommand(ST7796S_PWCTR3);
    {
        uint8_t data[] = {0xA7};
        writeData(data, sizeof(data));
    }

    // Positive gamma
    writeCommand(ST7796S_GMCTRP1);
    {
        uint8_t data[] = {0xF0, 0x09, 0x13, 0x12, 0x12, 0x2B, 0x3C, 0x44, 0x4B, 0x1B, 0x18, 0x17, 0x1D, 0x21};
        writeData(data, sizeof(data));
    }

    // Negative gamma
    writeCommand(ST7796S_GMCTRN1);
    {
        uint8_t data[] = {0xF0, 0x09, 0x13, 0x0C, 0x0D, 0x27, 0x3B, 0x44, 0x4D, 0x0B, 0x17, 0x17, 0x1D, 0x21};
        writeData(data, sizeof(data));
    }

    writeCommand(ST7796S_MADCTL);
    {
        uint8_t data[] = {ST7796S_MAD_DATA_RIGHT_THEN_DOWN};
        writeData(data, sizeof(data));
    }

    HAL_Delay(100);
    writeCommand(ST7796S_NORON);
    writeCommand(ST7796S_INVOFF);
    writeCommand(ST7796S_SLPOUT);
    HAL_Delay(100);
    writeCommand(ST7796S_DISPON);
    HAL_Delay(100);

    // Backlight on
    HAL_GPIO_WritePin(ST7789_LED_PORT, ST7789_LED_PIN, GPIO_PIN_RESET);

    setAddressWindow(0,0,100,100);
    for(int i=0; i<100*100;i++)
    {
        uint8_t data[] = {0x00};
        writeData(data, sizeof(data));

    }

}

int16_t ST7789::stop() {
    writeCommand(0x10); // Enter sleep mode
    HAL_GPIO_WritePin(ST7789_LED_PORT, ST7789_LED_PIN, GPIO_PIN_SET); // Backlight off
}

void ST7789::select() {
    HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_RESET);
}


void ST7789::unselect() {
    HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_SET);
}


void ST7789::reset() {
    HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_SET);
}

void ST7789::InitDisplayDataTransfer() {
    HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_SET);
}

void ST7789::EndDisplayDataTransfer() {
    HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_RESET);
}


void ST7789::writeCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_RESET);
    select();
    HAL_SPI_Transmit(spi_port, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    //unselect();
}


void ST7789::writeData(uint8_t *buff, size_t buff_size) {

    HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_SET);
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


void ST7789::setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {

    writeCommand(ST7796S_CASET);
    { // CASET
        uint8_t data[] = {static_cast<uint8_t>(((x0 + 0) >> 8) & 0xFF), static_cast<uint8_t>((x0 + 0) & 0xFF),
                          static_cast<uint8_t>(((x1 + 0) >> 8) & 0xFF), static_cast<uint8_t>((x1 + 0) & 0xFF)};
        writeData(data, sizeof(data));
    }
    // row address set
    writeCommand(ST7796S_PASET);
    {// RASET
        uint8_t data[] = {static_cast<uint8_t>(((y0 + 0) >> 8) & 0xFF), static_cast<uint8_t>((y0 + 0) & 0xFF),
                          static_cast<uint8_t>(((y1 + 0) >> 8) & 0xFF), static_cast<uint8_t>((y1 + 0) & 0xFF)};
        writeData(data, sizeof(data));
    }
    // write to RAM
    writeCommand(ST7796S_RAMWR); // RAMWR


}






