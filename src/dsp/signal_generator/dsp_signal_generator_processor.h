//
// Created by Angel Dust on 02/07/2024.
//

#ifndef TRX_FRONTEND_DSP_SIGNAL_GENERATOR_PROCESSOR_H
#define TRX_FRONTEND_DSP_SIGNAL_GENERATOR_PROCESSOR_H

#include "dsp/dsp_processor.h"
#include "dsp/blocks/signal_generator.h"
#include "dsp/blocks/ook_modulator.h"
#include "dsp/blocks/pulse_generator.h"

class DspSignalGeneratorProcessor : public DspProcessor {

public:

    DspSignalGeneratorProcessor() : modulator(&pulse, &sine) {
        this->status.direction = DSP_DIRECTION_OUT;
    }

    void set_config(uint32_t baseband_f, uint32_t mod_f, uint8_t mod_duty, uint32_t sample_rate, adc_type dc_offset);
    void work(const buffer_t<complex_t> *buffer) override;

protected:

    SignalGenerator sine;
    PulseGenerator pulse;
    OOKModulator modulator;
};

#endif //TRX_FRONTEND_DSP_SIGNAL_GENERATOR_PROCESSOR_H
