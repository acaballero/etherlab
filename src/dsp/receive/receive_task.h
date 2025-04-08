//
// Created by Angel Dust on 04/04/2025.
//

#ifndef TRX_FRONTEND_RECEIVE_TASK_H
#define TRX_FRONTEND_RECEIVE_TASK_H

#include <memory>
#include <sys/_stdint.h>
#include "dsp/decimation/dsp_fir_decimator_q15.h"
#include "dsp/task.h"
#include "types.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include "../decimation/dsp_fir_decimator_float.h"

class ReceiveTask : public Task {

  public:
    static constexpr uint32_t audio_bw_hz = 48000; // Audio bandwidth

    ReceiveTask(void (*onSucess)(), void (*onError)(DSP_ERROR));

    void work() override;

    void start() override;

    void stop() override;

  private:
    DspFIRDecimatorFloat<24, adc_type> decimator_i_0{};
    DspFIRDecimatorFloat<32, adc_type> decimator_i_1{};
};

#endif // TRX_FRONTEND_RECEIVE_TASK_H
