//
// Created by Angel Dust on 29/04/2025.
//

#include "MemoryFree.h"
#include "config.h"
#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft_types.h"
#include "dsp/firFilter.h"
#include "dsp_fir_decimator_float_complex.h"
#include "dsp/window.h"
#include <algorithm>
#include <exception>
#include "printf.h"

template class DspFIRDecimatorFloatComplex<FIR_DECIMATOR_SIGNAL_TAPS>;

template <int TAPS> void DspFIRDecimatorFloatComplex<TAPS>::decimate(buffer_t<complex_t_f32> &src, buffer_t<complex_t_f32> &dst) {
    // Not implemented
}

template <int TAPS> void DspFIRDecimatorFloatComplex<TAPS>::decimate(float32_t *src_i, float32_t *src_q, float32_t *dst_i, float32_t *dst_q, size_t n_samples) {
    arm_fir_decimate_f32(&fir_xi_hi, src_i, tmp_buff_i, n_samples);
    arm_fir_decimate_f32(&fir_xq_hq, src_q, tmp_buff_q, n_samples);

    for (size_t i = 0; i < n_samples / this->factor; ++i) {
        *(dst_i++) = tmp_buff_i[i] - tmp_buff_q[i]; // real
    }

    arm_fir_decimate_f32(&fir_xi_hq, src_i, dst_q, n_samples);
    arm_fir_decimate_f32(&fir_xq_hi, src_q, dst_q, n_samples);

    for (size_t i = 0; i < n_samples / this->factor; ++i) {
        *(dst_q++) = tmp_buff_i[i] - tmp_buff_q[i]; // imag
    }
}

template <int TAPS> bool DspFIRDecimatorFloatComplex<TAPS>::init() {

    bool b = false;

    float32_t coeffs[TAPS * 2];

    // This generates a complex vector with TAPS*2 length
    b = generate_fir_filter_taps(BPF, coeffs, TAPS, this->input_rate, start_frequency + (this->bandwidth / 2), this->bandwidth / 2);

    // Unzip complex taps
    for (int i = 0; i < TAPS; i++) {
        coeffs_i[i] = coeffs[i * 2 + 0];
        coeffs_q[i] = coeffs[i * 2 + 1];
    }

    // Taps must be reversed to use cmsis decimators
    std::reverse(coeffs_i, coeffs_i + TAPS);
    std::reverse(coeffs_q, coeffs_q + TAPS);

    arm_status status = arm_fir_decimate_init_f32(&fir_xi_hi, TAPS, this->factor, coeffs_i, state_xi_hi, DSP_BLOCK);
    arm_fir_decimate_init_f32(&fir_xq_hq, TAPS, this->factor, coeffs_q, state_xq_hq, DSP_BLOCK);
    arm_fir_decimate_init_f32(&fir_xq_hi, TAPS, this->factor, coeffs_q, state_xq_hi, DSP_BLOCK);
    arm_fir_decimate_init_f32(&fir_xi_hq, TAPS, this->factor, coeffs_i, state_xi_hq, DSP_BLOCK);

    b = b && status == arm_status::ARM_MATH_SUCCESS;

    initialized = b;

    return initialized;
}

template <int TAPS> bool DspFIRDecimatorFloatComplex<TAPS>::config(uint32_t input_rate, uint32_t bandwidth, uint16_t f, uint32_t start_freq) {

    this->input_rate = input_rate;
    this->bandwidth = bandwidth;
    this->factor = f;
    this->start_frequency = start_freq;

    return init();
}

template <int TAPS> void DspFIRDecimatorFloatComplex<TAPS>::clear_state() {
    memset(fir_xi_hi.pState, 0, sizeof(state_xi_hi));
    memset(fir_xq_hq.pState, 0, sizeof(state_xq_hq));
    memset(fir_xq_hi.pState, 0, sizeof(state_xq_hi));
    memset(fir_xi_hq.pState, 0, sizeof(state_xi_hq));
}

template <int TAPS> bool DspFIRDecimatorFloatComplex<TAPS>::get_initialized() const {
    return initialized;
}

template <int TAPS> void DspFIRDecimatorFloatComplex<TAPS>::set_factor(uint16_t f) {
    this->factor = f;
    fir_xi_hi.M = f;
    fir_xq_hq.M = f;
    fir_xi_hq.M = f;
    fir_xq_hq.M = f;
}
