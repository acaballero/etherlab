//
// Created by Angel Dust on 05/04/2021.
//

#ifndef TRX_FRONTEND_DSP_CONFIG_H
#define TRX_FRONTEND_DSP_CONFIG_H

#include <cstdio>
#include "dsp_common.h"

namespace dsp {

struct st_test_signal_params {
    int8_t pulse_duty = 50;
    uint32_t baseband_frequency = 1000;
    uint32_t modulation_frequency = 1000;
};

struct st_dsp_config {
    int8_t gain = DSP_MIN_TX_GAIN_DB;
    st_test_signal_params test_signal;
};

extern st_dsp_config config;

void set_config(st_dsp_config &);
st_dsp_config get_config();
} // namespace dsp

#endif // TRX_FRONTEND_DSP_CONFIG_H
