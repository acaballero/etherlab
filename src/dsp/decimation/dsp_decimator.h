//
// Created by Angel Dust on 20/04/2021.
//

#ifndef TRX_FRONTEND_DSP_DECIMATOR_H
#define TRX_FRONTEND_DSP_DECIMATOR_H

#include <stdio.h>
#include <stddef.h>
#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"

template <typename T> class DspDecimator {

  public:
    DspDecimator(const uint16_t factor) : factor(factor){};

    DspDecimator(uint32_t input_rate, uint32_t output_rate, uint16_t factor)
        : input_rate(input_rate), output_rate(output_rate), factor(factor){

                                                            };

    virtual void decimate(buffer_t<T> &src, buffer_t<T> &dst) = 0;

    uint16_t getFactor() const;

    void setFactor(uint16_t factor);

    uint32_t getInputRate() const;

    void setInputRate(uint32_t inputRate);

    uint32_t getOutputRate() const;

    void setOutputRate(uint32_t outputRate);

  protected:
    uint32_t input_rate;
    uint32_t output_rate;
    uint16_t factor;
};

#endif // TRX_FRONTEND_DSP_DECIMATOR_H
