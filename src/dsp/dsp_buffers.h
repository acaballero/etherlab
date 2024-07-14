//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_BUFFERS_H
#define TRX_FRONTEND_DSP_BUFFERS_H

#include "dsp_common.h"
#include "buffer.hpp"

// DSP processing block size. Defines the number of samples adquired in each DMA cycle
#define DSP_BLOCK 16
// ACD FIFO buffer
#define DSP_FIFO_BLOCK_BYTES  512*48 // write 12 sectors at a time to the SD_CARD
#define DSP_FIFO_SIZE DSP_FIFO_BLOCK_BYTES*3 // Must be multiple of DSP_FIFO_BLOCK_BYTES
// ACD DMA buffer
extern complex_t adc_buff[DSP_BLOCK * 2];

// ACD DAC buffer
extern complex_t dac_buff[DSP_BLOCK * 2];


extern buffer_t<adc_type> dsp_temp_buf;

//__attribute__((section(".fccmram"))) // Can't be in CCM RAM if DMA is used
extern uint8_t dsp_fifo_buff[DSP_FIFO_SIZE];

extern buffer_t<complex_t> adc_buffer_1;
extern buffer_t<complex_t> adc_buffer_2;
extern buffer_t<complex_t> dac_buffer_1;
extern buffer_t<complex_t> dac_buffer_2;

extern FIFO fifo;

#endif //TRX_FRONTEND_DSP_BUFFERS_H

