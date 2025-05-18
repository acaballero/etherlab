//
// Created by Angel Dust on 04/04/2025.
//

#ifndef TRX_FRONTEND_RECEIVE_TASK_H
#define TRX_FRONTEND_RECEIVE_TASK_H

#include <memory>
#include "dsp/buffer.hpp"
#include "dsp/decimation/dsp_iir_decimator.h"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/task.h"
#include "types.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include "receive_task_base.h"
#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "dsp/decimation/dsp_fir_decimator_float_complex.h"
#include "dsp/modulation/dsp_demodulate.h"
#include "dsp/blocks/dc_block.h"
#include "dsp/audio/audio_compressor.hpp"

class ReceiveTask : public ReceiveTaskBase {

  public:
    ReceiveTask(void (*onSucess)(), void (*onError)(DSP_ERROR));

  private:
    // De-empth filter
    DspIIRDecimator<1> deemph_filter;
    bool deemph_enabled = false;
    // Audio low-pass filter
    // DspIIRDecimator<1> audio_lpf;

    FeedForwardCompressor compressor;
    bool compressor_enabled = false;

    bool init() override;
    void process_audio(buffer_t<float32_t> &buff_out_f32) override;
    MODULATION_MODE get_modulation_mode() override;
};

#endif // TRX_FRONTEND_RECEIVE_TASK_H
