//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_PROCESSORS_H
#define TRX_FRONTEND_DSP_PROCESSORS_H

#include "dsp_processor.h"

namespace dsp {
enum DSP_PROCESSOR_ID {

    DSP_PROCESSOR_CAPTURE,
    DSP_PROCESSOR_REPLAY,
    DSP_PROCESSOR_SIGNAL_GENERATOR,
    DSP_PROCESSOR_RECEIVE,
    DSP_PROCESSOR_TRANSMIT,

};

extern const char *processorNames[];

extern DspProcessor *processors[];
} // namespace dsp
#endif // TRX_FRONTEND_DSP_PROCESSORS_H
