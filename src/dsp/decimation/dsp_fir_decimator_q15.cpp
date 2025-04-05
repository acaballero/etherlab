//
// Created by Angel Dust on 21/04/2021.
//

#include "dsp/buffer.hpp"
#include "dsp/firFilter.h"
#include "dsp_fir_decimator_q15.h"
#include "dsp/window.h"

#include "../../../lib/DspFilters/include/Dsp.h"
#include "../../../lib/DspFilters/include/ChebyshevI.h"
#include "../../../lib/DspFilters/include/State.h"
#include "../../../lib/DspFilters/include/Cascade.h"
#include <sys/_stdint.h>

template class DspFIRDecimatorQ15<32, short>;
template class DspFIRDecimatorQ15<24, short>;

template <int TAPS, typename T> void DspFIRDecimatorQ15<TAPS, T>::decimate(buffer_t<T> &src, buffer_t<T> &dst) { this->decimate(src, dst, 0, 2); }

/*
 * Decimate a DSP_BLOCK size I/Q sample buffer (I/Q are interleaved)
 * This function is intended to be called either for the I or Q half of the buffer
 * @param Start: 0: Process I samples; 1: Process Q samples
 */

//__attribute__((section(".ccmram")))
template <int TAPS, typename T> void DspFIRDecimatorQ15<TAPS, T>::decimate(buffer_t<T> &src, buffer_t<T> &dst, uint8_t start, uint8_t n_channels) {

    // TODO: Consider skipping the first processed blocks to account for the delay group of the filter

    uint16_t decimated_block_size = src.count / this->factor;
    q15_t signalb[src.count];
    q15_t signalOut[decimated_block_size];

    // Extract the signal from the interleaved IQ buffer

    for (uint16_t i = start, j = 0; j < src.count; i += n_channels, j++) {
        signalb[j] = src.p[i];
    }

    arm_fir_decimate_q15(&dsp_fir_decimate_instance, signalb, signalOut, src.count);

    // Write to the final adc_buffer in interleaved IQ format
    for (uint16_t i = start, j = 0; j < decimated_block_size; i += 2, j++) {
        dst.p[i] = signalOut[j];
    }
}

template <int TAPS, typename T> void DspFIRDecimatorQ15<TAPS, T>::initFilter() {

    // Generate a FIR filter with a cutoff frequency of f_khz

    generateFIRFilterCoeffsq15(LPF, dsp_firCoeffs15, TAPS, this->input_rate, this->output_rate, 0);

    // Apply window

    float fir_filter_window[TAPS];
    generate_window(1, fir_filter_window, TAPS);
    q15_t fir_filter_window_q15[TAPS];
    arm_float_to_q15(fir_filter_window, fir_filter_window_q15, TAPS);

    arm_mult_q15(dsp_firCoeffs15, fir_filter_window_q15, dsp_firCoeffs15, TAPS);

#if DEBUG_FFT
    printf("DSP LPF FIR Filter coefficients:\r\n");
    print_vector_f32(firCoeffs32, FFT_LPF_FIR_FILTER_NTAPS);

#endif

    this->dsp_fir_decimate_instance.M = this->factor;
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(dsp_firStateBuffer));
}

template <int TAPS, typename T> void DspFIRDecimatorQ15<TAPS, T>::config(uint32_t input_rate, uint32_t output_rate, uint16_t factor) {

    this->input_rate = input_rate;
    this->output_rate = output_rate;
    this->factor = factor;
    this->initFilter();
}
