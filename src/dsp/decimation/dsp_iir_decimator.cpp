//
// Created by Angel Dust on 21/04/2021.
//

#include "../buffer.hpp"
#include "Elliptic.h"
#include "dsp/fir_filter.h"
#include "dsp_iir_decimator.h"
#include "../../../lib/DspFilters/include/Dsp.h"
#include "../../../lib/DspFilters/include/ChebyshevI.h"
#include "../../../lib/DspFilters/include/State.h"
#include "../../../lib/DspFilters/include/Cascade.h"
#include "../../../lib/DspFilters/include/Filter.h"
#include "../../status.h"

template class DspIIRDecimator<1>; // Pre-declared
template class DspIIRDecimator<2>; // Pre-declared

template <int order> void DspIIRDecimator<order>::decimate(buffer_t<int16_t> &src, buffer_t<int16_t> &dst) {
    // FIXME: Shouldn't this use a different decimator instance for each channel?
    this->decimate(src, dst, 0, 2, 2);
    this->decimate(src, dst, 1, 2, 2);
}

template <int order> void DspIIRDecimator<order>::decimate(int16_t *src_i, int16_t *src_q, int16_t *dst_i, int16_t *dst_q, size_t n_samples) {
    status::handleError(status::ST_ERROR, "Not implemented");
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
template <int order>
void DspIIRDecimator<order>::decimate(const buffer_t<int16_t> &src, buffer_t<int16_t> &dst, const uint8_t channel_n, const uint8_t n_channels_in,
                                      const uint8_t n_channels_out) {

    uint8_t n = src.count / n_channels_in;

    float signalb[n];
    float signalOut[n];

    for (uint16_t i = channel_n, j = 0; j < n; i += n_channels_in, j++) {
        signalb[j] = src.p[i];
    }

    arm_biquad_cascade_df1_f32(&iir_instance, signalb, signalOut, n);

    for (uint16_t i = channel_n, j = 0; j < n; i += n_channels_out, j += this->factor) {
        dst.p[i] = signalOut[j];
    }
}

template <int order>
void DspIIRDecimator<order>::decimate(const buffer_t<float32_t> &src, buffer_t<float32_t> &dst, const uint8_t channel_n, const uint8_t n_channels_in,
                                      uint8_t n_channels_out) {

    uint8_t n = src.count / n_channels_in;

    float in[n];
    float out[n];

    for (uint16_t i = channel_n, j = 0; j < n; i += n_channels_in, j++) {
        in[j] = src.p[i];
    }

    arm_biquad_cascade_df1_f32(&iir_instance, in, out, n);

    for (uint16_t i = channel_n, j = 0; j < n; i += n_channels_out, j += this->factor) {
        dst.p[i] = out[j];
    }
}

template <int order> void DspIIRDecimator<order>::init() {

    // Generate coefficients for the current DSP parameters

    Dsp::Cascade::Stage *dg;

    if (type == LPF) {
        Dsp::SimpleFilter<Dsp::Butterworth::LowPass<order>, 1, Dsp::DirectFormI> f;

        f.setup(order,                    // order
                this->input_rate,         // sample rate
                ((double)this->bandwidth) // cutoff frequency
        );                                // ripple dB

        Dsp::Cascade::Storage st = f.getCascadeStorage();
        dg = st.stageArray;
    } else if (type == HPF) {
        Dsp::SimpleFilter<Dsp::Butterworth::HighPass<order>, 1, Dsp::DirectFormI> f;

        f.setup(order,                    // order
                this->input_rate,         // sample rate
                ((double)this->bandwidth) // cutoff frequency
                                          // Ripple
        );                                // Rolloff

        Dsp::Cascade::Storage st = f.getCascadeStorage();
        dg = st.stageArray;
    } else {
        Dsp::SimpleFilter<Dsp::Butterworth::BandPass<order>, 1, Dsp::DirectFormI> f;
        uint32_t bw = bandwidth - start_frequency; // I know, bandwidth is such a bad naming for the cutoff freq when it comes to band-pass

        f.setup(order,                      // order
                this->input_rate,           // sample rate
                start_frequency + (bw / 2), // center frequency
                bw                          // center frequency
        );                                  // Rolloff

        Dsp::Cascade::Storage st = f.getCascadeStorage();
        dg = st.stageArray;
    }

    n_stages = (order + 1) / 2;

    // Convert to CMSIS format (output coefficients are negated)
    LOG("IIR Filter : type %d\n", type);
    // LOG("a=[%f,%f,%f]\n", dg[0].m_a0, dg[0].m_a1, dg[0].m_a2);
    // LOG("b=[%f,%f,%f]\n", dg[0].m_b0, dg[0].m_b1, dg[0].m_b2);
    LOG("rate %d, bw: %d, start_freq:%d\n", input_rate, bandwidth, start_frequency);

    coeffs[0] = dg[0].m_b0;
    coeffs[1] = dg[0].m_b1;
    coeffs[2] = dg[0].m_b2;
    coeffs[3] = -dg[0].m_a1;
    coeffs[4] = -dg[0].m_a2;

    if (n_stages == 2) {
        coeffs[5] = dg[1].m_b0;
        coeffs[6] = dg[1].m_b1;
        coeffs[7] = dg[1].m_b2;
        coeffs[8] = -dg[1].m_a1;
        coeffs[9] = -dg[1].m_a2;
    }

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
    arm_biquad_cascade_df1_init_f32(&iir_instance, n_stages, coeffs, state);
#endif
}

template <int order> bool DspIIRDecimator<order>::config(uint32_t input_rate, uint32_t cutoff_freq, uint16_t factor, filter_type type) {

    if (input_rate != this->input_rate || cutoff_freq != this->bandwidth || factor != this->factor || type != this->type) {
        this->input_rate = input_rate;
        this->bandwidth = cutoff_freq;
        this->factor = factor;
        this->type = type;

        this->init();
    }

    return true;
}

void test_iir_decimator() {

    DspIIRDecimator<4> d;

    d.config(10000, 5000, 8, LPF);
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
