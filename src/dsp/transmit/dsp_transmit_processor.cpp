//
// Created by Angel Dust on 21/01/2026.
//

#include "dsp_transmit_processor.h"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/blocks/signal_generator.h"
#include "tinyusb/usb_audio_dsp_bridge.h"
#include "types.h"
#include "printf.h"
#include <sys/_stdint.h>

void DspTransmitProcessor::work(const buffer_t<adc_type> *buffer) {

    int16_t *p;

    if (status.status != DSP_STATUS_RUNNING) {
        memset((char *)buffer->p, 0, buffer->count << 1);
        return;
    }

    uint16_t block_size_bytes = buffer->size_bytes;

    //  GPIOD->BSRR |= GPIO_PIN_9;

    int32_t av = output_stream.available((char **)&p);

    if (av >= block_size_bytes) {

        int16_t *out_p = (int16_t *)buffer->p;

        // DAC output
        for (size_t i = 0; i < buffer->count; i += 2) {
            *(out_p++) = ((int16_t *)p)[i];
            *(out_p++) = ((int16_t *)p)[i + 1];
        }

        output_stream.consume(block_size_bytes, (char **)&p);

    } else if (status.processed_blocks) {
        // Underruns will surely happen at the start of the process
        status.fifo_underruns++;
    }
    // GPIOD->BSRR |= GPIO_PIN_9 << 16;
}
