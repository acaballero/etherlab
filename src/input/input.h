//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_INPUT_H
#define TRX_FRONTEND_INPUT_H

#include "../../lib/InputPin/GPIOInputPin.h"

extern GPIOInputPin TouchPanelInterruptPin;
extern GPIOInputPin FrontPanelInterruptPin;
extern GPIOInputPin BackBtnInputPin;
void touchPanelInterruptCallback();
void backBtnInterruptCallback();
void front_panel_interrupt_callback();
extern int8_t last_pressed_button_id;

#endif // TRX_FRONTEND_INPUT_H
