//
// Created by Angel Dust on 21/04/2021.
//

#ifndef TRX_FRONTEND_DSP_IIR_DECIMATOR_H
#define TRX_FRONTEND_DSP_IIR_DECIMATOR_H

#include "dsp_decimator.h"
#include "dsp_iir_decimator.h"
#include "stdio.h"
#include "stddef.h"
#include <sys/_stdint.h>

template <int order = 2> class DspIIRDecimator : public DspDecimator<int16_t> {

  public:
    DspIIRDecimator() : DspDecimator<int16_t>(0){};

    DspIIRDecimator(uint32_t factor) : DspDecimator<int16_t>(factor){};

    DspIIRDecimator(uint32_t input_rate, uint32_t output_rate, uint16_t factor) : DspDecimator<int16_t>(input_rate, output_rate, factor) {

        this->init();
    };

    bool config(uint32_t input_rate, uint32_t output_rate, uint16_t factor);

    void decimate(buffer_t<int16_t> &src, buffer_t<int16_t> &dst) override;

    void decimate(buffer_t<int16_t> &src, buffer_t<int16_t> &dst, uint8_t channel_n, uint8_t n_channels_in, uint8_t n_channels_out);

    void decimate(buffer_t<float32_t> &src, buffer_t<float32_t> &dst, uint8_t channel_n, uint8_t n_channels_in, uint8_t n_channels_out);

  private:
    void init();

    float32_t coeffs[10];

    arm_biquad_casd_df1_inst_f32 iir_instance;
    int n_stages;
    float state[8];
};

void test_iir_decimator();

#endif // TRX_FRONTEND_DSP_IIR_DECIMATOR_H
