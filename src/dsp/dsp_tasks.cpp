//
// Created by Angel Dust on 16/04/2021.
//

#include "dsp_tasks.h"
#include "dsp/replay/replay_task.h"
#include "dsp/capture/capture_task.h"
#include "dsp.h"
#include "dsp/signal_generator/signal_generator_task.h"

CaptureTask captureTask(dspSuccess, dspError);
ReplayTask replayTask(dspSuccess, dspError);
SignalGeneratorTask signalGeneratorTask(dspSuccess, dspError);

namespace dsp {

Task *tasks[]{&captureTask, &replayTask, &signalGeneratorTask};
} // namespace dsp
