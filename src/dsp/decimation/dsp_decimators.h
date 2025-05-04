//
// Created by Angel Dust on 21/04/2021.
//

#ifndef TRX_FRONTEND_DSP_DECIMATORS_H
#define TRX_FRONTEND_DSP_DECIMATORS_H

#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "dsp/fft/fft_types.h"
#include "dsp_iir_decimator.h"

extern DspIIRDecimator<4> IIRDecimator_I;
extern DspIIRDecimator<4> IIRDecimator_Q;

#endif // TRX_FRONTEND_DSP_DECIMATORS_H
