//
// Created by Angel Dust on 21/04/2021.
//

#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft_types.h"
#include "dsp/firFilter.h"
#include "dsp_fir_decimator_float.h"
#include "dsp/window.h"

#include "../../../lib/DspFilters/include/Dsp.h"
#include "../../../lib/DspFilters/include/ChebyshevI.h"
#include "../../../lib/DspFilters/include/State.h"
#include "../../../lib/DspFilters/include/Cascade.h"

template class DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS, float>;
template class DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS, adc_type>;
template class DspFIRDecimatorFloat<32, adc_type>;
template class DspFIRDecimatorFloat<24, adc_type>;

template <int TAPS, typename T> void DspFIRDecimatorFloat<TAPS, T>::decimate(buffer_t<T> &src, buffer_t<T> &dst) { this->decimate(src, dst, 0, 2); }

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
    float signalb[n_samples];
    float signalOut[decimated_block_size];

    // Extract the signal from the interleaved IQ buffer
    for (uint16_t i = start, j = 0; j < n_samples; i += n_channels, j++) {
        signalb[j] = src.p[i];
    }

    arm_fir_decimate_f32(&dsp_fir_decimate_instance, signalb, signalOut, n_samples);

    // Write to the final adc_buffer in interleaved IQ format
    for (uint16_t i = start, j = 0; j < decimated_block_size; i += n_channels, j++) {
        dst.p[i] = signalOut[j];
    }
}

template <int TAPS, typename T> void DspFIRDecimatorFloat<TAPS, T>::initFilter() {

    if (filter_type == BPF) {
        generateFIRFilterCoeffs(filter_type, firCoeffs, TAPS, this->input_rate, start_frequency, this->output_rate);
    } else {
        generateFIRFilterCoeffs(filter_type, firCoeffs, TAPS, this->input_rate, this->output_rate, 0);
    }

    // Apply window

    float fir_filter_window[TAPS];
    generate_window(1, fir_filter_window, TAPS);
    arm_mult_f32(firCoeffs, fir_filter_window, firCoeffs, TAPS);

#if DEBUG_FFT
    printf("DSP LPF FIR Filter coefficients:\r\n");
    print_vector_f32(firCoeffs32, FFT_LPF_FIR_FILTER_NTAPS);
#endif

    dsp_fir_decimate_instance.M = this->factor;
    initialized = true;
    clear_state();
}

template <int TAPS, typename T> void DspFIRDecimatorFloat<TAPS, T>::config(uint32_t input_rate, uint32_t output_rate, uint16_t f, uint32_t start_freq) {

    this->input_rate = input_rate;
    this->output_rate = output_rate;
    this->factor = f;
    if (start_freq) {
        filter_type = BPF;
        start_frequency = start_freq;
    } else {
        filter_type = LPF;
        start_frequency = 0;
    }
    initFilter();
}

template <int TAPS, typename T> void DspFIRDecimatorFloat<TAPS, T>::clear_state() { memset(dsp_fir_decimate_instance.pState, 0, sizeof(firStateBuffer)); }

template <int TAPS, typename T> bool DspFIRDecimatorFloat<TAPS, T>::isInitialized() const { return initialized; }

template <int TAPS, typename T> void DspFIRDecimatorFloat<TAPS, T>::setFactor(uint16_t factor) {
    this->factor = factor;
    dsp_fir_decimate_instance.M = factor;
}
