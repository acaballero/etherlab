//
// Created by Angel Dust on 29/06/2024.
//

#ifndef TRX_FRONTEND_AGC_H
#define TRX_FRONTEND_AGC_H

#include "Signal.h"
#include "os/periodic_task.h"

namespace agc {
extern Signal signal_agc_voltage;
extern Signal signal_gain;
extern float agc_voltage;

float get_agc();
int get_gain();
int get_analog_gain();
bool is_overload();
extern os::periodic_task task;
void reset();
void set_release_ms(uint32_t v = 0);
void set_attack_ms(uint32_t v = 0);
} // namespace agc

#endif // TRX_FRONTEND_AGC_H
