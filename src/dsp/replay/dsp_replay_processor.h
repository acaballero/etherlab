//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_REPLAY_PROCESSOR_H
#define TRX_FRONTEND_DSP_REPLAY_PROCESSOR_H

#include "dsp/dsp_processor.h"

#define DSP_REPLAY_DEBUG 0

class DspReplayProcessor : public DspProcessor {

public:

    DspReplayProcessor() {
        this->status.direction = DSP_DIRECTION_OUT;
    }


    void work(const buffer_t<complex_t> *buffer) override;


};
#endif //TRX_FRONTEND_DSP_REPLAY_PROCESSOR_H
