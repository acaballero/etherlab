//
// Created by Angel Dust on 21/04/2021.
//

#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft_types.h"
#include "dsp/fir_filter.h"
#include "dsp_fir_decimator_float.h"
#include "dsp/window.h"
#include <algorithm>
#include <exception>
#include <sys/_stdint.h>

template class DspFIRDecimatorFloatBase<FFT_LPF_FIR_FILTER_NTAPS, float>;
template class DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS, float>;
// template class DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS, adc_type>;
template class DspFIRDecimatorFloat<FIR_DECIMATOR_1ST_HALFBAND_TAPS, complex_t_f32>;
template class DspFIRDecimatorFloat<FIR_DECIMATOR_SIGNAL_TAPS, complex_t_f32>;

template <int TAPS, typename T> void DspFIRDecimatorFloat<TAPS, T>::decimate(buffer_t<T> &src, buffer_t<T> &dst) {
    this->decimate(src, dst, 0, 2);
}

/*
 * Decimate a DSP_BLOCK size I/Q sample buffer (I/Q are interleaved)
 * This function is intended to be called either for the I or Q half of the buffer
 * @param Start: 0: Process I samples; 1: Process Q samples
 */

//__attribute__((section(".ccmram")))
template <int TAPS, typename T> void DspFIRDecimatorFloat<TAPS, T>::decimate(buffer_t<T> &src, buffer_t<T> &dst, uint8_t start, uint8_t n_channels) {

    // TODO: Consider skipping the first processed blocks to account for the delay group of the filter

    uint16_t n_samples = src.count / n_channels;
    uint16_t decimated_block_size = n_samples / this->factor; // DMA buffer size (DSP_BLOCK) / decimation factor

    // Extract the signal from the interleaved IQ buffer
    for (uint16_t i = start, j = 0; j < n_samples; i += n_channels, j++) {
        this->tmp_buff_in[j] = src.p[i];
    }

    arm_fir_decimate_f32(&this->dsp_fir_decimate_instance, this->tmp_buff_in, this->tmp_buff_out, n_samples);

    // Write to the final adc_buffer in interleaved IQ format
    for (uint16_t i = start, j = 0; j < decimated_block_size; i += n_channels, j++) {
        dst.p[i] = this->tmp_buff_out[j];
    }
}

// template <int TAPS> void DspFIRDecimatorFloat<TAPS, complex_t_f32>::decimate(buffer_t<complex_t_f32> &src, buffer_t<complex_t_f32> &dst) {

//     uint16_t n_samples = src.count;
//     uint16_t decimated_block_size = n_samples / this->factor; // DMA buffer size (DSP_BLOCK) / decimation factor

//     // Extract the signal from the interleaved IQ buffer
//     for (uint16_t i = 0; i < n_samples; i++) {
//         tmp_buff_in[i] = src.p[i].i;
//         tmp_buff_in_q[i] = src.p[i].r;
//     }

//     arm_fir_decimate_f32(&dsp_fir_decimate_instance, tmp_buff_in, tmp_buff_out, n_samples);
//     arm_fir_decimate_f32(&dsp_fir_decimate_instance_q, tmp_buff_in_q, tmp_buff_out_q, n_samples);

//     // Write to the final buffer in interleaved IQ format
//     for (uint16_t i = 0; i < decimated_block_size; i++) {
//         dst.p[i].i = tmp_buff_out[i];
//         dst.p[i].r = tmp_buff_out_q[i];
//     }
// }

template <int TAPS> void DspFIRDecimatorFloat<TAPS, complex_t_f32>::decimate(buffer_t<complex_t_f32> &src, float32_t *dst_i, float32_t *dst_q) {

    uint16_t n_samples = src.count;

    dsp::unzip_f32((const float32_t *)src.p, tmp_buff_in, tmp_buff_in_q, n_samples);

    arm_fir_decimate_f32(&dsp_fir_decimate_instance, tmp_buff_in, dst_i, n_samples);
    arm_fir_decimate_f32(&dsp_fir_decimate_instance_q, tmp_buff_in_q, dst_q, n_samples);
}

template <int TAPS> void DspFIRDecimatorFloat<TAPS, complex_t_f32>::decimate(buffer_t<complex_t_f32> &src, buffer_t<complex_t_f32> &dst) {

    uint16_t n_samples = src.count;
    uint16_t decimated_block_size = n_samples / this->factor; // DMA buffer size (DSP_BLOCK) / decimation factor

    dsp::unzip_f32((const float32_t *)src.p, tmp_buff_in, tmp_buff_in_q, n_samples);

    arm_fir_decimate_f32(&dsp_fir_decimate_instance, tmp_buff_in, tmp_buff_out, n_samples);
    arm_fir_decimate_f32(&dsp_fir_decimate_instance_q, tmp_buff_in_q, tmp_buff_out_q, n_samples);

    dsp::zip_f32(tmp_buff_out, tmp_buff_out_q, (float32_t *)dst.p, decimated_block_size);
}

template <int TAPS>
void DspFIRDecimatorFloat<TAPS, complex_t_f32>::decimate(float32_t *src_i, float32_t *src_q, float32_t *dst_i, float32_t *dst_q, size_t n_samples) {
    arm_fir_decimate_f32(&dsp_fir_decimate_instance, src_i, dst_i, n_samples);
    arm_fir_decimate_f32(&dsp_fir_decimate_instance_q, src_q, dst_q, n_samples);
}

template <int TAPS>
void DspFIRDecimatorFloat<TAPS, complex_t_f32>::decimate(float32_t *src_i, float32_t *src_q, buffer_t<complex_t_f32> &dst, size_t n_samples) {

    uint16_t decimated_block_size = n_samples / this->factor; // DMA buffer size (DSP_BLOCK) / decimation factor

    arm_fir_decimate_f32(&dsp_fir_decimate_instance, src_i, tmp_buff_out, n_samples);
    arm_fir_decimate_f32(&dsp_fir_decimate_instance_q, src_q, tmp_buff_out_q, n_samples);

    dsp::zip_f32(tmp_buff_out, tmp_buff_out_q, (float32_t *)dst.p, decimated_block_size);
}

template <int TAPS, typename T> bool DspFIRDecimatorFloatBase<TAPS, T>::init() {

    bool b = false;

    if (type == BPF) {
        b = generate_fir_filter_taps(type, coeffs, TAPS, this->input_rate, start_frequency, this->bandwidth);
    } else {
        b = generate_fir_filter_taps(type, coeffs, TAPS, this->input_rate, this->bandwidth, 0);
    }

    // Taps must be reversed to use cmsis decimators
    std::reverse(coeffs, coeffs + TAPS);

    arm_status status = arm_fir_decimate_init_f32(&dsp_fir_decimate_instance, TAPS, this->factor, coeffs, state, DSP_BLOCK);

    b = b && status == arm_status::ARM_MATH_SUCCESS;

    initialized = b;

    return initialized;
}

template <int TAPS> bool DspFIRDecimatorFloat<TAPS, complex_t_f32>::init() {

    bool ret = DspFIRDecimatorFloatBase<TAPS, complex_t_f32>::init();

    arm_status status = arm_fir_decimate_init_f32(&dsp_fir_decimate_instance_q, TAPS, this->factor, this->coeffs, state_q, DSP_BLOCK);

    ret = ret && status == arm_status::ARM_MATH_SUCCESS;
    return ret;
}

template <int TAPS, typename T> bool DspFIRDecimatorFloat<TAPS, T>::config(uint32_t input_rate, uint32_t bandwidth, uint16_t f, uint32_t start_freq) {

    this->input_rate = input_rate;
    this->bandwidth = bandwidth;
    this->factor = f;

    if (start_freq) {
        this->type = BPF;
        this->start_frequency = start_freq;
    } else {
        this->type = LPF;
        this->start_frequency = 0;
    }

    return this->init();
}

template <int TAPS> void DspFIRDecimatorFloat<TAPS, complex_t_f32>::clear_state() {
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(state));
    memset(dsp_fir_decimate_instance_q.pState, 0, sizeof(state_q));
}

template <int TAPS> bool DspFIRDecimatorFloat<TAPS, complex_t_f32>::config(uint32_t input_rate, uint32_t bandwidth, uint16_t f, uint32_t start_freq) {
    this->input_rate = input_rate;
    this->bandwidth = bandwidth;
    this->factor = f;

    if (start_freq) {
        this->type = BPF;
        this->start_frequency = start_freq;
    } else {
        this->type = LPF;
        this->start_frequency = 0;
    }

    return this->init();
}

template <int TAPS, typename T> void DspFIRDecimatorFloatBase<TAPS, T>::clear_state() {
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(state));
}

template <int TAPS, typename T> bool DspFIRDecimatorFloatBase<TAPS, T>::get_initialized() const {
    return initialized;
}

template <int TAPS, typename T> void DspFIRDecimatorFloatBase<TAPS, T>::set_factor(uint16_t factor) {
    this->factor = factor;
    dsp_fir_decimate_instance.M = factor;
}

template <int TAPS> void DspFIRDecimatorFloat<TAPS, complex_t_f32>::set_factor(uint16_t f) {
    this->factor = f;
    dsp_fir_decimate_instance.M = f;
    dsp_fir_decimate_instance_q.M = f;
}
