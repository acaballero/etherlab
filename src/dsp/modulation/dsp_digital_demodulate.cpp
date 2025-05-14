//
// Created by Angel Dust on 14/05/2025.
//
#include "dsp_digital_demodulate.h"
#include <cstdint>
#include <cstdio>
#include <sys/_stdint.h>
#include "arm_math.h"
#include "dsp/dsp_common.h"

namespace dsp {

void qpsk_demodulator::init() {
    generate_raisedcosine_taps(rrc_taps, rrc_n_taps, sps);
    arm_fir_init_f32(&rrc_filter_i, rrc_n_taps, rrc_taps, rrc_state_i, 1);
    arm_fir_init_f32(&rrc_filter_q, rrc_n_taps, rrc_taps, rrc_state_q, 1);

    pll.reset();
}

void qpsk_demodulator::work(buffer_t<complex_t_f32> &src, buffer_t<uint8_t> &dst) {

    /*
       Assumes buffer is alreadt raise-cosine filtered
        float i_filt, q_filt;
        arm_fir_f32(&rrc_filter_i, &src_p[i].i, &i_filt, 1);
        arm_fir_f32(&rrc_filter_r, &src_p[i].r, &q_filt, 1);
        complex_t filtered = {i_filt, q_filt};
     */

    float timing_gain = 0.01f;

    float mu = 0.0f;

    uint32_t bit_idx = 0;

    uint8_t *dst_p = dst.p;
    complex_t_f32 *src_p = src.p;

    for (uint32_t i = 0; i < src.count; i += 1) {

        float32_t s_i = src_p[i].i;
        float32_t s_r = src_p[i].r;

        // Sample near symbol boundary (simple sub-sampling)
        if (fmodf((float)i + mu, sps) < 0.5f) {
            // Costas NCO: mix down
            complex_t_f32 pll_sample;
            pll.get_complex_sample(pll_sample);

            complex_t_f32 mixed = {s_i * pll_sample.i - s_r * pll_sample.r, s_i * pll_sample.r + s_r * pll_sample.i};

            // QPSK decision
            dst_p[bit_idx++] = (mixed.i >= 0.0f) ? 1 : 0;
            dst_p[bit_idx++] = (mixed.r >= 0.0f) ? 1 : 0;

            // Phase error estimate (Costas loop)
            float error = mixed.i * mixed.r;
            pll.update(error);

            // Gardner timing error (approximate)
            if (i >= 1 && i + 1 < src.count) {
                float prev_i = src.p[i - 1].i;
                float next_i = src.p[i + 1].i;
                float timing_error = (next_i - prev_i) * s_i;
                mu += timing_gain * timing_error;

                // Clamp mu
                if (mu > sps) {
                    mu -= sps;
                }
                if (mu < 0) {
                    mu += sps;
                }
            }
        }
    }
}
} // namespace dsp
