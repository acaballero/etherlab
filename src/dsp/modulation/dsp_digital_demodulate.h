//
// Created by Angel Dust on 14/05/2025.
//
#ifndef __DSP_DEMODULATE_H__
#define __DSP_DEMODULATE_H__

#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/blocks/output.h"
#include "dsp_hilbert.h"
#include "dsp/blocks/nco.h"
#include <stdint.h>

namespace dsp {

class digital_demodulator {
  public:
    virtual void work(buffer_t<complex_t_f32> &src, buffer_t<uint8_t> &dst) = 0;
};

class qpsk_demodulator : public digital_demodulator {
  public:
    qpsk_demodulator() {
        init();
    }
    void work(buffer_t<complex_t_f32> &src, buffer_t<uint8_t> &dst) override;
    void configure(const int samples_per_symbol) {
        sps = samples_per_symbol;
        init();
    }

  private:
    static int constexpr rrc_n_taps = 33; // Raised cosine filter taps
    int sps = 4;                          // Samples per symbol
    float rrc_taps[rrc_n_taps];           // To be initialized
    arm_fir_instance_f32 rrc_filter_i;
    arm_fir_instance_f32 rrc_filter_q;
    float rrc_state_i[rrc_n_taps + DSP_BLOCK - 1];
    float rrc_state_q[rrc_n_taps + DSP_BLOCK - 1];

    NCO pll{0.02f}; // TODO: Adjust gain based on loop bandwidth

    void init();
};

} // namespace dsp

#endif
