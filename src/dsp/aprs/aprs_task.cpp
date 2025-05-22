//
// Created by Angel Dust on 18/05/2025.
//

#include "aprs_task.hpp"
#include "dsp/aprs/aprs_packet.h"
#include "hw/stm32f4xx/adc.h"
#include "stdio.h"
#include "stm32f4xx_hal.h"
#include <sys/_stdint.h>

namespace dsp {

Signal aprs_signal;

MODULATION_MODE APRSTask::get_modulation_mode() {
    return FM;
}

void APRSTask::process_audio(buffer_t<float32_t> &audio) {

    // Audio signal processing
    for (size_t c = 0; c < audio.count; c++) {
        const int32_t sample_int = audio.p[c] * 32768.0f;
        int32_t current_sample = __SSAT(sample_int, 16);

        current_sample /= 128;

        // Delay line put
        delay_line[delay_line_index & 0x3F] = current_sample;

        // Delay line get, and LPF
        sample_mixed = (delay_line[(delay_line_index - (samples_per_bit / 2)) & 0x3F] * current_sample) / 4;
        sample_filtered = prev_mixed + sample_mixed + (prev_filtered / 2);

        delay_line_index++;

        prev_filtered = sample_filtered;
        prev_mixed = sample_mixed;

        // Slice
        sample_bits <<= 1;

        uint8_t bit = (sample_filtered < -20) ? 1 : 0;
        sample_bits |= bit;

        // Check for "clean" transition: either 0011 or 1100
        if ((((sample_bits >> 2) ^ sample_bits) & 3) == 3) {
            // Adjust phase
            if (phase < 0x8000) {
                phase += 0x800; // Is this a proper value ?
            } else {
                phase -= 0x800;
            }
        }

        phase += phase_inc;

        if (phase >= 0x1000) {

            // DEBUG
            static uint32_t i = 0;
            if (i++ % 30000 == 0) {
                std::string str = std::string("EADB0") + "ABCDEFGHIJ"[HAL_GetTick() % 7];
                if (HAL_GetTick() % 200 < 100) {
                    aprs_packet.init_test_packet(str, "APRS", "INFO text containing several lines that has to be wrapped up");
                } else {
                    aprs_packet.init_test_packet(str, "APRS", "SHORT info text");
                }
                aprs_signal.emit(&aprs_packet);
            }
            // DEBUG

            phase &= 0xFFFF;

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
}

bool APRSTask::parse_bit(const uint8_t current_bit) {
    uint8_t decoded_bit = ~(current_bit ^ last_bit) & 0x1;
    last_bit = current_bit;

    // int16_t log = decoded_bit == 0 ? -32768 : 32767;
    // if(stream){
    //     const size_t bytes_to_write = sizeof(log) * 1;
    //     const auto result = stream->write(&log, bytes_to_write);
    // }

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
            if (packet_buffer_size + 1 >= 256) {
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

    return false;
}

bool APRSTask::init() {

    samples_per_bit = this->status.sample_rate / baudrate;

    phase_inc = (0x10000 * baudrate) / this->status.sample_rate;
    phase = 0;

    // Delay line
    delay_line_index = 0;

    state = WAIT_FLAG;

    return true;
}

} // namespace dsp
