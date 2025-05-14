//
// Created by Angel Dust on 02/07/2024.
//

#ifndef TRX_FRONTEND_SIGNAL_GENERATOR_H
#define TRX_FRONTEND_SIGNAL_GENERATOR_H

#include "dsp/dsp_common.h"
#include "stdio.h"
#include "dsp/buffer.hpp"
#include "output.h"

enum SIGNAL_SHAPE { SIGNAL_SHAPE_SIN, SIGNAL_SHAPE_SAW_UP, SIGNAL_SHAPE_SAW_DOWN, SIGNAL_SHAPE_TRI };

class SignalGenerator : public Output<complex_t> {

  public:
    void init();

    SignalGenerator() {
        SignalGenerator(1000, 1000, SIGNAL_SHAPE_SIN);
    };

    SignalGenerator(uint32_t f, uint32_t sr, SIGNAL_SHAPE shape) : tone_shape{shape}, frequency{f}, sample_rate(sr) {
        init();
    };

    void set_config(uint32_t frequency, uint32_t sample_rate);

    void set_shape(SIGNAL_SHAPE shape);

    void get_block(buffer_t<complex_t> &buff) override;

    void get_complex_sample(complex_t &sample) override;

    void get_sample(adc_type &sample) override;

  protected:
    adc_type get_sample(uint32_t phase);

    // All 32 bit values are fixed point 8/24

    uint32_t tone_delta{0};

    // FM phase
    // uint32_t fm_delta{};

    SIGNAL_SHAPE tone_shape{SIGNAL_SHAPE_SIN};

    // uint32_t sample_count{0};
    // bool auto_off{};

    uint32_t frequency{0};
    uint32_t sample_rate{0};
    uint32_t tone_phase{0};

    // uint32_t phase{0};
    // uint32_t delta{0};
    // uint32_t sphase{0};
};

#endif // TRX_FRONTEND_SIGNAL_GENERATOR_H
