//
// Created by Angel Dust on 16/04/2021.
//

#include "dsp_processors.h"
#include "dsp/capture/dsp_capture_processor.h"
#include "dsp/receive/dsp_receive_processor.h"
#include "dsp/replay/dsp_replay_processor.h"
#include "dsp/signal_generator/dsp_signal_generator_processor.h"
#include "dsp/transmit/dsp_transmit_processor.h"

namespace dsp {
DspCaptureProcessor captureProcessor;
DspReplayProcessor replayProcessor;
DspSignalGeneratorProcessor signalGeneratorProcessor;
DspReceiveProcessor receiveProcessor;
DspTransmitProcessor transmitProcessor;
const char *processorNames[] = {"Capture", "Replay", "Signal generator", "Receive", "Transmit"};
DspProcessor *processors[]{&captureProcessor, &replayProcessor, &signalGeneratorProcessor, &receiveProcessor, &transmitProcessor};
} // namespace dsp
