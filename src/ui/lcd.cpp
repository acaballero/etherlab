//
// Created by Angel Dust on 18/04/2021.
//

#include "lcd.h"
#include "../../lib/ST77XX-STM32/st7789_fb.h"

extern SPI_HandleTypeDef LCD_SPI_HANDLE;

ST7789 lcd(&LCD_SPI_HANDLE);

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

void lcd_sleep() {
    lcd.stop();
}
