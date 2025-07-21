//
// Created by Angel Dust on 20/04/2021.
//

#ifndef TRX_FRONTEND_DSP_DECIMATOR_H
#define TRX_FRONTEND_DSP_DECIMATOR_H

#include <stdio.h>
#include <stddef.h>
#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"
#include "dsp/fir_filter.h"

template <typename T> class DspDecimator {

  public:
    DspDecimator(const uint16_t factor) : factor(factor){};

    DspDecimator(uint32_t input_rate, uint32_t bandwidth, uint16_t factor) : input_rate(input_rate), bandwidth(bandwidth), factor(factor){};

    virtual ~DspDecimator() = default;

    virtual bool config(uint32_t input_rate, uint32_t bandwidth, uint16_t factor, uint32_t start_frequency = 0) = 0;
    virtual void decimate(T *src_i, T *src_q, T *dst_i, T *dst_q, size_t n_samples) = 0;
    virtual void decimate(buffer_t<T> &src, buffer_t<T> &dst) = 0;

    uint16_t get_factor() const {
        return factor;
    };

    virtual void set_factor(uint16_t factor);

    uint32_t get_input_rate() const;

    void set_input_rate(uint32_t input_rate);

    uint32_t get_bandwidth() const;

    void set_bandwidth(uint32_t output_rate);

  protected:
    uint32_t input_rate;

    // -3 dB frequency (not the output rate)
    uint32_t bandwidth;
    uint16_t factor;
};

#endif // TRX_FRONTEND_DSP_DECIMATOR_H
