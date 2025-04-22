//
// Created by Angel Dust on 04/04/2025.
//

#ifndef TRX_FRONTEND_RECEIVE_TASK_H
#define TRX_FRONTEND_RECEIVE_TASK_H

#include <memory>
#include "dsp/buffer.hpp"
#include "dsp/decimation/dsp_fir_decimator_q15.h"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/task.h"
#include "types.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "dsp/modulation/dsp_demodulate.h"
#include "dsp/blocks/dc_block.h"

class ReceiveTask : public Task {

  public:
    static constexpr uint32_t audio_bw_hz = 48000; // Audio bandwidth
    static constexpr uint8_t max_decimators = 2;   // Max number of cascaded decimators

    ReceiveTask(void (*onSucess)(), void (*onError)(DSP_ERROR));

    void work() override;

    bool start() override;

    void stop() override;

  private:
    // Cascaded decimators
    DspFIRDecimatorFloat<FIR_DECIMATOR_1ST_HALFBAND_TAPS, complex_t_f32> decimators[max_decimators];
    // Signal decimator. The last narrowband signal decimator
    DspFIRDecimatorFloat<FIR_DECIMATOR_SIGNAL_TAPS, complex_t_f32> signal_decimator;
    DCBlock dc_block_i{0.98};
    DCBlock dc_block_q{0.98};
    std::unique_ptr<dsp::demodulator> demodulator;
    std::unique_ptr<dsp::demodulator> get_modulator();

    float32_t tmp_buff_data[DSP_BLOCK * 4 * 2];

    // 4 temp buffers are used to purposedly avoid overlapping buffers or in-place decimation processing in the hope (is it worth it?) that the compiler
    // is able to fully optimize the loops with instruction reordering
    float32_t *bi1_p = tmp_buff_data;
    float32_t *bq1_p = tmp_buff_data + DSP_BLOCK * 2;
    float32_t *bi2_p = tmp_buff_data + DSP_BLOCK * 4;
    float32_t *bq2_p = tmp_buff_data + DSP_BLOCK * 6;

    buffer_t<float32_t> tmp_buff{bi1_p, DSP_BLOCK};

    bool init_decimators();
    uint8_t n_decimators;
};

#endif // TRX_FRONTEND_RECEIVE_TASK_H
