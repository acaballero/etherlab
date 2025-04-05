//
// Created by Angel Dust on 04/04/2025.
//

#ifndef TRX_FRONTEND_DSP_RECEIVE_PROCESSOR_H
#define TRX_FRONTEND_DSP_RECEIVE_PROCESSOR_H

#include "dsp/dsp_processor.h"
#include "types.h"

class DspReceiveProcessor : public DspProcessor {

  public:
    DspReceiveProcessor() {
        // This processor takes samples from the ACD and puts samples in the DAC and the other way around (so DSP_DIRECTION_INOUT)
        this->status.direction = DSP_DIRECTION_INOUT;
    }

    void work(const buffer_t<complex_t> *buffer) override;

  protected:
};

#endif // TRX_FRONTEND_DSP_RECEIVE_PROCESSOR_H
