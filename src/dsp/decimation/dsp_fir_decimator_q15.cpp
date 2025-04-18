//
// Created by Angel Dust on 21/04/2021.
//

#include "arm_math.h"
#include "dsp/buffer.hpp"
#include "dsp/firFilter.h"
#include "dsp_fir_decimator_q15.h"
#include "dsp/window.h"
#include "../../../lib/DspFilters/include/Dsp.h"
#include "../../../lib/DspFilters/include/ChebyshevI.h"
#include "../../../lib/DspFilters/include/State.h"
#include "../../../lib/DspFilters/include/Cascade.h"

template class DspFIRDecimatorQ15<32, short>;
template class DspFIRDecimatorQ15<24, short>;

template <int TAPS, typename T> void DspFIRDecimatorQ15<TAPS, T>::decimate(buffer_t<T> &src, buffer_t<T> &dst) {
    this->decimate(src, dst, 0, 2);
}

/*
 * Decimate a DSP_BLOCK size I/Q sample buffer (I/Q are interleaved)
 * This function is intended to be called either for the I or Q half of the buffer
 * @param Start: 0: Process I samples; 1: Process Q samples
 */

template <int TAPS, typename T> void DspFIRDecimatorQ15<TAPS, T>::decimate(buffer_t<T> &src, buffer_t<T> &dst, uint8_t start, uint8_t n_channels) {

    // TODO: Consider skipping the first processed blocks to account for the delay group of the filter
    uint16_t n_samples = src.count / n_channels;
    uint16_t decimated_block_size = src.count / this->factor;

    // Extract the signal from the interleaved IQ buffer

    for (uint16_t i = start, j = 0; j < n_samples; i += n_channels, j++) {
        this->tmp_buff_in[j] = src.p[i];
    }

    arm_fir_decimate_q15(&this->dsp_fir_decimate_instance, this->tmp_buff_in, this->tmp_buff_out, n_samples);

    // Write to the final adc_buffer in interleaved IQ format
    for (uint16_t i = start, j = 0; j < decimated_block_size; i += 2, j++) {
        dst.p[i] = this->tmp_buff_out[j];
    }
}

template <int TAPS> void DspFIRDecimatorQ15<TAPS, complex_t>::decimate(buffer_t<complex_t> &src, buffer_t<complex_t> &dst) {

    uint16_t n_samples = src.count;
    uint16_t decimated_block_size = n_samples / this->factor; // DMA buffer size (DSP_BLOCK) / decimation factor
    const complex_t *src_p = src.p;

    // Extract the signal from the interleaved IQ buffer
    for (uint16_t i = 0; i < n_samples; i += 2) {

        // Load 4 interleaved samples (I0,Q0,I1,Q1)

        int32_t in1 = *__SIMD32(src_p)++; // SIMD32 [Q0 | I0]
        int32_t in2 = *__SIMD32(src_p)++; // SIMD32 [Q1 | I1]

        // Extract I samples
        int32_t i_pack = __PKHTB(in2, in1, 16); // [I1 | I0]
        int32_t q_pack = __PKHBT(in1, in2, 16); // [Q1 | Q0]

        *(int32_t *)&tmp_buff_in[i] = i_pack;
        *(int32_t *)&tmp_buff_in_q[i] = q_pack;
    }

    arm_fir_decimate_q15(&dsp_fir_decimate_instance, tmp_buff_in, tmp_buff_out, n_samples);
    arm_fir_decimate_q15(&dsp_fir_decimate_instance_q, tmp_buff_in_q, tmp_buff_out_q, n_samples);

    // Write to the final buffer in interleaved IQ format
    for (uint16_t i = 0; i < decimated_block_size; i += 2) {

        // Load 2 I and 2 Q samples
        int32_t i_pack = *__SIMD32(tmp_buff_out)++;   // [I1 | I0]
        int32_t q_pack = *__SIMD32(tmp_buff_out_q)++; // [Q1 | Q0]

        int32_t out1 = __PKHBT(i_pack, q_pack, 16); // [Q0 | I0]
        int32_t out2 = __PKHTB(q_pack, i_pack, 16); // [Q1 | I1]

        // Store interleaved output
        *(int32_t *)&dst.p[2 * i] = out1;
        *(int32_t *)&dst.p[2 * i + 2] = out2;
    }
}

template <int TAPS, typename T> bool DspFIRDecimatorQ15Base<TAPS, T>::init() {

    generate_fir_filter_taps_q15(LPF, coeffs, TAPS, this->input_rate, this->output_rate, 0);

    this->dsp_fir_decimate_instance.M = this->factor;
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(state));
}

template <int TAPS, typename T> bool DspFIRDecimatorQ15Base<TAPS, T>::config(uint32_t input_rate, uint32_t output_rate, uint16_t factor) {

    this->input_rate = input_rate;
    this->output_rate = output_rate;
    this->factor = factor;
    return this->init();
}

template <int TAPS> void DspFIRDecimatorQ15<TAPS, complex_t>::clear_state() {
    memset(dsp_fir_decimate_instance.p.State, 0, sizeof(state));
    memset(dsp_fir_decimate_instance_q.pState, 0, sizeof(state_q));
}

template <int TAPS, typename T> void DspFIRDecimatorQ15Base<TAPS, T>::clear_state() {
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(state));
}

template <int TAPS, typename T> bool DspFIRDecimatorQ15Base<TAPS, T>::get_initialized() const {
    return initialized;
}

template <int TAPS, typename T> void DspFIRDecimatorQ15Base<TAPS, T>::set_factor(uint16_t factor) {
    this->factor = factor;
    dsp_fir_decimate_instance.M = factor;
}
