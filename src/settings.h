//
// Created by Angel Dust on 01/11/2019.
//

#ifndef TRX_FRONTEND_SETTINGS_H
#define TRX_FRONTEND_SETTINGS_H

#include "config.h"
#include "hw/stm32.h"

uint8_t settings_read(Config *settings);
uint8_t settings_write(Config *settings);


#endif //TRX_FRONTEND_SETTINGS_H
