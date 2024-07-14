//
// Created by Angel Dust on 21/04/2021.
//

#ifndef TRX_FRONTEND_DSP_FIR_DECIMATOR_Q15_H
#define TRX_FRONTEND_DSP_FIR_DECIMATOR_Q15_H

#include "stdio.h"
#include "dsp_decimator.h"
#include "dsp/dsp_buffers.h"
#include "dsp/fft/fft_types.h"


class DspFIRDecimatorQ15 : public DspDecimator<int16_t> {

public:

    DspFIRDecimatorQ15() : DspDecimator<int16_t>(0) {};

    DspFIRDecimatorQ15(uint32_t input_rate, uint32_t output_rate, uint16_t factor)
    : DspDecimator<int16_t>(input_rate, output_rate, factor)
    {
        this->initFilter();
    };

    void decimate(buffer_t <int16_t> &src, buffer_t <int16_t> &dst) override;
    void config(uint32_t input_rate, uint32_t output_rate, uint16_t factor);
    void decimate(buffer_t <int16_t> &src, buffer_t <int16_t> &dst, uint8_t start, uint8_t n_channels);

private:

    void initFilter();

    q15_t dsp_firCoeffs15[FFT_LPF_FIR_FILTER_NTAPS];

    q15_t dsp_firStateBuffer[FFT_LPF_FIR_FILTER_NTAPS + DSP_BLOCK - 1];

    arm_fir_decimate_instance_q15 dsp_fir_decimate_instance = {
            1, FFT_LPF_FIR_FILTER_NTAPS, dsp_firCoeffs15, dsp_firStateBuffer
    };

};


#endif //TRX_FRONTEND_DSP_FIR_DECIMATOR_Q15_H
