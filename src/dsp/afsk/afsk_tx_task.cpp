//
// Created by Angel Dust on 04/06/2025.
//

#include "afsk_tx_task.h"
#include "arm_math.h"
#include "dsp/aprs/aprs_packet.h"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/receive/receive_task_base.h"
#include "hw/stm32f4xx/adc.h"
#include "main_board.h"
#include "printf.h"
#include "stdio.h"
#include "stm32f4xx_hal.h"
#include "dsp/blocks/signal_generator.h"
#include <cstring>
#include <stdint.h>

namespace dsp {

void AFSKTXTask::work() {

    if (status.status != DSP_STATUS_RUNNING) {
        return;
    }

    char *out_p;

    uint32_t free = output_stream.free(&out_p);

    if (free >= DSP_BLOCK) {

        this->status.processed_blocks++;

        for (size_t i = 0; i < free; i++) {
            if (sample_count >= afsk_samples_per_bit) { // End of sample

                cur_word = *word_ptr;

                if (!cur_word) {
                    // End of data
                    if (repeat_counter < afsk_repeat) {
                        // Repeat
                        bit_pos = 0;
                        word_ptr = packet_ptr;
                        cur_word = *word_ptr;
                        repeat_counter++;
                    } else {
                        // Stop
                        cur_word = 0;
                        stop();
                    }
                }

                cur_bit = (cur_word >> (symbol_count - bit_pos)) & 1;

                if (bit_pos >= symbol_count) {
                    bit_pos = 0;
                    word_ptr++;
                } else {
                    bit_pos++;
                }

                sample_count = 0;
            } else {
                sample_count++;
            }

            if (cur_bit) {
                tone_phase += afsk_phase_inc_mark;
            } else {
                tone_phase += afsk_phase_inc_space;
            }

            tone_sample = sine_table_i8[(tone_phase & 0xFF000000U) >> 24];

            delta = tone_sample * fm_delta;

            phase += delta;
            sphase = phase + (64 << 24);

            re = (sine_table_i8[(sphase & 0xFF000000U) >> 24]);
            im = (sine_table_i8[(phase & 0xFF000000U) >> 24]);

            ((complex_t *)out_p)[i] = {{re, im}};
        }

        output_stream.feed(free);
    }
}

void AFSKTXTask::configure(uint32_t phase_inc_mark, uint32_t phase_inc_space, uint8_t repeat, uint8_t symbol_count, uint32_t bandwidth, uint16_t *data_ptr) {

    uint32_t samples_per_bit = config.fft.sample_rate / phase_inc_mark; // Assumes mark=1200 and divisor of sample_rate
    afsk_samples_per_bit = samples_per_bit;
    afsk_phase_inc_mark = phase_inc_mark * AFSK_DELTA_COEF;
    afsk_phase_inc_space = phase_inc_space * AFSK_DELTA_COEF;
    afsk_repeat = repeat - 1;
    fm_delta = bandwidth * (0xFFFFFFULL / AFSK_SAMPLERATE); // get max deviation?
    symbol_count = symbol_count - 1;
    sample_count = afsk_samples_per_bit;
    repeat_counter = 0;
    bit_pos = 0;
    packet = {data_ptr, AFSK_MAX_PACKET_SIZE};
    word_ptr = packet.c_str();
    cur_word = 0;
    cur_bit = 0;
}

bool AFSKTXTask::start() {

    bool ret = radio_config({.direction = RF_DIRECTION_TX,
                             .sample_freq = status.sample_rate,
                             .freq = 0,
                             .mode = DSP}); // Radio mode is DSP so the signal is routed to the audio amp

    if (!ret) {
        halt(DSP_ERR);
        return false;
    }

    main_board::setMute(GPIO_PIN_RESET);

    // Start task processing timer
    // TODO: This should be done by the caller of this method and be generic for all tasks
    HAL_TIM_Base_Start_IT(&TASKS_TIMER_HANDLE);

    // Se the fifo processing frequency
    update_timer(TASKS_TIMER_TYPEDEF, 80, TASKS_TIMER_TYPEDEF_CLOCK_HZ / 100000);
    status.status = DSP_STATUS_RUNNING;

    return true;
}

} // namespace dsp
