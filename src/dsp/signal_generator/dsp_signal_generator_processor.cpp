//
// Created by Angel Dust on 02/07/2024.
//

#include "dsp_signal_generator_processor.h"
#include "dsp/dsp_common.h"
#include "dsp/blocks/signal_generator.h"
#include "types.h"

void DspSignalGeneratorProcessor::work(const buffer_t<adc_type> *buffer) {
    this->info.processed_blocks++;
    buffer_t<complex_t> wrapped_buffer{(complex_t *)buffer->p, buffer->count / 2};
    modulator.get_block(wrapped_buffer);

    // for (int i = 0; i < buffer->count; i += 10) {

    //     LOG_RAW("%d ; %d\n", buffer->p[i], buffer->p[i + 1]);
    // }
}

void DspSignalGeneratorProcessor::set_config(uint32_t baseband_f, uint32_t mod_f, uint8_t mod_duty, uint32_t sample_rate) {
    baseband.set_config(baseband_f, sample_rate);
    pulse.set_config(mod_f, sample_rate);
    pulse.set_duty(mod_duty);
    modulator.set_modulation(&pulse);
    modulator.set_modulation_offset(0x7FF);
}

void DspSignalGeneratorProcessor::set_config(uint32_t baseband_f, uint32_t mod_f, dsp::SIGNAL_SHAPE shape, uint32_t sample_rate) {
    baseband.set_config(baseband_f, sample_rate);
    signal.set_config(mod_f, sample_rate);
    signal.set_shape(shape);
    modulator.set_modulation(&signal);
    modulator.set_modulation_offset(shape == dsp::SIGNAL_SHAPE_SIN ? 0 : 0x7FF);
}
