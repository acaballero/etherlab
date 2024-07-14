//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_ENCODER_H
#define TRX_FRONTEND_ENCODER_H

#include "../../lib/InputPin/GPIOInputPin.h"

void doEncoderA();

void doPushButton();

extern GPIOInputPin RotAInputPin;
extern GPIOInputPin RotBInputPin;
extern GPIOInputPin RotBtnInputPin;

#endif //TRX_FRONTEND_ENCODER_H
