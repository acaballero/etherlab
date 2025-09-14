//
// Created by Angel Dust on 18/05/2025.
//

#include "aprs_packet.h"
#include "stm32f4xx.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <math.h>
#include <ostream>
#include <stdint.h>
#include <stdio.h>
#include <array>
#include <functional>

template <typename T> struct buffer_t {

    T *const p;
    size_t count;
    uint32_t sample_rate;

    uint32_t decimation_factor = 1;
    uint32_t decimated_size_bytes = 0;
    size_t size_bytes = 0;

    constexpr buffer_t() : p{nullptr}, count{0}, sample_rate{0}, decimation_factor{1} {
        size_bytes = count * sizeof(p[0]);
        decimated_size_bytes = size_bytes / decimation_factor;
    }

    constexpr buffer_t(const buffer_t<T> &other)
        : p{other.p}, count{other.count}, sample_rate{other.sample_rate}, decimation_factor{other.decimation_factor},
          decimated_size_bytes{other.decimated_size_bytes}, size_bytes{other.size_bytes} {
    }

    constexpr buffer_t(T *const p, const size_t count, const uint32_t sampling_rate = 0, const uint32_t decimation_factor = 1)
        : p{p}, count{count}, sample_rate{sampling_rate}, decimation_factor{decimation_factor} {
        size_bytes = count * sizeof(p[0]);
        decimated_size_bytes = size_bytes / decimation_factor;
    }

    operator bool() const {
        return (p != nullptr);
    }
};

static uint16_t crc_ccitt_tab[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 0x1081, 0x0108, 0x3393,
    0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876, 0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af,
    0x4434, 0x55bd, 0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5, 0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 0xbdcb,
    0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb, 0xce4c, 0xdfc5, 0xed5e, 0xfcd7,
    0x8868, 0x99e1, 0xab7a, 0xbaf3, 0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a, 0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb,
    0xaa72, 0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1, 0x7387, 0x620e,
    0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70, 0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c,
    0xd3a5, 0xe13e, 0xf0b7, 0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff, 0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 0x2942, 0x38cb, 0x0a50,
    0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e,
    0x5cf5, 0x4d7c, 0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 0xd68d,
    0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a, 0xe70e, 0xf687, 0xc41c, 0xd595,
    0xa12a, 0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9, 0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9,
    0x8330, 0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78};

class APRSTask {
  public:
    void set_bit_threshold(int8_t v) {
        bit_threshold = v;
    }
    int8_t get_bit_threshold() {
        return bit_threshold;
    }

    static constexpr size_t baudrate = 1200;
    static constexpr size_t buffer_size = 256;
    static constexpr size_t delay_line_length = 64; // Note: Must be a power of 2 so the index can be ANDded
    static constexpr size_t delay_line_ix_mask = delay_line_length - 1;
    float_t noise_threshold = 0.02f;

    float_t alpha = 0.005f; // slow RMS smoothing

    float_t samples_per_bit{};

    enum State { WAIT_FLAG, WAIT_FRAME, IN_FRAME };

    // Array size is long enough down to B bauds (B = audio_bandwidth / delay_line_length)
    std::array<float_t, delay_line_length> delay_line{0};

    State state{};
    size_t delay_line_index{};

    uint32_t sample_bits{0};
    float_t phase{}, phase_inc{};
    float_t rms_est = 0.0f;
    float_t thr_high;
    float_t thr_low;
    int sample_rate;
    int32_t last_sample_sign = 0;
    float_t sample_mixed{}, prev_mixed{}, sample_filtered{}, prev_filtered{};
    uint8_t last_bit = 0;
    uint8_t ones_count = 0;
    uint8_t current_byte = 0;
    uint8_t byte_index = 0;
    uint8_t packet_buffer[buffer_size];
    int32_t bit_threshold = -1;
    size_t packet_buffer_size = 0;
    float_t phase_adjust = 1.0f / 32;
    float_t scale = 20;

    APRSPacket aprs_packet{};

    bool parse_packet();
    bool parse_bit(const uint8_t bit);
    void parse_ax25();

    bool init();
    int process_audio(buffer_t<float_t> &buff_out_f32);

    std::function<void(APRSPacket *)> on_packet;
};

int APRSTask::process_audio(buffer_t<float> &audio) {

    float *audio_sample_p = audio.p;

    int packet_end_index = -1;

    for (size_t c = 0; c < audio.count; c++, audio_sample_p++) {

        float_t sample_float = *audio_sample_p * scale;
        const int32_t sample_int = sample_float;

        float_t current_sample = sample_float; // __SSAT(sample_int, 16);

        //   current_sample /= 128;
        // Delay line put
        delay_line[delay_line_index & delay_line_ix_mask] = current_sample;

        // Delay line get, and LPF
        sample_mixed = (delay_line[(delay_line_index - (size_t)(samples_per_bit / 2.0f)) & delay_line_ix_mask] * current_sample) / 4;
        sample_filtered = prev_mixed + sample_mixed + (prev_filtered / 2);

        delay_line_index++;

        prev_filtered = sample_filtered;
        prev_mixed = sample_mixed;

        // Slice
        sample_bits <<= 1;

        uint8_t bit;

        if (alpha != 0) {
            // Update rms estimate
            rms_est = rms_est - alpha * (rms_est - abs(sample_filtered));

            // Threshold and hysteresis
            thr_high = noise_threshold * rms_est;
            thr_low = -noise_threshold * rms_est;

            // !!! DEBUG
            // audio.p[c] = sample_filtered;
            // !!! DEBUG

            if (sample_filtered < thr_low) {
                last_sample_sign = 1;
            } else if (sample_filtered > thr_high) {
                last_sample_sign = 0;
            }
            bit = last_sample_sign;
        } else {
            if (sample_filtered < bit_threshold) {
                bit = 1;
            } else {
                bit = 0;
            }
        }

        sample_bits |= bit;

        // Check for "clean" transition: either 0011 or 1100
        if ((((sample_bits >> 2) ^ sample_bits) & 3) == 3) {
            // Adjust phase
            if (phase < 0.5f) {
                phase += phase_adjust;
            } else {
                phase -= phase_adjust;
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
                    if (parse_packet()) {
                        packet_end_index = c;
                    }
                }
            }
        }
    }

    return packet_end_index;
}

bool APRSTask::parse_packet() {
    // validate crc
    bool b = false;
    if (packet_buffer_size >= APRS_MIN_LENGTH) {
        uint16_t crc = 0xFFFF;

        for (size_t i = 0; i < packet_buffer_size; i++) {
            uint8_t byte = packet_buffer[i];
            crc = ((crc >> 8) ^ crc_ccitt_tab[(crc ^ byte) & 0xFF]) & 0xFFFF;
        }

        if (crc == 0xF0B8) {
            parse_ax25();
            b = true;
        }
    }
    return b;
}

void APRSTask::parse_ax25() {
    aprs_packet.clear();
    aprs_packet.set_valid_checksum(true);

    for (size_t i = 0; i < packet_buffer_size; i++) {
        aprs_packet.set(i, packet_buffer[i]);
    }

    on_packet(&aprs_packet);
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

    samples_per_bit = (float_t)sample_rate / baudrate;

    phase_inc = (float_t)baudrate / sample_rate;
    phase = 0.0f;

    // Delay line
    delay_line_index = 0;

    state = WAIT_FLAG;

    std::cout << "Decoder: sample rate: " << sample_rate << " | phase_inc: " << phase_inc << " | samples_per_bit: " << samples_per_bit << std::endl;
    return true;
}
