//
// Created by Angel Dust on 16/04/2021.
//

#include "dsp_buffers.h"
#include "buffer.hpp"
#include "config.h"
#include "dsp/dsp_common.h"

// ACD DMA buffer
ALIGN_32 adc_type adc_buff[DSP_ADC_BUFF_SIZE];

// DAC DMA buffer
ALIGN_32 adc_type dac_buff[DSP_ADC_BUFF_SIZE];

//__attribute__((section(".fccmram"))) // Can't be in CCM RAM if DMA is used
ALIGN_32 uint8_t dsp_output_fifo_buff[DSP_OUTPUT_FIFO_SIZE];
ALIGN_32 uint8_t dsp_input_fifo_buff[DSP_FIFO_SIZE];
// ALIGN_32 uint8_t tmp_buff[DSP_FIFO_SIZE >> 1];

FIFO output_stream((char *)dsp_output_fifo_buff, DSP_OUTPUT_FIFO_SIZE);
FIFO input_stream((char *)dsp_input_fifo_buff, DSP_FIFO_SIZE);

// Buffers wrapping to the ADC/DAC vectors
buffer_t<adc_type> adc_buffer_1 = {(adc_type *const)(adc_buff), DSP_BLOCK * 2, 0, COMPLEX_INTERLEAVED};
buffer_t<adc_type> adc_buffer_2 = {(adc_type *const)(adc_buff + DSP_BLOCK * 2), DSP_BLOCK * 2, 0, COMPLEX_INTERLEAVED};
buffer_t<adc_type> dac_buffer_1 = {(adc_type *const)(dac_buff), DSP_BLOCK * 2, 0, COMPLEX_INTERLEAVED};
buffer_t<adc_type> dac_buffer_2 = {(adc_type *const)(dac_buff + DSP_BLOCK * 2), DSP_BLOCK * 2, 0, COMPLEX_INTERLEAVED};
// buffer_t<adc_type> tmp_buffer = {(adc_type *const)(tmp_buff), DSP_BLOCK * 2, 0, COMPLEX_INTERLEAVED};

void reset_dac_buffer(adc_type value, uint16_t offset, uint16_t count) {
    for (int i = offset; i < offset + count; i++) {
        dac_buff[i] = value;
    }
}
