//
// Created by Angel Dust on 04/04/2025.
//

#include "dsp_receive_processor.h"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/blocks/signal_generator.h"
#include "tinyusb/tusb_config.h"
#include "tinyusb/usb_audio_dsp_bridge.h"
#include "tinyusb/usb_composite_device.h"
#include "types.h"
#include "printf.h"

void DspReceiveProcessor::work(const buffer_t<adc_type> *buffer) {

    int16_t *p;

    // TODO: Find some other way of making this processor know whether is reading or writing
    if (buffer->p == adc_buffer_1.p || buffer->p == adc_buffer_2.p) {

        if (info.status != DSP_STATUS_RUNNING) {
            return;
        }

        uint16_t block_size_bytes = buffer->size_bytes;

        // GPIOD->BSRR |= GPIO_PIN_9;
        uint32_t free = input_stream.free((char **)&p);

        if (free >= block_size_bytes) {
            info.processed_blocks++;
            input_stream.write_block((char *)buffer->p, buffer->size_bytes);
        } else {
            info.fifo_overruns++;
        }
        // GPIOD->BSRR |= GPIO_PIN_9 << 16;
    } else {
        if (info.status != DSP_STATUS_RUNNING) {
            memset((char *)buffer->p, 0, buffer->count << 1);
            return;
        }

        uint16_t block_size_bytes = buffer->size_bytes / 2;

        //  GPIOD->BSRR |= GPIO_PIN_9;
        uint32_t av = output_stream.available((char **)&p);

        if (av >= block_size_bytes) {

            int16_t *out_p = (int16_t *)buffer->p;

            // DAC output
            for (size_t i = 0; i < buffer->count / 2; i++) {
                out_p[i * 2] = ((int16_t *)p)[i];
            }

            if (usb_audio_is_streaming(ITF_IX_MICROPHONE)) {
                // Apply gain

                // Note the buffer sample rate must be a divisor of the sample rate of the required for the USB so we can do fast interpolation
                usb_audio_send(p, buffer->count / 2, info.sample_rate);
            }

            output_stream.consume(block_size_bytes, (char **)&p);

        } else if (info.processed_blocks) {
            // Underruns will surely happen at the start of the process
            memset((char *)buffer->p, 0, buffer->count << 1);
            info.fifo_underruns++;
        }
        // GPIOD->BSRR |= GPIO_PIN_9 << 16;
    }

    // Error rate
}
