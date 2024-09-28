//
// Created by Angel Dust on 18/04/2021.
//

#ifndef TRX_FRONTEND_LCD_H
#define TRX_FRONTEND_LCD_H

#include "hw/hw_config.h"
#include "../../lib/ST77XX-STM32/st7789_fb.h"

#if LCD_ENABLED

extern ST7789 lcd;

#endif

void lcd_init();
void lcd_sleep();

#endif //TRX_FRONTEND_LCD_H
