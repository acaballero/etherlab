//
// Created by Angel Dust on 21/01/2026.
//

#ifndef TRX_TRANSMIT_PROCESSOR_H
#define TRX_TRANSMIT_PROCESSOR_H

#include "dsp/dsp_processor.h"
#include "types.h"

class DspTransmitProcessor : public DspProcessor {

  public:
    DspTransmitProcessor() {
        // This processor takes samples from the USB and puts samples in the DAC
        this->info.direction = DSP_DIRECTION_OUT;
    }

    void work(const buffer_t<adc_type> *buffer) override;
};

#endif // TRX_TRANSMIT_PROCESSOR_H
