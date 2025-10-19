//
// Created by Angel Dust on 18/05/2025.
//

#include "aprs_rx_task.h"
#include "dsp/aprs/aprs_packet.h"
#include "dsp/blocks/beep_generator.h"
#include "dsp/dsp_common.h"
#include "dsp/receive/receive_task_base.h"
#include "main_board.h"
#include "stdio.h"
#include "stm32f4xx_hal.h"
#include "s_strength.h"

namespace dsp {

Signal aprs_signal;

MODULATION_MODE APRSTask::get_modulation_mode() const {
    return FM;
}

void APRSTask::process_audio(buffer_t<float32_t> &audio) {

    // Audio signal processing
    // NOTE: Expects REAL-valued buffer

    bool is_noise = false;
    if (squelch_enabled && squelch.is_noise(audio)) {
        // Flag is noise, but don't clear the audio since we don't want to miss a single sample when it is not
        is_noise = true;
    }

    if (deemph_enabled) {
        deemph_filter.decimate(audio, audio, 0, 1, 1);
    }

    if (audio_bpf_enabled) {
        audio_bpf.decimate(audio, audio, 0, 1, 1);
    }

    float32_t *audio_sample_p = audio.p;

    for (size_t c = 0; c < audio.count; c++, audio_sample_p++) {

        const int32_t sample_int = *audio_sample_p;
        int32_t current_sample = __SSAT(sample_int, 16);

        //   current_sample /= 128;
        // Delay line put
        delay_line[delay_line_index & delay_line_ix_mask] = current_sample;

        // Delay line get, and LPF
        sample_mixed = (delay_line[(delay_line_index - (size_t)(samples_per_bit / 2.0f)) & delay_line_ix_mask] * current_sample) / 4;
        sample_filtered = prev_mixed + sample_mixed + (prev_filtered / 2);

        delay_line_index++;

        prev_filtered = sample_filtered;
        prev_mixed = sample_mixed;

        // Update rms estimate
        rms_est = rms_est - alpha * (rms_est - abs(sample_filtered));

        // Threshold and hysteresis
        thr_high = noise_threshold * rms_est;
        thr_low = -noise_threshold * rms_est;

        // !!! DEBUG
        // audio.p[c] = sample_filtered;
        // !!! DEBUG

        // Slice
        sample_bits <<= 1;

        if (sample_filtered < thr_low) {
            last_sample_sign = 1;
        } else if (sample_filtered > thr_high) {
            last_sample_sign = 0;
        }
        uint8_t bit = last_sample_sign;

        sample_bits |= bit;

        // Check for "clean" transition: either 0011 or 1100
        if ((((sample_bits >> 2) ^ sample_bits) & 3) == 3) {
            // Adjust phase
            if (phase < 0.5f) {
                phase += 1.0f / 32; // Is this a proper value ?
            } else {
                phase -= 1.0f / 32;
            }
        }

        phase += phase_inc;

        if (phase >= 1.0f) {

            // DEBUG
            // static uint32_t i = 0;
            // if (i++ % 3000 == 0) {
            //     std::string str = std::string("EADB0") + "ABCDEFGHIJ"[HAL_GetTick() % 7];
            //     if (HAL_GetTick() % 200 > 100) {
            //         aprs_packet.init_test_packet(str, "APRS", "INFO text containing several lines that has to be wrapped up");
            //     } else {
            //         aprs_packet.init_test_packet(str, "APRS", "SHORT info text");
            //     }

            //     aprs_signal.emit(&aprs_packet);
            // }

            // for (int j = 31; j >= 0; j--) {
            //      printf_("%d", (sample_bits >> j) & 1);	    // }

            // printf_("\n");
            //  DEBUG

            phase -= 1.0f;

            if (true) {
                uint8_t bit;
                if (__builtin_popcount(sample_bits & 0xFF) >= 0x05) {
                    bit = 0x1;
                } else {
                    bit = 0x0;
                }

                if (parse_bit(bit)) {
                    parse_packet();
                }
            }
        }

        if (beeper.is_beep_active()) { // If the beeper is active (from previous detection), emit its sample
            adc_type beep_sample = 0;
            beeper.get_sample(beep_sample);
            audio.p[c] = beep_sample;
        }
    }

    // if (HAL_GetTick() % 1000 == 1) {
    //     LOG("rms:%.1f\n", rms_est);
    // }

    if (!beeper.is_beep_active() && is_noise) {
        // Ouput silence
        memset(audio.p, 0, audio.size_bytes);
    }
}

void APRSTask::set_beeper() {

    beeper.set_sample_rate(status.sample_rate);
    beeper.init(BEEP_SUCCESS);
}

void APRSTask::set_squelch() {

    if (config.squelch_level) {
        float threshold = max2(0, 10 - config.squelch_level);
        squelch.config(threshold, status.sample_rate, 2.2f * get_audio_bw_hz());
        squelch_enabled = true;

    } else {
        squelch_enabled = false;
    }
}

void APRSTask::parse_packet() {
    // validate crc
    if (packet_buffer_size >= APRS_MIN_LENGTH) {
        uint16_t crc = 0xFFFF;

        for (size_t i = 0; i < packet_buffer_size; i++) {
            uint8_t byte = packet_buffer[i];
            crc = ((crc >> 8) ^ dsp::crc_ccitt_tab[(crc ^ byte) & 0xFF]) & 0xFFFF;
        }

        if (crc == 0xF0B8) {
            parse_ax25();
        }
    }
}

void APRSTask::parse_ax25() {
    aprs_packet.clear();
    aprs_packet.set_valid_checksum(true);

    for (size_t i = 0; i < packet_buffer_size; i++) {
        aprs_packet.set(i, packet_buffer[i]);
    }

    aprs_signal.emit(&aprs_packet);
    beeper.restart();
}

bool APRSTask::parse_bit(const uint8_t current_bit) {
    uint8_t decoded_bit = ~(current_bit ^ last_bit) & 0x1;
    last_bit = current_bit;

    if (decoded_bit & 0x1) {
        if (ones_count < 8) {
            ones_count++;
        }
    } else {
        if (ones_count > 6) { // not valid
            state = WAIT_FLAG;
            current_byte = 0;
            ones_count = 0;
            byte_index = 0;
            packet_buffer_size = 0;
            return false;
        } else if (ones_count == 6) { // flag
            bool done = false;
            if (state == IN_FRAME) {
                done = true;
            } else {
                packet_buffer_size = 0;
            }
            state = WAIT_FRAME;
            current_byte = 0;
            ones_count = 0;
            byte_index = 0;

            return done;
        } else if (ones_count == 5) { // bit stuff
            ones_count = 0;
            return false;
        } else {
            ones_count = 0;
        }
    }

    // store
    current_byte = current_byte >> 1;
    current_byte |= (decoded_bit == 0x1 ? 0x80 : 0x0);
    byte_index++;

    if (byte_index >= 8) {
        byte_index = 0;
        if (state == WAIT_FRAME) {
            state = IN_FRAME;
        }

        if (state == IN_FRAME) {
            if (packet_buffer_size + 1 >= buffer_size) {

                state = WAIT_FLAG;
                current_byte = 0;
                ones_count = 0;
                byte_index = 0;
                packet_buffer_size = 0;
                return false;
            }
            packet_buffer[packet_buffer_size++] = current_byte;
        }
    }

    // DEBUG

    // if (packet_buffer_size == 20) {
    //     for (size_t i = 0; i < packet_buffer_size; i++) {
    //         for (int j = 7; j >= 0; j--) {
    //             printf_("%d", (packet_buffer[i] >> j) & 1);
    //         }

    //         printf_("|");
    //     }
    //     printf_("\n");
    // }
    // Debug

    return false;
}

bool APRSTask::init() {

    // Not required with the current float accumulator
    // if (status.sample_rate % baudrate != 0) {
    //     LOG("ERROR: Initializing APRS: Sample rate %d is not divisible by baud rate %d\n", status.sample_rate, baudrate);
    //     return false;
    // }

    samples_per_bit = (float32_t)this->status.sample_rate / baudrate;

    phase_inc = (float32_t)baudrate / this->status.sample_rate;
    phase = 0.0f;

    LOG("Initializing APRS | samples per bit: %.2f | phase delta: %.2f\n", samples_per_bit, phase_inc);

    // Delay line
    delay_line_index = 0;

    state = WAIT_FLAG;

    bool ok = deemph_filter.config(status.sample_rate, 300, 1, LPF);

    if (dsp::apply_audio_bpf()) {
        ok = ok && audio_bpf.config(status.sample_rate, get_audio_bw_hz(), 1, 800);
        audio_bpf_enabled = true;
    } else {
        audio_bpf_enabled = false;
    }

    if (!squelch_signal_token) {
        squelch_signal_token = sstrength::squelch_signal.add(NULL, [this](void *, const void *) {
            set_squelch();
        });
    }

    if (beeper_enabled) {
        set_beeper();
        beeper.stop(); // stopped at first
    }

    set_squelch();

    return ok;
}

APRSTask::~APRSTask() {
    if (squelch_signal_token) {
        sstrength::squelch_signal.remove(squelch_signal_token);
    }
}

} // namespace dsp
