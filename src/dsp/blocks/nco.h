//
// Created by Angel Dust on 15/05/2025.
//

#ifndef TRX_NCO_H
#define TRX_NCO_H

#include "dsp/dsp_common.h"
#include "stdio.h"
#include "dsp/buffer.hpp"
#include "output.h"
#include "blocks_common.h"

class NCO : public Output<complex_t_f32> {

  public:
    NCO(float gain) : gain{gain} {};

    void reset() {
        phase = 0;
        freq = 0;
        loop_filter = 0;
    }

    // Call once per symbol/sample
    void update(float error) {
        loop_filter += gain * error;
        freq = loop_filter;
        phase += freq;
        if (phase > M_PI) {
            phase -= 2.0f * M_PI;
        }
        if (phase < -M_PI) {
            phase += 2.0f * M_PI;
        }
    }

    void get_block(buffer_t<complex_t_f32> &buff) override;
    void get_complex_sample(complex_t_f32 &sample) override;
    void get_sample(float32_t &sample) override;

  private:
    float phase = 0;
    float freq = 0;
    float loop_filter = 0;
    float gain;
};

#endif
