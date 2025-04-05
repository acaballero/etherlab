//
// Created by Angel Dust on 16/04/2021.
//

#include "dsp_buffers.h"
#include "buffer.hpp"

// ACD DMA buffer
complex_t adc_buff[DSP_BLOCK * 2];

// ACD DAC buffer
complex_t dac_buff[DSP_BLOCK * 2];

__attribute__((section(".fccmram"))) adc_type dsp_temp_buf_data[DSP_BLOCK * 2]; // complex_t is twice the size of adc_type (I/Q samples), so a temp buffer to
                                                                                // store only one half of the adc_buff is DSP_BLOCK*2 adc_type items
buffer_t<adc_type> dsp_temp_buf = {(adc_type *const)dsp_temp_buf_data, DSP_BLOCK * 2, 0};

//__attribute__((section(".fccmram"))) // Can't be in CCM RAM if DMA is used
uint8_t dsp_output_fifo_buff[DSP_FIFO_SIZE];
uint8_t dsp_input_fifo_buff[DSP_FIFO_SIZE];

FIFO output_stream((char *)dsp_output_fifo_buff, DSP_FIFO_SIZE);
FIFO input_stream((char *)dsp_input_fifo_buff, DSP_FIFO_SIZE);

// Buffers wrapping to the ADC/DAC vectors
buffer_t<complex_t> adc_buffer_1 = {(complex_t *const)(adc_buff), DSP_BLOCK};
buffer_t<complex_t> adc_buffer_2 = {(complex_t *const)(adc_buff + DSP_BLOCK), DSP_BLOCK};
buffer_t<complex_t> dac_buffer_1 = {(complex_t *const)(dac_buff), DSP_BLOCK};
buffer_t<complex_t> dac_buffer_2 = {(complex_t *const)(dac_buff + DSP_BLOCK), DSP_BLOCK};
