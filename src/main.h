//
// Created by Angel Dust on 13/07/2017.
//

#ifndef TRX_FRONTEND_H
#define TRX_FRONTEND_H

#include "dsp/dsp.h"
#include "dsp/fft/fft.h"
#include "hw/stm32.h"
#include "hw/hw_config.h"
#include "types.h"
#include "radio.h"
#include "../lib/InputPin/MCP23017InputPin.h"
#include "../lib/InputPin/GPIOInputPin.h"
#include "ShiftReg.h"

#ifdef __cplusplus
extern "C" {
#endif

#if SWO_ENABLED
int _write(int, char *, int);
#endif

#if USB_ENABLED
void USB_LP_CAN_RX0_IRQHandler(void);
#endif

void TIM3_IRQHandler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

extern bool change_drive_strength;
extern bool change_calibration;

#endif
