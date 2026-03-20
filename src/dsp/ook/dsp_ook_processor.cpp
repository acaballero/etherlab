//
// OOK (On-Off Keying) Transmit Processor
//

#include "dsp_ook_processor.h"
#include "dsp/dsp_common.h"
#include "types.h"
#include <cstring>

void DspOOKProcessor::set_config(uint32_t carrier_freq, uint32_t mark_us, uint32_t space_us,
                                  uint32_t pause_us, uint32_t sample_rate) {
    this->mark_duration_us = mark_us;
    this->space_duration_us = space_us;
    this->pause_us = pause_us;
    this->cached_sample_rate = sample_rate;

    // Convert microseconds to sample counts
    const float32_t us_to_samples = (float32_t)sample_rate / 1000000.0f;

    // Mark/space must be >= 1 sample for the interval engine to make progress.
    mark_samples = (float32_t)mark_us * us_to_samples;
    if (mark_samples < 1.0f) {
        mark_samples = 1.0f;
    }

    space_samples = (float32_t)space_us * us_to_samples;
    if (space_samples < 1.0f) {
        space_samples = 1.0f;
    }

    // Pause may be zero (no gap).
    pause_samples = (float32_t)pause_us * us_to_samples;

    carrier.set_config(carrier_freq, sample_rate);
    carrier.set_shape(dsp::SIGNAL_SHAPE_SIN);

    // Reset playback state
    sample_accumulator = 0;
    bit_index = 0;
    current_rep = 0;
    finished = false;
    in_pause = false;
    current_interval_len = 0;
    brute_advance_pending = false;
}

void DspOOKProcessor::set_sequence(const std::vector<uint8_t> &seq) {
    mode = OOKTxMode::Manual;
    brute_advance_pending = false;

    sequence = seq;
    sample_accumulator = 0;
    bit_index = 0;
    current_rep = 0;
    finished = false;
    in_pause = false;
    current_interval_len = 0;
}

void DspOOKProcessor::set_bruteforce(OOKBruteProtocol protocol, uint32_t start_code, uint32_t stop_code, uint32_t step) {
    mode = OOKTxMode::BruteForce;

    brute_protocol = protocol;
    brute_start_code = start_code;
    brute_stop_code = stop_code;
    brute_step = (step == 0) ? 1 : step;

    brute_counter = brute_start_code;
    brute_advance_pending = false;

    sample_accumulator = 0;
    bit_index = 0;
    current_rep = 0;
    finished = false;
    in_pause = false;
    current_interval_len = 0;

    if (!load_brute_sequence(brute_counter)) {
        finished = true;
    }
}

bool DspOOKProcessor::load_brute_sequence(uint32_t code) {
    const auto *preset = ook_brute_get_preset(brute_protocol);
    if (!preset) {
        return false;
    }

    // Respect preset bit width.
    code &= ook_brute_max_code(*preset);

    return ook_brute_build_sequence(*preset, code, sequence);
}

bool DspOOKProcessor::advance_brute_code() {
    const auto *preset = ook_brute_get_preset(brute_protocol);
    if (!preset) {
        return false;
    }

    const uint32_t max_code = ook_brute_max_code(*preset);

    // The start/stop might exceed the preset range; clamp counter to the preset size.
    brute_start_code &= max_code;
    brute_stop_code &= max_code;

    uint32_t next = brute_counter + brute_step;

    if (next > brute_stop_code) {
        if (loop) {
            next = brute_start_code;
        } else {
            return false;
        }
    }

    brute_counter = next;
    return load_brute_sequence(brute_counter);
}

uint32_t DspOOKProcessor::get_frame_duration_us() const {
    uint32_t total = 0;
    for (auto bit : sequence) {
        total += (bit != 0) ? mark_duration_us : space_duration_us;
    }
    return total;
}

void DspOOKProcessor::work(const buffer_t<adc_type> *buffer) {

    if (this->info.status != DSP_STATUS_RUNNING || finished) {
        memset((char *)buffer->p, 0, buffer->count << 1);
        return;
    }

    if (sequence.empty()) {
        memset((char *)buffer->p, 0, buffer->count << 1);
        return;
    }

    this->info.processed_blocks++;

    buffer_t<complex_t> wrapped{(complex_t *)buffer->p, buffer->count / 2};
    complex_t sample;

    for (size_t i = 0; i < wrapped.count; i++) {

        // If we haven't set up the current interval yet, do so now
        if (current_interval_len == 0) {

            if (in_pause) {
                // We just entered the pause interval
                current_interval_len = pause_samples;
            } else if (bit_index < sequence.size()) {
                // Set up a mark or space interval based on current bit
                bool is_mark = (sequence[bit_index] != 0);
                current_interval_len = is_mark ? mark_samples : space_samples;
            } else {
                // Sequence exhausted (end of one frame)
                current_rep++;

                if (mode == OOKTxMode::BruteForce) {

                    if (current_rep < repetitions) {
                        // Repeat the same code
                        bit_index = 0;
                        sample_accumulator = 0;

                        if (pause_samples > 0) {
                            in_pause = true;
                            current_interval_len = pause_samples;
                        } else if (!sequence.empty()) {
                            const bool is_mark = (sequence[0] != 0);
                            current_interval_len = is_mark ? mark_samples : space_samples;
                        }

                    } else {
                        // Finished repeating this code. Advance to next code after the pause.
                        current_rep = 0;

                        if (pause_samples > 0) {
                            in_pause = true;
                            brute_advance_pending = true;
                            current_interval_len = pause_samples;
                        } else {
                            // No pause: advance immediately.
                            if (!advance_brute_code() || sequence.empty()) {
                                for (size_t j = i; j < wrapped.count; j++) {
                                    wrapped.p[j].i = 0;
                                    wrapped.p[j].r = 0;
                                }
                                finished = true;
                                return;
                            }

                            bit_index = 0;
                            sample_accumulator = 0;
                            const bool is_mark = (sequence[0] != 0);
                            current_interval_len = is_mark ? mark_samples : space_samples;
                        }
                    }

                } else {

                    // Manual sequence mode: repetitions and/or loop on the same sequence
                    if (loop || current_rep < repetitions) {
                        bit_index = 0;
                        sample_accumulator = 0;

                        if (pause_samples > 0) {
                            in_pause = true;
                            current_interval_len = pause_samples;
                        } else {
                            // No pause, restart immediately
                            bool is_mark = (sequence[0] != 0);
                            current_interval_len = is_mark ? mark_samples : space_samples;
                        }
                    } else {
                        // Done — fill rest with silence
                        for (size_t j = i; j < wrapped.count; j++) {
                            wrapped.p[j].i = 0;
                            wrapped.p[j].r = 0;
                        }
                        finished = true;
                        return;
                    }
                }
            }
        }

        // Generate output sample
        bool emit_carrier = false;
        if (!in_pause && bit_index < sequence.size()) {
            emit_carrier = (sequence[bit_index] != 0);
        }

        // Always advance carrier phase (even during silence) to avoid phase discontinuities
        carrier.get_complex_sample(sample);

        if (emit_carrier) {
            wrapped.p[i].i = sample.i;
            wrapped.p[i].r = sample.r;
        } else {
            wrapped.p[i].i = 0;
            wrapped.p[i].r = 0;
        }

        // Advance within current interval
        sample_accumulator += 1.0f;
        if (sample_accumulator >= current_interval_len) {
            sample_accumulator -= current_interval_len;
            current_interval_len = 0; // Will be set up next iteration

            if (in_pause) {
                // Pause finished
                in_pause = false;

                if (mode == OOKTxMode::BruteForce && brute_advance_pending) {
                    brute_advance_pending = false;
                    if (!advance_brute_code() || sequence.empty()) {
                        for (size_t j = i + 1; j < wrapped.count; j++) {
                            wrapped.p[j].i = 0;
                            wrapped.p[j].r = 0;
                        }
                        finished = true;
                        return;
                    }
                }

                bit_index = 0;
            } else {
                bit_index++;
            }
        }
    }
}
