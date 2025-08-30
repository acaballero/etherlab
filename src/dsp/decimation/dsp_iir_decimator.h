//
// Created by Angel Dust on 21/04/2021.
//

#ifndef TRX_FRONTEND_DSP_IIR_DECIMATOR_H
#define TRX_FRONTEND_DSP_IIR_DECIMATOR_H

#include "dsp/fir_filter.h"
#include "dsp_decimator.h"
#include "dsp_iir_decimator.h"
#include "stdio.h"
#include "stddef.h"

template <int order = 2> class DspIIRDecimator : public DspDecimator<int16_t> {

  public:
    DspIIRDecimator() : DspDecimator<int16_t>(0){};

    DspIIRDecimator(uint32_t factor) : DspDecimator<int16_t>(factor){};

    DspIIRDecimator(uint32_t input_rate, uint32_t output_rate, uint16_t factor) : DspDecimator<int16_t>(input_rate, output_rate, factor), type{LPF} {
        this->init();
    };
    bool config(uint32_t input_rate, uint32_t bandwidth, uint16_t factor, uint32_t start_f = 0) override {
        start_frequency = start_f;
        config(input_rate, bandwidth, factor, start_f ? BPF : LPF);
    }
    bool config(uint32_t input_rate, uint32_t cutoff_freq, uint16_t factor = 1, filter_type type = LPF);

    void decimate(buffer_t<int16_t> &src, buffer_t<int16_t> &dst) override;
    void decimate(int16_t *src_i, int16_t *src_q, int16_t *dst_i, int16_t *dst_q, size_t n_samples) override;
    void decimate(const buffer_t<int16_t> &src, buffer_t<int16_t> &dst, const uint8_t channel_n, const uint8_t n_channels_in, const uint8_t n_channels_out);

    void decimate(const buffer_t<float32_t> &src, buffer_t<float32_t> &dst, const uint8_t channel_n, const uint8_t n_channels_in, const uint8_t n_channels_out);

  private:
    void init();

    float32_t coeffs[10];

    arm_biquad_casd_df1_inst_f32 iir_instance;
    int n_stages;
    float state[8];
    filter_type type;
    uint32_t start_frequency; // Start frequency for the band-pass case
};

void test_iir_decimator();

#endif // TRX_FRONTEND_DSP_IIR_DECIMATOR_H
