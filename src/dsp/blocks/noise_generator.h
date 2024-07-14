//
// Created by Angel Dust on 02/07/2024.
//

#ifndef TRX_FRONTEND_NOISE_GENERATOR_H
#define TRX_FRONTEND_NOISE_GENERATOR_H

#include "stdio.h"
#include "dsp/buffer.hpp"
#include "output.h"

class NoiseGenerator : public Output {

public:

    NoiseGenerator() {};

    void get_block(buffer_t<complex_t> &buff) override;
    void get_complex_sample(complex_t &sample) override;
    void get_sample(adc_type &sample) override;

protected:
    adc_type get_sample();

    // Noise "random" number and feedback
    uint32_t noise_seed{0x54DF0119};
    uint32_t feedback{};

    //uint32_t sample_count{0};
    //bool auto_off{};
};


#endif //TRX_FRONTEND_NOISE_GENERATOR_H
