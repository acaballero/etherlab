//
// Created by Angel Dust on 02/07/2024.
//

#ifndef TRX_FRONTEND_SIGNAL_GENERATOR_TASK_H
#define TRX_FRONTEND_SIGNAL_GENERATOR_TASK_H

#include <memory>
#include "dsp/task.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"


class SignalGeneratorTask : public Task {

public:

    SignalGeneratorTask(void (*onSucess)(), void (*onError)(DSP_ERROR));

    void work() override;

    void start() override;

    void stop() override;

private:


};

#endif //TRX_FRONTEND_SIGNAL_GENERATOR_TASK_H
