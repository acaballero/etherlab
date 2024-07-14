//
// Created by Angel Dust on 21/04/2021.
//

#ifndef TRX_FRONTEND_DSP_FIR_DECIMATOR_FLOAT_H
#define TRX_FRONTEND_DSP_FIR_DECIMATOR_FLOAT_H

#include "stdio.h"
#include "dsp_decimator.h"
#include "dsp_fir_decimator_q15.h"
#include "dsp/dsp_buffers.h"
#include "dsp/fft/fft_types.h"


class DspFIRDecimatorFloat : public DspDecimator<float> {

public:

    DspFIRDecimatorFloat() : DspDecimator<float>(0) {};

    DspFIRDecimatorFloat(uint32_t input_rate, uint32_t output_rate, uint16_t factor)
    : DspDecimator<float>(input_rate, output_rate, factor)
    {
        this->initFilter();
    };

    void decimate(buffer_t <float> &src, buffer_t <float> &dst) override;
    void config(uint32_t input_rate, uint32_t output_rate, uint16_t factor);
    void clear_state();
    void decimate(buffer_t <float> &src, buffer_t <float> &dst, uint8_t start, uint8_t n_channels);
    bool isInitialized() const;
    void setFactor(uint16_t factor);

private:

    void initFilter();

    bool initialized=false;

    float firCoeffs[FFT_LPF_FIR_FILTER_NTAPS];
    float firStateBuffer[FFT_LPF_FIR_FILTER_NTAPS + DSP_BLOCK - 1];

    arm_fir_decimate_instance_f32 dsp_fir_decimate_instance = {
            1, FFT_LPF_FIR_FILTER_NTAPS, firCoeffs, firStateBuffer
    };

};


#endif //TRX_FRONTEND_DSP_FIR_DECIMATOR_FLOAT_H
