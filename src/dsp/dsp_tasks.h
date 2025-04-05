//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_TASKS_H
#define TRX_FRONTEND_DSP_TASKS_H

#include "task.h"

namespace dsp {
enum DSP_TASK_ID {

    DSP_TASK_CAPTURE,
    DSP_TASK_REPLAY,
    DSP_TASK_SIGNAL_GENERATOR,
    DSP_TASK_RECEIVE

};

extern Task *tasks[];
} // namespace dsp
#endif // TRX_FRONTEND_DSP_TASKS_H
