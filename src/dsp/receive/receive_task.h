//
// Created by Angel Dust on 04/04/2025.
//

#ifndef TRX_FRONTEND_RECEIVE_TASK_H
#define TRX_FRONTEND_RECEIVE_TASK_H

#include <memory>
#include <sys/_stdint.h>
#include "dsp/decimation/dsp_fir_decimator_q15.h"
#include "dsp/dsp_common.h"
#include "dsp/task.h"
#include "types.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include "../decimation/dsp_fir_decimator_float.h"
#include "dsp/modulation/dsp_demodulate.hpp"
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
    DspFIRDecimatorQ15<FIR_DECIMATOR_1ST_HALFBAND_TAPS, complex_t> decimators[max_decimators];
    // Signal decimator. The last narrowband signal decimator
    DspFIRDecimatorQ15<FIR_DECIMATOR_SIGNAL_TAPS, complex_t> signal_decimator;
    DCBlock dc_block_i{0.98};
    DCBlock dc_block_q{0.98};
    std::unique_ptr<dsp::demodulator> demodulator;
    std::unique_ptr<dsp::demodulator> get_modulator();
    bool init_decimators();
    uint8_t n_decimators;
};

#endif // TRX_FRONTEND_RECEIVE_TASK_H
