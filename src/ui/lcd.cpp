//
// Created by Angel Dust on 18/04/2021.
//

#include "lcd.h"
#include "../../lib/ST77XX-STM32/st7789_fb.h"
#include "os/task_manager.h"
#include "status.h"

extern SPI_HandleTypeDef LCD_SPI_HANDLE;

ST7789 lcd(&LCD_SPI_HANDLE);

void lcd_init() {

    LOG("LCD init\n");

#if LCD_ENABLED

    // Note this doesn't turn on the backlight yet (the view manager does when ready) to prevent a white screen for appearing before
    int8_t ret = lcd.begin();

    if (ret < 0) {
        LOG("Error initializing display");
    }

#endif
}

void lcd_sleep() {
    lcd.stop();
}
