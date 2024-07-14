//
// Created by Angel Dust on 18/04/2021.
//

#include "lcd.h"

extern SPI_HandleTypeDef LCD_SPI_HANDLE;

ILI9341 lcd(&LCD_SPI_HANDLE);

void lcd_init() {

#if LCD_ENABLED

    int8_t ret = lcd.begin();

    if (ret < 0) {

#if DEBUG
        debug_print("Error initializing display");
#endif

    }

#endif
}

void lcd_sleep()
{
    lcd.stop();
}
