//
// Created by Angel Dust on 16/04/2021.
//

#include "dsp_buffers.h"
#include "buffer.hpp"
#include "dsp/dsp_common.h"

// ACD DMA buffer
ALIGN_32 adc_type adc_buff[DSP_ADC_BUFF_SIZE];

// DAC DMA buffer
ALIGN_32 adc_type dac_buff[DSP_ADC_BUFF_SIZE];

//__attribute__((section(".fccmram"))) // Can't be in CCM RAM if DMA is used
ALIGN_32 uint8_t dsp_output_fifo_buff[DSP_OUTPUT_FIFO_SIZE];
ALIGN_32 uint8_t dsp_input_fifo_buff[DSP_FIFO_SIZE];

FIFO output_stream((char *)dsp_output_fifo_buff, DSP_OUTPUT_FIFO_SIZE);
FIFO input_stream((char *)dsp_input_fifo_buff, DSP_FIFO_SIZE);

// Buffers wrapping to the ADC/DAC vectors
buffer_t<adc_type> adc_buffer_1 = {(adc_type *const)(adc_buff), DSP_BLOCK * 2, 0, COMPLEX_INTERLEAVED};
buffer_t<adc_type> adc_buffer_2 = {(adc_type *const)(adc_buff + DSP_BLOCK * 2), DSP_BLOCK * 2, 0, COMPLEX_INTERLEAVED};
buffer_t<adc_type> dac_buffer_1 = {(adc_type *const)(dac_buff), DSP_BLOCK * 2, 0, COMPLEX_INTERLEAVED};
buffer_t<adc_type> dac_buffer_2 = {(adc_type *const)(dac_buff + DSP_BLOCK * 2), DSP_BLOCK * 2, 0, COMPLEX_INTERLEAVED};
