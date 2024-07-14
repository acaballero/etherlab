//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_CAPTURE_PROCESSOR_H
#define TRX_FRONTEND_DSP_CAPTURE_PROCESSOR_H

#include "dsp/dsp_processor.h"
#include "dsp/blocks/dc_block.h"

class DspCaptureProcessor : public DspProcessor {
public:

    DspCaptureProcessor() {

        this->status.direction = DSP_DIRECTION_IN;
        this->status.n_channels = 2;
    }

    void work(const buffer_t<complex_t> *buffer) override;

private:

    DCBlock dc_blocker_i {.995};
    DCBlock dc_blocker_q {.995};

};

#endif //TRX_FRONTEND_DSP_CAPTURE_PROCESSOR_H
