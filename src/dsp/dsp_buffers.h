//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_BUFFERS_H
#define TRX_FRONTEND_DSP_BUFFERS_H

#include "dsp_common.h"
#include "buffer.hpp"

// DSP processing block size. Defines the number of samples adquired in each DMA cycle
#define DSP_BLOCK 128

#define DSP_DAC_BUFF_SIZE (DSP_BLOCK << 2)
#define DSP_ADC_BUFF_SIZE (DSP_BLOCK << 2)

// Must be a multiple of any chunk that a stream processor taks will try to read
// For example, if the capture task needs to write 1024 bytes per block and the receive task 64 bytes, that's ok.
// If one task needs,for example, 512 bytes and another 1500, the FIFO block cannot be either 1500 or 3000
// TODO: This is a consequence of using a FIFO of contiguous memory blocks. A better approach is using a FIFO of memory "buckets", so the fifo contains
// pointers to memory blocks of arbirary size. However, note that the current implementation ensures all fifo operations are O(1) and very fast.
// 512 is the default SD sector size. Also, this must be more than MAX_DECIMATION_FACTOR*DSP_BLOCK*(4 bytes per sample) so
#define DSP_FIFO_BLOCK_BYTES (512) * 8 * 2
// FIFO size must be multiple of DSP_FIFO_BLOCK_BYTES. The number of BLOCK_BYTES blocks in it
#define DSP_FIFO_SIZE DSP_FIFO_BLOCK_BYTES * 3
#define DSP_OUTPUT_FIFO_SIZE DSP_FIFO_BLOCK_BYTES * 2
// ACD DMA buffer
extern adc_type adc_buff[DSP_ADC_BUFF_SIZE];

// DAC DMA buffer
extern adc_type dac_buff[DSP_DAC_BUFF_SIZE];

//__attribute__((section(".fccmram"))) // Can't be in CCM RAM if DMA is used
extern uint8_t dsp_output_fifo_buff[DSP_OUTPUT_FIFO_SIZE];
extern uint8_t dsp_input_fifo_buff[DSP_FIFO_SIZE];

// Half-DMA buffer wrappers
extern buffer_t<adc_type> adc_buffer_1;
extern buffer_t<adc_type> adc_buffer_2;
extern buffer_t<adc_type> dac_buffer_1;
extern buffer_t<adc_type> dac_buffer_2;
// extern buffer_t<adc_type> tmp_buffer;
extern FIFO output_stream;
extern FIFO input_stream;

void reset_dac_buffer();

#endif // TRX_FRONTEND_DSP_BUFFERS_H
