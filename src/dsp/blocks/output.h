//
// Created by Angel Dust on 03/07/2024.
//

#ifndef TRX_FRONTEND_OUTPUT_H
#define TRX_FRONTEND_OUTPUT_H


#include "dsp/dsp_common.h"
#include "dsp/buffer.hpp"

class Output {

public:
    virtual void get_block(buffer_t<complex_t> &buff) = 0;
    virtual void get_complex_sample(complex_t &sample) = 0;
    virtual void get_sample(adc_type &sample) = 0;
};


#endif //TRX_FRONTEND_OUTPUT_H
