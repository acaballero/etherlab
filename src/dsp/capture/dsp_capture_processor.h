//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_CAPTURE_PROCESSOR_H
#define TRX_FRONTEND_DSP_CAPTURE_PROCESSOR_H

#include "dsp/dsp_processor.h"
#include "dsp/blocks/dc_block.h"
#include "dsp/decimation/dsp_fir_decimator_float.h"

class DspCaptureProcessor : public DspProcessor {
  public:
    DspCaptureProcessor() {

        this->status.direction = DSP_DIRECTION_IN;
        this->status.n_channels = 2;
    }

    bool start() override;
    void work(const buffer_t<complex_t> *buffer) override;

  private:
    static constexpr int samples_per_batch =
        DSP_BLOCK; // Note  decimators and processor buffers are configured for a block size of DSP_BLOCK. Don't use bigger blocks or memory will be corrupted

    float32_t tmp_buff_data[samples_per_batch * 4];

    float32_t *bi1_p = tmp_buff_data;
    float32_t *bq1_p = tmp_buff_data + samples_per_batch;
    float32_t *bi2_p = tmp_buff_data + samples_per_batch * 2;
    float32_t *bq2_p = tmp_buff_data + samples_per_batch * 3;

    DspFIRDecimatorFloat<FIR_DECIMATOR_1ST_HALFBAND_TAPS, complex_t_f32> decimator;

    DCBlock dc_blocker_i{.999};
    DCBlock dc_blocker_q{.999};
};

#endif // TRX_FRONTEND_DSP_CAPTURE_PROCESSOR_H
