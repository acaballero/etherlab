//
// Created by Angel Dust on 04/06/2025.
//

#include "afsk_tx_task.h"
#include "arm_math.h"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft.h"
#include "dsp/receive/receive_task_base.h"
#include "hw/stm32f4xx/adc.h"
#include "main_board.h"
#include "printf.h"
#include "stdio.h"
#include "stm32f4xx_hal.h"
#include "dsp/blocks/signal_generator.h"
#include "types.h"
#include <cstring>
#include <stdint.h>

namespace dsp {

void AFSKTXTask::work() {

    if (status.status != DSP_STATUS_RUNNING || packet.size() == 0) {
        return;
    }

    char *out_p;

    uint32_t free = output_stream.free(&out_p);

    // Let's process multiple blocks per iteration
    int nb = 2;

    if (free >= status.block_size_bytes * nb) {

        uint16_t cur_word;

        for (int b = 0; b < nb; b++) {
            this->status.processed_blocks++;
            for (int i = DSP_BLOCK * b; i < DSP_BLOCK * (b + 1); i++) {

                if (fill_bits) {
                    fill_bits--;
                    cur_bit = 0;
                } else {
                    if (sample_count >= afsk_samples_per_bit) { // End of sample

                        cur_word = packet[packet_ix];

                        if (!cur_word) {
                            // End of data packet

                            if (repeat_counter < afsk_repeat) {

                                // Repeat
                                bit_pos = 0;
                                packet_ix = 0;
                                cur_word = packet[0];
                                repeat_counter++;

                            } else {
                                // Stop: Note that this will stop the replay processor, which likely will be finished with the buffer
                                // when it happens, but it is not guaranteed. Should find a way to make the processor be responsible of finishing

                                // LOG("Finished transmitting AFSK packet\n");

                                // After stopping, the loop will feed empty DSP_BLOCK so the DMA won't repeat but zeores until we have time to stop it all
                                if (delay_tail_ms && !tail_sent) {

                                    // Set tail bits
                                    fill_bits =
                                        (float)afsk_samples_per_bit * (float)delay_tail_ms * (((float)afsk_phase_inc_mark / (float)afsk_delta_coeff) / 1000.0f);
                                    tail_sent = true;

                                } else {
                                    stop();
                                }

                                break;
                            }
                        }

                        cur_bit = (cur_word >> (symbol_count - bit_pos)) & 1;

                        if (bit_pos >= symbol_count) {
                            bit_pos = 0;
                            packet_ix++;
                        } else {
                            bit_pos++;
                        }

                        sample_count = 0;
                    } else {
                        sample_count++;
                    }
                }

                if (cur_bit) {
                    tone_phase += afsk_phase_inc_mark;
                } else {
                    tone_phase += afsk_phase_inc_space;
                }

                uint32_t t_ix = (tone_phase & 0xFF000000U) >> 24;
                tone_sample = sine_table_i8[t_ix];

                delta = tone_sample * fm_delta;

                phase += delta;
                sphase = phase + (64 << 24);

                uint32_t r_ix = (sphase & 0xFF000000U) >> 24;
                uint32_t i_ix = (phase & 0xFF000000U) >> 24;

                re = (sine_table_i8[r_ix]);
                im = (sine_table_i8[i_ix]);

                // LOG("%d,", tone_sample);

                //((complex_t *)out_p)[i] = {{(adc_type)tone_sample, (adc_type)tone_sample}};
                ((complex_t *)out_p)[i] = {{re, im}};
            }

            // LOG("\n");

            output_stream.feed(status.block_size_bytes);

            if (status.processed_blocks == 1 && on_first_block) {
                on_first_block();
            }
        }
    }
}

void AFSKTXTask::configure(uint32_t phase_inc_mark, uint32_t phase_inc_space, uint8_t repeat, uint8_t symbols_per_byte, uint32_t bandwidth, uint16_t delay_ms,
                           uint16_t tail_ms) {

    afsk_delta_coeff = ((1ULL << 32) / config.fft.sample_rate);

    uint32_t samples_per_bit = config.fft.sample_rate / phase_inc_mark; // Assumes mark=1200 and divisor of sample_rate
    afsk_samples_per_bit = samples_per_bit;
    afsk_phase_inc_mark = phase_inc_mark * afsk_delta_coeff;
    afsk_phase_inc_space = phase_inc_space * afsk_delta_coeff;
    afsk_repeat = repeat - 1;
    fm_delta = bandwidth * (0xFFFFFFULL / config.fft.sample_rate); // FM deviation delta
    symbol_count = symbols_per_byte - 1;
    sample_count = afsk_samples_per_bit;
    delay_front_ms = min2(delay_ms, AFSK_MAX_DELAY_MS);
    fill_bits = (float)afsk_samples_per_bit * (float)delay_front_ms * ((float)phase_inc_mark / 1000.0f); // Start fill bits
    delay_tail_ms = min2(tail_ms, AFSK_MAX_TAIL_MS);
    tail_sent = false;

    status.reset();
    output_stream.reset();
}

void AFSKTXTask::set_data(uint16_t *data) {

    repeat_counter = 0;
    bit_pos = 0;
    uint32_t i = 0;

    packet.clear();
    for (i = 0; i < AFSK_MAX_PACKET_SIZE - 1 && data[i]; i++) {
        if (i >= packet.size()) {
            packet.resize(i + 20);
        }
        packet[i] = data[i];
    }

    packet.push_back(0);
    packet_ix = 0;
    cur_bit = 0;
}

bool AFSKTXTask::start() {

    LOG("------ [BEGIN] AFSKTX task START------\n");
    status.reset();
    status.direction = DSP_DIRECTION_OUT;
    status.sample_rate = config.fft.sample_rate;
    status.decimation_factor = 1;

    status.block_size_bytes = DSP_BLOCK * 2 * 2;
    status.n_channels = 2;
    status.bandwidth = fft_params.span;
    status.bits_per_sample = 16;
    status.decimated_block_size = DSP_BLOCK * 2 / status.decimation_factor / (this->status.n_channels == 1 ? 2 : 1);
    status.decimated_block_size_bytes = this->status.block_size_bytes / status.decimation_factor / (this->status.n_channels == 1 ? 2 : 1);

    bool ret = radio_config({.direction = RF_DIRECTION_TX,
                             .sample_freq = status.sample_rate,
                             .freq = 0,
                             .mode = DSP}); // Radio mode is DSP so the signal is routed to the audio amp

    if (!ret) {
        halt(DSP_ERR);
        return false;
    }

    main_board::setMute(GPIO_PIN_RESET);

    // Se the fifo processing frequency
    LOG("-- [END] AFSKTX ST --\n");
    // Start task processing timer
    // TODO: This should be done by the caller of this method and be generic for all tasks
    HAL_TIM_Base_Start_IT(&TASKS_TIMER_HANDLE);
    status.status = DSP_STATUS_RUNNING;
    update_timer(TASKS_TIMER_TYPEDEF, 10, TASKS_TIMER_TYPEDEF_CLOCK_HZ / 100000);

    return true;
}

void AFSKTXTask::stop() {

    // LOG("------ [BEGIN] AFSKTX task STOP------\n");
    if (this->status.status != DSP_STATUS_STOPPED) {

        output_stream.close();

        this->status.status = DSP_STATUS_STOPPED;

        // Stop media read processing timer
        HAL_TIM_Base_Stop_IT(&TASKS_TIMER_HANDLE);

        // Wait until data processing stops
        while (output_stream.available()) {
            HAL_Delay(4);
        }

        Task::stop(); // Let the base class finish housekeeping stuff

        // Put the radio back in RX
        radio_config({.direction = RF_DIRECTION_RX, .sample_freq = status.sample_rate, .freq = 0, .mode = DSP});
    }
    // LOG("------ [END] AFSKTX task STOP------\n");
}

} // namespace dsp
