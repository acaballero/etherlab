//
// Created by Angel Dust on 02/07/2024.
//

#include "dsp_signal_generator_processor.h"
#include "dsp/dsp_common.h"
#include "dsp/blocks/signal_generator.h"
#include "types.h"

void DspSignalGeneratorProcessor::work(const buffer_t<adc_type> *buffer) {
    this->status.processed_blocks++;
    buffer_t<complex_t> wrapped_buffer{(complex_t *)buffer->p, buffer->count};
    modulator.get_block(wrapped_buffer);

    int16_t *out_p = (int16_t *)buffer->p;

    for (size_t i = 0; i < buffer->count * 2; i += 2) {
        // TODO: Gain should be a generic and stackable block
        out_p[i] *= dsp::dsp_params->gain_factor;
        out_p[i + 1] *= dsp::dsp_params->gain_factor;
    }
}

void DspSignalGeneratorProcessor::set_config(uint32_t baseband_f, uint32_t mod_f, uint8_t mod_duty, uint32_t sample_rate, adc_type offset) {
    sine.set_config(baseband_f, sample_rate);
    pulse.set_config(mod_f, sample_rate);
    pulse.set_duty(mod_duty);
    modulator.set_dc_offset(offset);
}
