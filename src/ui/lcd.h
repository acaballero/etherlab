//
// Created by Angel Dust on 18/04/2021.
//

#ifndef TRX_FRONTEND_LCD_H
#define TRX_FRONTEND_LCD_H

#include "hw/hw_config.h"

#if LCD_ENABLED

#include "../../lib/ST77XX-STM32/ILI9341_fb.h"
#include "../../lib/ST77XX-STM32/XPT2046_touch.h"
extern ILI9341 lcd;

#endif

void lcd_init();
void lcd_sleep();

#endif //TRX_FRONTEND_LCD_H
