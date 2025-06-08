//
// Created by Angel Dust on 02/07/2024.
//

#ifndef TRX_FRONTEND_SIGNAL_GENERATOR_TASK_H
#define TRX_FRONTEND_SIGNAL_GENERATOR_TASK_H

#include <memory>
#include "dsp/task.h"
#include "types.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"

class SignalGeneratorTask : public Task {

  public:
    using Task::Task;

    void work() override;

    bool start() override;

    void stop() override;

    // TX: DAC output is downconverted and injected in the analog TX chain
    // RF: DAC output goes to the audio chain
    RF_DIRECTION mode = RF_DIRECTION_TX;

  private:
};

#endif // TRX_FRONTEND_SIGNAL_GENERATOR_TASK_H
