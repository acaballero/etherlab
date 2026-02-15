//
// Created by Angel Dust on 02/07/2024.
//

#ifndef TRX_FRONTEND_DSP_SIGNAL_GENERATOR_PROCESSOR_H
#define TRX_FRONTEND_DSP_SIGNAL_GENERATOR_PROCESSOR_H

#include "dsp/dsp_common.h"
#include "dsp/dsp_processor.h"
#include "dsp/blocks/signal_generator.h"
#include "dsp/blocks/modulator.h"
#include "dsp/blocks/pulse_generator.h"
#include "types.h"

class DspSignalGeneratorProcessor : public DspProcessor {

  public:
    DspSignalGeneratorProcessor() : modulator(&signal, &baseband) {
        this->info.direction = DSP_DIRECTION_OUT;
    }

    void set_config(uint32_t baseband_f, uint32_t mod_f, uint8_t mod_duty, uint32_t sample_rate);
    void set_config(uint32_t baseband_f, uint32_t mod_f, dsp::SIGNAL_SHAPE shape, uint32_t sample_rate);
    void work(const buffer_t<adc_type> *buffer) override;

    bool wait_first_block() override {
        return false;
    }

  protected:
    dsp::SignalGenerator baseband;
    dsp::SignalGenerator signal;
    dsp::PulseGenerator pulse;

    dsp::Modulator modulator;
};
#endif // TRX_FRONTEND_DSP_SIGNAL_GENERATOR_PROCESSOR_H
