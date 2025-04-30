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

class DspIIRDecimator : public DspDecimator<int16_t> {

  public:
    DspIIRDecimator() : DspDecimator<int16_t>(0){};

    DspIIRDecimator(uint32_t factor) : DspDecimator<int16_t>(factor){};

    DspIIRDecimator(uint32_t input_rate, uint32_t output_rate, uint16_t factor) : DspDecimator<int16_t>(input_rate, output_rate, factor) {
        this->initFilter();
    };

    bool config(uint32_t input_rate, uint32_t output_rate, uint16_t factor, uint32_t start_freq = 0);

    void decimate(buffer_t<int16_t> &src, buffer_t<int16_t> &dst) override;

    void decimate(buffer_t<int16_t> &src, buffer_t<int16_t> &dst, uint8_t channel_n, uint8_t n_channels_in, uint8_t n_channels_out);

  private:
    void initFilter();

    // IIR  LPF Filter coefficients
#if DSP_USE_IIR_Q15
    float32_t IIRFilterCoefficients[12] = {

        0.003918013880209662, 0, 0.007836027760419324, 0.003918013880209662, -1.786742340761579010, 0.802414396282417730,
        // b0,0, b1, b2, a1, a2
        0.007881382781770929, 0, 0.015762765563541857, 0.007881382781770929, -1.881929281613502040, 0.913454812740585820
        // b0,0, b1, b2, a1, a2

    };
#else
    float32_t IIRFilterCoefficients[10];
#endif

    const int IIRFilterNumStages = 2;

#if DSP_USE_IIR_Q15
    arm_biquad_casd_df1_inst_q15 iir_instance_I;
    q15_t IIRStateBufferI[8];
#else
    arm_biquad_casd_df1_inst_f32 iir_instance;
    float IIRStateBuffer[8];
#endif
};

void test_iir_decimator();

#endif // TRX_FRONTEND_DSP_IIR_DECIMATOR_H
