#ifndef __DSP_SOS_H__
#define __DSP_SOS_H__

#include "decimation/dsp_fir_decimator_float.h"
#include "fft/fft_types.h"
#include <cstdint>
#include <cstddef>

template <size_t N> class SOSFilter {
  public:
    //  void configure(const iir_biquad_df2_config_t config[N]) {
    //     for (size_t i = 0; i < N; i++)
    //         filters[i].configure(config[i]);
    // }

    float execute(float value) {
        for (auto &filter : filters) {
            // value = filter.execute(value);
        }
        return value;
    }

  private:
    DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS, float> filters[N];
};

#endif /*__DSP_SOS_H__*/
