//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_INPUT_CONTROLLER_H
#define TRX_FRONTEND_INPUT_CONTROLLER_H

#include "inputEvent.h"
#include "../../lib/InputPin/InputPinController.h"

/*
 * Dispatches the events in the interrupt callback
 */
#define DISPATCH_INMEDIATELY false
#define MAX_EVENTS_IN_QUEUE 3


extern InputPinController PinController;

void onInputEvent(st_inputEvent e);
void inputControllerInit();
void dispatchEvents();

#ifdef __cplusplus
extern "C" {
#endif

void TIM8_UP_TIM13_IRQHandler(void);

#ifdef __cplusplus
}
#endif


#endif //TRX_FRONTEND_INPUT_CONTROLLER_H
