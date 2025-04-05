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

template <int TAPS = FFT_LPF_FIR_FILTER_NTAPS, typename T = float> class DspFIRDecimatorFloat : public DspDecimator<T> {

  public:
    DspFIRDecimatorFloat() : DspDecimator<T>(0){};

    DspFIRDecimatorFloat(uint32_t input_rate, uint32_t output_rate, uint16_t factor) : DspDecimator<T>(input_rate, output_rate, factor) { this->initFilter(); };

    void decimate(buffer_t<T> &src, buffer_t<T> &dst) override;
    void config(uint32_t input_rate, uint32_t output_rate, uint16_t factor);
    void clear_state();
    void decimate(buffer_t<T> &src, buffer_t<T> &dst, uint8_t start, uint8_t n_channels);
    bool isInitialized() const;
    void setFactor(uint16_t factor);

  private:
    void initFilter();

    bool initialized = false;

    float firCoeffs[TAPS];
    float firStateBuffer[TAPS + DSP_BLOCK - 1];

    arm_fir_decimate_instance_f32 dsp_fir_decimate_instance = {1, TAPS, firCoeffs, firStateBuffer};
};

#endif // TRX_FRONTEND_DSP_FIR_DECIMATOR_FLOAT_H
