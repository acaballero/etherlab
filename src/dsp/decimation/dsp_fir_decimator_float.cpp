//
// Created by Angel Dust on 21/04/2021.
//

#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft_types.h"
#include "dsp/fir_filter.h"
#include "dsp_fir_decimator_float.h"
#include "dsp/window.h"
#include "handlers.h"
#include <algorithm>
#include <exception>
#include "memory_allocator.h"

template class DspFIRDecimatorFloatBase<FFT_LPF_FIR_FILTER_NTAPS, float32_t>;

// template class DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS>;
template class DspFIRDecimatorFloat<28>;
// template class DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS, adc_type>;

template class DspFIRDecimatorFloat<FIR_DECIMATOR_1ST_HALFBAND_TAPS>;
template class DspFIRDecimatorFloat<FIR_DECIMATOR_SIGNAL_TAPS>;

/*
 * Decimate a DSP_BLOCK size I/Q sample buffer (I/Q are interleaved)
 * This function is intended to be called either for the I or Q half of the buffer
 * @param Start: 0: Process I samples; 1: Process Q samples
 */
template <int TAPS>
void DspFIRDecimatorFloat<TAPS>::decimate(buffer_t<float32_t> &src, buffer_t<float32_t> &dst, uint8_t start, uint8_t n_channels, int start_dst) {

    // TODO: Consider skipping the first processed blocks to account for the delay group of the filter

    uint16_t n_samples = src.count / n_channels;
    uint16_t decimated_block_size = n_samples / this->factor; // DMA buffer size (DSP_BLOCK) / decimation factor
    if (start_dst == -1) {
        start_dst = start;
    }

    // Extract the signal from the interleaved IQ buffer
    for (uint16_t i = start, j = 0; j < n_samples; i += n_channels, j++) {
        this->tmp_buff[j] = src.p[i];
    }

    arm_fir_decimate_f32(&this->dsp_fir_decimate_instance, this->tmp_buff, this->tmp_buff, n_samples);

    // Write to the final adc_buffer in interleaved IQ format
    for (uint16_t i = start_dst, j = 0; j < decimated_block_size; i += n_channels, j++) {
        dst.p[i] = this->tmp_buff[j];
        // LOG_RAW("%.1f,", tmp_buff_out[j]);
    }
    // LOG_RAW("\n");
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

template <int TAPS> void DspFIRDecimatorFloat<TAPS>::decimate(buffer_t<complex_t_f32> &src, float32_t *dst_i, float32_t *dst_q) {

    uint16_t n_samples = src.count;

    dsp::unzip_f32((const float32_t *)src.p, tmp_buff, tmp_buff_q, n_samples);

    arm_fir_decimate_f32(&dsp_fir_decimate_instance, tmp_buff, dst_i, n_samples);
    arm_fir_decimate_f32(&dsp_fir_decimate_instance_q, tmp_buff_q, dst_q, n_samples);
}

template <int TAPS> void DspFIRDecimatorFloat<TAPS>::decimate(buffer_t<float32_t> &src, buffer_t<float32_t> &dst) {

    uint16_t n_samples = src.count;
    if (src.format == COMPLEX_INTERLEAVED && dst.format == COMPLEX_INTERLEAVED) {

        uint16_t decimated_block_size = n_samples / this->factor; // DMA buffer size (DSP_BLOCK) / decimation factor

        dsp::unzip_f32((const float32_t *)src.p, tmp_buff, tmp_buff_q, n_samples);

        arm_fir_decimate_f32(&dsp_fir_decimate_instance, tmp_buff, tmp_buff, n_samples);
        arm_fir_decimate_f32(&dsp_fir_decimate_instance_q, tmp_buff_q, tmp_buff_q, n_samples);

        dsp::zip_f32(tmp_buff, tmp_buff_q, (float32_t *)dst.p, decimated_block_size);
    } else if (src.format == COMPLEX_SEQUENTIAL && dst.format == COMPLEX_SEQUENTIAL) {
        decimate((float32_t *)src.p, (float32_t *)src.p + src.count, (float32_t *)dst.p, (float32_t *)dst.p + src.count / this->factor, src.count);
    } else if (src.format == COMPLEX_SEQUENTIAL && dst.format == COMPLEX_INTERLEAVED) {
        decimate((float32_t *)src.p, (float32_t *)src.p + src.count, dst, dst.count);
    } else if (src.format == REAL && dst.format == REAL) {
        arm_fir_decimate_f32(&dsp_fir_decimate_instance, src.p, dst.p, n_samples);
    } else {
        HardFault_Handler();
    }
}

template <int TAPS> void DspFIRDecimatorFloat<TAPS>::decimate(float32_t *src_i, float32_t *src_q, float32_t *dst_i, float32_t *dst_q, size_t n_samples) {
    if (dsp_fir_decimate_instance.numTaps > 100 || dsp_fir_decimate_instance_q.numTaps > 100) {
        HardFault_Handler();
    }

    arm_fir_decimate_f32(&dsp_fir_decimate_instance, src_i, dst_i, n_samples);
    arm_fir_decimate_f32(&dsp_fir_decimate_instance_q, src_q, dst_q, n_samples);
}

template <int TAPS> void DspFIRDecimatorFloat<TAPS>::decimate(float32_t *src_i, float32_t *src_q, buffer_t<float32_t> &dst, size_t n_samples) {

    uint16_t decimated_block_size = n_samples / this->factor; // DMA buffer size (DSP_BLOCK) / decimation factor

    arm_fir_decimate_f32(&dsp_fir_decimate_instance, src_i, tmp_buff, n_samples);
    arm_fir_decimate_f32(&dsp_fir_decimate_instance_q, src_q, tmp_buff_q, n_samples);

    dsp::zip_f32(tmp_buff, tmp_buff_q, (float32_t *)dst.p, decimated_block_size);
}

template <int TAPS, typename T> bool DspFIRDecimatorFloatBase<TAPS, T>::init(uint16_t block_size) {

    bool b = false;

    if (coeffs == nullptr) {
        coeffs = (float32_t *)CCMMemoryAllocator::alloc(TAPS * sizeof(float32_t));
        state = (float32_t *)CCMMemoryAllocator::alloc(state_size);
        tmp_buff = (float32_t *)CCMMemoryAllocator::alloc(block_size * sizeof(float32_t));
    }

    if (type == BPF) {
        b = generate_fir_filter_taps(type, coeffs, TAPS, this->input_rate, start_frequency, this->bandwidth);
    } else {

        b = generate_fir_filter_taps(type, coeffs, TAPS, this->input_rate, this->bandwidth, 0);
    }

    // Taps must be reversed to use cmsis decimators
    std::reverse(coeffs, coeffs + TAPS);

    arm_status status = arm_fir_decimate_init_f32(&dsp_fir_decimate_instance, TAPS, this->factor, coeffs, state, block_size);

    b = b && status == arm_status::ARM_MATH_SUCCESS;

    initialized = b;

    return initialized;
}

template <int TAPS> bool DspFIRDecimatorFloat<TAPS>::init(uint16_t block_size) {

    bool ret = DspFIRDecimatorFloatBase<TAPS, float32_t>::init(block_size);

    if (state_q == nullptr) {
        state_q = (float32_t *)CCMMemoryAllocator::alloc(state_size);
        tmp_buff_q = (float32_t *)CCMMemoryAllocator::alloc(block_size * sizeof(float32_t));
    }

    arm_status status = arm_fir_decimate_init_f32(&dsp_fir_decimate_instance_q, TAPS, this->factor, this->coeffs, state_q, block_size);

    ret = ret && status == arm_status::ARM_MATH_SUCCESS;
    return ret;
}

template <int TAPS> bool DspFIRDecimatorFloat<TAPS>::config(uint32_t input_rate, uint32_t bandwidth, uint16_t f, uint32_t start_freq, uint16_t block_size) {

    LOG("DspFIRDecimatorFloat config | input_rate: %u | bandwidth: %d", input_rate, bandwidth);
    LOG_RAW(" | factor: %d | block_size: %d\n", f, block_size);
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

    return this->init(block_size);
}

template <int TAPS> void DspFIRDecimatorFloat<TAPS>::reset() {
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(state_size));
    memset(dsp_fir_decimate_instance_q.pState, 0, sizeof(state_size));
}

template <int TAPS, typename T> void DspFIRDecimatorFloatBase<TAPS, T>::reset() {
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(state_size));
}

template <int TAPS, typename T> bool DspFIRDecimatorFloatBase<TAPS, T>::get_initialized() const {
    return initialized;
}

template <int TAPS, typename T> void DspFIRDecimatorFloatBase<TAPS, T>::set_factor(uint16_t factor) {
    this->factor = factor;
    dsp_fir_decimate_instance.M = factor;
}

template <int TAPS> void DspFIRDecimatorFloat<TAPS>::set_factor(uint16_t f) {
    this->factor = f;
    dsp_fir_decimate_instance.M = f;
    dsp_fir_decimate_instance_q.M = f;
}
