
#include "fsk_rx_task.h"
#include "arm_math.h"
#include "dsp/buffer.hpp"
#include "dsp/receive/receive_task_base.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>

namespace {
/* Count of bits that differ between the two values. */
uint8_t diff_bit_count(uint32_t left, uint32_t right) {
    uint32_t diff = left ^ right;
    uint8_t count = 0;
    for (size_t i = 0; i < sizeof(diff) * 8; ++i) {
        if (((diff >> i) & 0x1) == 1) {
            ++count;
        }
    }

    return count;
}
} // namespace

void FSKRXTask::clear_data_bits() {
    data = 0;
    bit_count = 0;
}

void FSKRXTask::handle_sync(bool b) {
    clear_data_bits();
    has_sync_ = true;
    inverted = b;
    word_count = 0;
}

void FSKRXTask::process_bits(const buffer_t<uint8_t> &buffer) {
    // Process all of the bits in the bits queue.
    while (buffer.count > 0) {
        // Wait until data_ is full.
        if (bit_count < data_bit_count) {
            continue;
        }

        // Wait for the sync frame.
        if (!has_sync_) {
            if (diff_bit_count(data, sync_codeword) <= 2) {
                handle_sync(/*inverted=*/false);
            } else if (diff_bit_count(data, ~sync_codeword) <= 2) {
                handle_sync(/*inverted=*/true);
            }
            continue;
        }
    }
}

/* FSKRXTask ***************************************/

void FSKRXTask::process_audio(buffer_t<float32_t> &buffer) {

    // process_bits();
}

void FSKRXTask::configure(size_t deviation, uint32_t sample_rate) {
    // Extract message variables.
    this->deviation = deviation;
    // channel_decimation = message.channel_decimation;
    //  channel_filter_taps = message.channel_filter;

    // TODO: Do we need to use the taps that the decimators get configured with?
    // channel_filter_low_f = taps_200k_decim_1.low_frequency_normalized * sample_rate;
    // channel_filter_high_f = taps_200k_decim_1.high_frequency_normalized * sample_rate;
    // channel_filter_transition = taps_200k_decim_1.transition_normalized * sample_rate;
}

void FSKRXTask::flush() {
    // word_extractor.flush();
}

bool FSKRXTask::init() {

    clear_data_bits();
    has_sync_ = false;
    inverted = false;
    word_count = 0;

    samples_processed = 0;
    return true;
}

void FSKRXTask::send_packet(uint32_t data) {
    // Not implemented
}
