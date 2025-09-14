//
// Created by Angel Dust on 03/07/2024.
//

#ifndef TRX_FRONTEND_OUTPUT_H
#define TRX_FRONTEND_OUTPUT_H

#include "dsp/dsp_common.h"
#include "dsp/buffer.hpp"

template <typename T = complex_t, typename = std::enable_if_t<std::is_same<T, complex_t_f32>::value || std::is_same<T, complex_t>::value>> class Output {

  public:
    using SampleType = typename std::conditional<std::is_same<T, complex_t>::value, adc_type, float32_t>::type;

    virtual void get_block(buffer_t<T> &buff) = 0;
    virtual void get_complex_sample(T &sample) = 0;
    virtual void get_sample(SampleType &sample) = 0;

    void set_gain_db(int g) {
        gain_db = g;
        gain_factor = powf(10.0f, g / 20.0f);
    }
    int get_gain_db() {
        return gain_db;
    }

  protected:
    int gain_db{0};
    float gain_factor{1.0f};
};

#endif // TRX_FRONTEND_OUTPUT_H
