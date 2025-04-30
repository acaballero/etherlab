//
// Created by Angel Dust on 21/04/2021.
//

#include "../buffer.hpp"
#include "dsp_iir_decimator.h"
#include "../../../lib/DspFilters/include/Dsp.h"
#include "../../../lib/DspFilters/include/ChebyshevI.h"
#include "../../../lib/DspFilters/include/State.h"
#include "../../../lib/DspFilters/include/Cascade.h"
#include <sys/_stdint.h>

void DspIIRDecimator::decimate(buffer_t<int16_t> &src, buffer_t<int16_t> &dst) {
    this->decimate(src, dst, 0, 2, 2);
    this->decimate(src, dst, 1, 2, 2);
}

/*
 * Decimate a DSP_BLOCK size I/Q sample buffer (I/Q are interleaved)
 * This function is intended to be called either for the I or Q half of the buffer
 * @param n_channels: 1: Store one channel sequentially; 2: Store two channels interleaved
 * @param start: 0: Process I samples; 1: Process Q samples
 *
 * WARN: This function overwrites de original buffer
 */
//__attribute__((section(".ccmram")))
void DspIIRDecimator::decimate(buffer_t<int16_t> &src, buffer_t<int16_t> &dst, uint8_t channel_n, uint8_t n_channels_in, uint8_t n_channels_out) {

    uint8_t n = src.count / n_channels_in;

#if DSP_USE_IIR_Q15
    q15_t signalb[n];
    q15_t signalOut[n];
#else
    float signalb[n];
    float signalOut[n];
#endif

#if DSP_USE_IIR_Q15
    // Extract the signal from the interleaved IQ buffer

    for (int i = start, j = 0; j < n; i += 2, j++) {
        signalb[j] = (25) << 6; // buffer[i]<<3; //scale
    }

    arm_biquad_cascade_df1_q15(&iir_instance_I, signalb, signalOut, n);
#else

    // Extract the signal from the interleaved IQ buffer
    for (uint16_t i = channel_n, j = 0; j < n; i += n_channels_in, j++) {
        signalb[j] = src.p[i];
    }

    arm_biquad_cascade_df1_f32(&iir_instance, signalb, signalOut, n);
#endif

#if DSP_USE_IIR_Q15
    // Write to the final adc_buffer in interleaved IQ format
    for (int i = start, j = 0; j < n; i += 2, j += this->factor) {
        buffer[i] = signalOut[j] >> 6;
    }
#else
    // Write the decimated final adc_buffer in interleaved IQ format
    for (uint16_t i = channel_n, j = 0; j < n; i += n_channels_out, j += this->factor) {
        dst.p[i] = signalOut[j];
    }
#endif
}

void DspIIRDecimator::initFilter() {

    // Generate coefficients for the current DSP parameters

    Dsp::SimpleFilter<Dsp::ChebyshevI::LowPass<4>, 1, Dsp::DirectFormI> f;
    f.setup(4,                               // order
            this->input_rate,                // sample rate
            ((double)this->bandwidth / 2.0), // center frequency
            0.01);                           // ripple dB

    Dsp::Cascade::Storage st = f.getCascadeStorage();
    Dsp::Cascade::Stage *dg = st.stageArray;

    // Convert to CMSIS format (output coefficients are negated)
    IIRFilterCoefficients[0] = dg[0].m_b0;
    IIRFilterCoefficients[1] = dg[0].m_b1;
    IIRFilterCoefficients[2] = dg[0].m_b2;
    IIRFilterCoefficients[3] = -dg[0].m_a1;
    IIRFilterCoefficients[4] = -dg[0].m_a2;
    IIRFilterCoefficients[5] = dg[1].m_b0;
    IIRFilterCoefficients[6] = dg[1].m_b1;
    IIRFilterCoefficients[7] = dg[1].m_b2;
    IIRFilterCoefficients[8] = -dg[1].m_a1;
    IIRFilterCoefficients[9] = -dg[1].m_a2;

#if DSP_USE_IIR_Q15
    q15_t coeffs[12];

    // If the float coefficients exceed the range [+1 -1), we have to scale and use the postShift feature of the IIR initialization function
    uint8_t postShift = 1;
    for (int i = 0; i < 12; i++) {
        IIRFilterCoefficients[i] /= 2;
    }
    arm_float_to_q15(IIRFilterCoefficients, coeffs, 12);

    arm_biquad_cascade_df1_init_q15(&iir_instance_I, IIRFilterNumStages, coeffs, IIRStateBufferI, postShift);
#else
    arm_biquad_cascade_df1_init_f32(&iir_instance, IIRFilterNumStages, IIRFilterCoefficients, IIRStateBuffer);
#endif
}

bool DspIIRDecimator::config(uint32_t input_rate, uint32_t output_rate, uint16_t factor, uint32_t start_Freq) {

    this->input_rate = input_rate;
    this->bandwidth = output_rate;
    this->factor = factor;
    this->initFilter();

    return true;
}

void test_iir_decimator() {

    DspIIRDecimator d;

    d.config(10000, 5000, 8);
    int16_t buff[16];
    int16_t buff_o[16];
    buffer_t<int16_t> b(buff, 16);
    buffer_t<int16_t> bo(buff_o, 16);

    for (int i = 0; i < 16; i++) {
        b.p[i] = 1300;
    }

    for (int i = 0; i < 16; i++) {

        printf("Decimation pass %d:\n", i);

        d.decimate(b, bo, 0, 1, 1);
        for (int j = 0; j < 16; j++) {
            printf("%d -> %d\n", b.p[j], bo.p[j]);
        }
    }
}
