//
// Created by Angel Dust on 29/06/2024.
//

#ifndef TRX_FRONTEND_AGC_H
#define TRX_FRONTEND_AGC_H

#include "Signal.h"

namespace agc {
extern Signal signal;
extern float agc_voltage;

float get_agc();
int get_gain();
bool is_overload();
void loop();
} // namespace agc

#endif // TRX_FRONTEND_AGC_H
