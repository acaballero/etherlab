//
// Created by Angel Dust on 16/04/2021.
//

#include "dsp_processors.h"
#include "dsp/capture/dsp_capture_processor.h"
#include "dsp/replay/dsp_replay_processor.h"
#include "dsp/signal_generator/dsp_signal_generator_processor.h"


DspCaptureProcessor captureProcessor;
DspReplayProcessor replayProcessor;
DspSignalGeneratorProcessor signalGeneratorProcessor;

DspProcessor *processors[] {
    &captureProcessor,
    &replayProcessor,
    &signalGeneratorProcessor
};