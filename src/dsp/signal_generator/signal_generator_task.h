//
// Created by Angel Dust on 02/07/2024.
//

#ifndef TRX_FRONTEND_SIGNAL_GENERATOR_TASK_H
#define TRX_FRONTEND_SIGNAL_GENERATOR_TASK_H

#include <memory>
#include "dsp/signal_generator/dsp_signal_generator_processor.h"
#include "dsp/task.h"
#include "types.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"

class SignalGeneratorTask : public Task {

  public:
    using Task::Task;

    void work() override;

    bool start_impl() override;

    void stop() override;

    // TX: DAC output is downconverted and injected in the analog TX chain
    // RX: DAC output goes to the audio chain
    RF_DIRECTION mode = RF_DIRECTION_TX;

  protected:
    std::unique_ptr<DspProcessor> create_processor() override {
        return std::make_unique<DspSignalGeneratorProcessor>();
    }
};

#endif // TRX_FRONTEND_SIGNAL_GENERATOR_TASK_H
