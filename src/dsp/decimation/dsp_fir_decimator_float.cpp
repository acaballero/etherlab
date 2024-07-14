//
// Created by Angel Dust on 21/04/2021.
//

#include "dsp/buffer.hpp"
#include "dsp/firFilter.h"
#include "dsp_fir_decimator_float.h"
#include "dsp/window.h"

#include "../../../lib/DspFilters/include/Dsp.h"
#include "../../../lib/DspFilters/include/ChebyshevI.h"
#include "../../../lib/DspFilters/include/State.h"
#include "../../../lib/DspFilters/include/Cascade.h"

void DspFIRDecimatorFloat::decimate(buffer_t<float> &src, buffer_t<float> &dst) {
    this->decimate(src, dst, 0, 2);
}

/*
 * Decimate a DSP_BLOCK size I/Q sample buffer (I/Q are interleaved)
 * This function is intended to be called either for the I or Q half of the buffer
 * @param Start: 0: Process I samples; 1: Process Q samples
 */

//__attribute__((section(".ccmram")))
void DspFIRDecimatorFloat::decimate(buffer_t<float> &src, buffer_t<float> &dst, uint8_t start, uint8_t n_channels) {

    // TODO: Consider skipping the first processed blocks to account for the delay group of the filter

    uint16_t decimated_block_size = src.count / this->factor; // DMA buffer size (DSP_BLOCK) / decimation factor
    float signalb[src.count];
    float signalOut[decimated_block_size];

    // Extract the signal from the interleaved IQ buffer
    for (uint16_t i = start, j = 0; j < src.count; i += n_channels, j++) {
        signalb[j] = src.p[i];
    }

    arm_fir_decimate_f32(&dsp_fir_decimate_instance, signalb, signalOut, src.count);

    // Write to the final adc_buffer in interleaved IQ format
    for (uint16_t i = start, j = 0; j < decimated_block_size; i += 2, j++) {
        dst.p[i] = signalOut[j];
    }
}

void DspFIRDecimatorFloat::initFilter() {

    // Generate a FIR filter with a cutoff frequency of f_khz

    generateFIRFilterCoeffs(LPF, firCoeffs, FFT_LPF_FIR_FILTER_NTAPS, this->input_rate, this->output_rate, 0);

    // Apply window

    float fir_filter_window[FFT_LPF_FIR_FILTER_NTAPS];
    generate_window(1, fir_filter_window, FFT_LPF_FIR_FILTER_NTAPS);
    arm_mult_f32(firCoeffs, fir_filter_window, firCoeffs, FFT_LPF_FIR_FILTER_NTAPS);

#if DEBUG_FFT
    printf("DSP LPF FIR Filter coefficients:\r\n");
    print_vector_f32(firCoeffs32, FFT_LPF_FIR_FILTER_NTAPS);
#endif

    this->dsp_fir_decimate_instance.M = this->factor;
    this->initialized = true;
    this->clear_state();
}

void DspFIRDecimatorFloat::config(uint32_t input_rate, uint32_t output_rate, uint16_t factor) {

    this->input_rate = input_rate;
    this->output_rate = output_rate;
    this->factor = factor;
    this->initFilter();
}

void DspFIRDecimatorFloat::clear_state() {
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(firStateBuffer));
}

bool DspFIRDecimatorFloat::isInitialized() const {
    return initialized;
}

void DspFIRDecimatorFloat::setFactor(uint16_t factor) {
    this->factor = factor;
    this->dsp_fir_decimate_instance.M = this->factor;
}


