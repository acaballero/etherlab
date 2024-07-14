//
// Created by Angel Dust on 21/06/2024.
//

#include "power.h"
#include "hw/stm32.h"

void hal_sleep() {
    //HAL_SuspendTick();
    //HAL_PWR_EnableSleepOnExit();
    //HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

void hal_wakeup() {
    //SystemClock_Config();
    //HAL_ResumeTick();
}