//
// Created by Angel Dust on 02/07/2024.
//

#ifndef TRX_FRONTEND_PULSE_GENERATOR_H
#define TRX_FRONTEND_PULSE_GENERATOR_H

#include "dsp/dsp_common.h"
#include "stdio.h"
#include "dsp/buffer.hpp"
#include "output.h"
#include "blocks_common.h"

class PulseGenerator : public Output<complex_t> {

  public:
    PulseGenerator() {
        PulseGenerator(1000, 1000);
    };

    PulseGenerator(uint32_t f, uint32_t sr) : frequency{f}, sample_rate(sr) {
        init();
    };

    void set_config(uint32_t frequency, uint32_t sample_rate);

    void set_duty(uint8_t duty);

    void get_block(buffer_t<complex_t> &buff) override;

    void get_complex_sample(complex_t &sample) override;

    void get_sample(adc_type &sample) override;

  protected:
    adc_type get_sample(uint32_t phase);

    void init();

    // All 32 bit values are fixed point 8/24

    uint32_t tone_delta{0};

    // uint32_t sample_count{0};
    // bool auto_off{};

    uint32_t frequency{0};
    uint32_t sample_rate{0};
    uint32_t tone_phase{0};

    uint8_t duty{50};
    uint8_t crossover_phase;
    // uint32_t phase{0};
    // uint32_t delta{0};
    // uint32_t sphase{0};
};

#endif // TRX_FRONTEND_PULSE_GENERATOR_H
