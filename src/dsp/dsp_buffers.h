//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_BUFFERS_H
#define TRX_FRONTEND_DSP_BUFFERS_H

#include "dsp_common.h"
#include "buffer.hpp"

// DSP processing block size. Defines the number of samples adquired in each DMA cycle
#define DSP_BLOCK 64

// Must be a multiple of any chunk that a stream processor taks will try to read
// For example, if the capture task needs to write 1024 bytes per block and the receive task 64 bytes, that's ok.
// If one task needs,for example, 512 bytes and another 1500, the FIFO block cannot be either 1500 or 3000
// TODO: This is a consequence of using a FIFO of contiguous memory blocks. A better approach is using a FIFO of memory "buckets", so the fifo contains
// pointers to memory blocks of arbirary size. However, note that the current implementation ensures all fifo operations are O(1) and very fast.
#define DSP_FIFO_BLOCK_BYTES (512) * 8         // 512 is the default SD sector size
#define DSP_FIFO_SIZE DSP_FIFO_BLOCK_BYTES * 4 // Must be multiple of DSP_FIFO_BLOCK_BYTES
#define DSP_OUTPUT_FIFO_SIZE DSP_FIFO_BLOCK_BYTES * 2
// ACD DMA buffer
extern complex_t adc_buff[DSP_BLOCK * 2];

// DAC DMA buffer
extern complex_t dac_buff[DSP_BLOCK * 2];

extern buffer_t<adc_type> dsp_temp_buf;

//__attribute__((section(".fccmram"))) // Can't be in CCM RAM if DMA is used
extern uint8_t dsp_output_fifo_buff[DSP_OUTPUT_FIFO_SIZE];
extern uint8_t dsp_input_fifo_buff[DSP_FIFO_SIZE];

// Half-DMA buffer wrappers
extern buffer_t<complex_t> adc_buffer_1;
extern buffer_t<complex_t> adc_buffer_2;
extern buffer_t<complex_t> dac_buffer_1;
extern buffer_t<complex_t> dac_buffer_2;

extern FIFO output_stream;
extern FIFO input_stream;

#endif // TRX_FRONTEND_DSP_BUFFERS_H
