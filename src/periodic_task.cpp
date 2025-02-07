//
// Created by Angel Dust on 29/06/2022.
//

#include "periodic_task.h"
#include "printf.h"
#include "status.h"
#include "utils.hpp"
#include "hw/stm32_hal.h"

void periodic_task::set_enabled(bool b) { enabled = b; }

void periodic_task::loop() {
    if (_period_ms && enabled) {
        unsigned long ms = HAL_GetTick();

        if (ms >= _next_ms) {

            _callback();

            // float instant_rate = 1000.0 / (float)(ms - _last_ms);
            //_rate = _rate - (0.1 * (_rate - instant_rate));

            _last_ms = ms;

            uint64_t next = _next_ms + _period_ms;
            ms = HAL_GetTick();
            _next_ms = next > ms ? next : ms + _period_ms;

            //  float expected_rate = 1000.0 / (float)_period_ms;
            // float difference = expected_rate - _rate;
            // char buff[200];
            // char tmp[20], tmp1[20], tmp2[20];
            // sprintf(buff, "%s: Executed task: %s, period: %s (%.2f), next: %s, rate: %.2f/%.2f (%.2f)\n", format_long(ms, tmp), "S",
            //         format_long(_period_ms, tmp1), expected_rate, format_long(_next_ms, tmp2), instant_rate, _rate, difference);
            // printf_(buff);
        }
    }
}
