//
// Created by Angel Dust on 29/06/2022.
//

#include "periodic_task.h"
#include "hw/stm32_hal.h"

void periodic_task::loop() {

    if (_period_ms) {
        unsigned long m = HAL_GetTick();

        if (m - _last_ms > _period_ms) {
            _callback();
            _last_ms = m;
        }
    }
}
