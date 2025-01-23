#ifndef __ILI9341_FB_H
#define __ILI9341_FB_H

// landscape

// default orientation
/*
#define DISPLAY_X_PIXELS  240
#define DISPLAY_Y_PIXELS 320
#define ILI9341_ROTATION (ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR)
*/
// rotate right

#define ILI9341_CMD 0
#define ILI9341_DATA 1

#define ILI9341_MADCTL_MY 0x80
#define ILI9341_MADCTL_MX 0x40
#define ILI9341_MADCTL_MV 0x20
#define ILI9341_MADCTL_ML 0x10
#define ILI9341_MADCTL_RGB 0x00
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH 0x04

#ifndef DISPLAY_X_PIXELS
#define DISPLAY_X_PIXELS 320
#endif
#ifndef DISPLAY_Y_PIXELS
#define DISPLAY_Y_PIXELS 240
#endif
#define ILI9341_ROTATION (ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR) // 90º
//#define ILI9341_ROTATION (ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR)  // 270º

// rotate left
/*
#define DISPLAY_X_PIXELS  320
#define DISPLAY_Y_PIXELS 240
#define ILI9341_ROTATION (ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR)
*/

// upside down
/*
#define DISPLAY_X_PIXELS  240
#define DISPLAY_Y_PIXELS 320
#define ILI9341_ROTATION (ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR)
*/

#include "hw/stm32.h"
#include <stdint.h>
#include <string.h>
#include "ips_font.h"
#include "Display_afb.h"

#define ILI9341_RST_PORT DISP_RST_PORT
#define ILI9341_RST_PIN DISP_RST_PIN
#define ILI9341_DC_PORT DISP_DC_PORT
#define ILI9341_DC_PIN DISP_DC_PIN
#define ILI9341_CS_PORT DISP_CE_PORT
#define ILI9341_CS_PIN DISP_CE_PIN
#define ILI9341_LED_PORT DISP_LED_PORT
#define ILI9341_LED_PIN DISP_LED_PIN

/*****Use if need backlight control*****
#define BLK_PORT
#define BLK_PIN
***************************************/

#define USING_HORIZONAL 2

#define ILI9341_RST_Clr() HAL_GPIO_WritePin(ILI9341_RST_PORT, ILI9341_RST_PIN, GPIO_PIN_RESET)
#define ILI9341_RST_Set() HAL_GPIO_WritePin(ILI9341_RST_PORT, ILI9341_RST_PIN, GPIO_PIN_SET)

#define ILI9341_DC_Clr() HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_RESET)
#define ILI9341_DC_Set() HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_SET)

#define ILI9341_CS_Clr() HAL_GPIO_WritePin(ILI9341_CS_PORT, ILI9341_CS_PIN, GPIO_PIN_RESET)
#define ILI9341_CS_Set() HAL_GPIO_WritePin(ILI9341_CS_PORT, ILI9341_CS_PIN, GPIO_PIN_SET)

class ILI9341 : public Display {

  public:
    ILI9341(SPI_HandleTypeDef *);

    int16_t begin();

    int16_t stop();

    void select();

    void unselect();

    void reset();

    uint16_t getPixel(uint16_t x, uint16_t y);

  private:
    // void init(void);
    void writeCommand(uint8_t data);

    void writeData(uint8_t *buff, size_t buff_size);

    void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

    virtual void InitDisplayDataTransfer();

    virtual void EndDisplayDataTransfer();
};

#endif
