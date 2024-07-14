//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_INPUT_H
#define TRX_FRONTEND_INPUT_H

#include "../../lib/InputPin/GPIOInputPin.h"

extern GPIOInputPin TouchPanelInterruptPin;
extern GPIOInputPin AnalogKeyBoardInterruptPin;
extern GPIOInputPin BackBtnInputPin;
void calibrateAnalogKeyboard();
void analogKeyboardInterruptCallback();
void touchPanelInterruptCallback();
void backBtnInterruptCallback();
extern int8_t analogKeyboardLastPressedButton;

#endif //TRX_FRONTEND_INPUT_H
