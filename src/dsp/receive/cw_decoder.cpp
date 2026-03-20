#include "cw_decoder.h"

#include <algorithm>
#include <cmath>

namespace cw_decode {

Signal text_signal{"cw_decode_text"};

static constexpr float PI_F = 3.14159265358979323846f;

static float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(v, hi));
}

void CwDecoderBase::configure(uint32_t sample_rate_hz, uint32_t tone_hz) {
    sample_rate = sample_rate_hz;
    expected_tone_hz = tone_hz;

    const float default_wpm = 16.0f;
    dot_samples = (1.2f / default_wpm) * static_cast<float>(sample_rate);

    reset_common();
}

void CwDecoderBase::reset() {
    reset_common();
}

void CwDecoderBase::reset_common() {
    env = 0.0f;
    noise_floor = 0.002f;
    signal_floor = 0.03f;
    tone_on = false;
    run_samples = 0;
    current_symbol.clear();
    pending_text.clear();
    symbol_conf_accum = 0.0f;
    symbol_conf_count = 0;
}

std::string CwDecoderBase::take_text() {
    std::string out;
    out.swap(pending_text);
    return out;
}

bool CwDecoderBase::envelope_detect(float abs_sample) {
    if (!tone_on) {
        noise_floor += 0.002f * (env - noise_floor);
    } else {
        signal_floor += 0.002f * (env - signal_floor);
    }

    if (signal_floor < (noise_floor + 0.01f)) {
        signal_floor = noise_floor + 0.01f;
    }

    const float on_threshold = noise_floor + (signal_floor - noise_floor) * 0.55f;
    const float off_threshold = noise_floor + (signal_floor - noise_floor) * 0.30f;

    if (!tone_on && abs_sample >= on_threshold) {
        return true;
    }
    if (tone_on && abs_sample <= off_threshold) {
        return false;
    }

    return tone_on;
}

void CwDecoderBase::add_symbol_confidence(float confidence) {
    symbol_conf_accum += clampf(confidence, 0.0f, 1.0f);
    if (symbol_conf_count < 255) {
        symbol_conf_count++;
    }
}

void CwDecoderBase::handle_standard_transition(bool now_on, float run_f, float snr_conf) {
    if (!now_on) {
        const bool is_dot = run_f <= 1.9f * dot_samples;
        const float target = is_dot ? dot_samples : (3.0f * dot_samples);
        const float timing_conf = clampf(1.0f - (std::fabs(run_f - target) / std::max(1.0f, target)), 0.0f, 1.0f);
        const float elem_conf = 0.65f * timing_conf + 0.35f * clampf(snr_conf, 0.0f, 1.0f);
        add_symbol_confidence(elem_conf);

        if (is_dot) {
            current_symbol.push_back('.');
            dot_samples = 0.90f * dot_samples + 0.10f * run_f;
        } else {
            current_symbol.push_back('-');
            dot_samples = 0.95f * dot_samples + 0.05f * (run_f / 3.0f);
        }

        dot_samples = clampf(dot_samples, sample_rate * 0.02f, sample_rate * 0.22f);
        return;
    }

    const float word_gap_mult = static_cast<float>(dsp::dsp_config.cw_word_gap_mult_x10) * 0.1f;
    const float letter_gap_mult = static_cast<float>(dsp::dsp_config.cw_letter_gap_mult_x10) * 0.1f;

    if (run_f >= word_gap_mult * dot_samples) {
        finalize_symbol();
        if (!pending_text.empty() && pending_text.back() != ' ') {
            pending_text.push_back(' ');
        }
    } else if (run_f >= letter_gap_mult * dot_samples) {
        finalize_symbol();
    }
}

void CwDecoderBase::handle_standard_idle_gap(uint32_t run) {
    if (!tone_on && !current_symbol.empty() &&
        run >= static_cast<uint32_t>((static_cast<float>(dsp::dsp_config.cw_word_gap_mult_x10) * 0.1f) * dot_samples)) {
        finalize_symbol();
        if (!pending_text.empty() && pending_text.back() != ' ') {
            pending_text.push_back(' ');
        }
        run_samples = 0;
    }
}

void CwDecoderBase::append_colored_char(char c, float confidence) {
    static constexpr char COLOR_MARK = '\x1B';

    uint8_t color_code = 2;
    if (confidence >= 0.80f) {
        color_code = 14;
    } else if (confidence >= 0.55f) {
        color_code = 3;
    }

    pending_text.push_back(COLOR_MARK);
    pending_text.push_back(static_cast<char>(color_code));
    pending_text.push_back(c);
}

void CwDecoderBase::finalize_symbol() {
    if (current_symbol.empty()) {
        return;
    }

    const char c = decode_symbol(current_symbol);
    const float conf = (symbol_conf_count > 0) ? (symbol_conf_accum / static_cast<float>(symbol_conf_count)) : 0.5f;

    if (c != '\0') {
        append_colored_char(c, conf);
    }

    symbol_conf_accum = 0.0f;
    symbol_conf_count = 0;
    current_symbol.clear();
}

char CwDecoderBase::decode_symbol(const std::string &s) const {
    struct Entry {
        const char *p;
        char c;
    };

    static const Entry table[] = {
        {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, {".", 'E'}, {"..-.", 'F'},
        {"--.", 'G'}, {"....", 'H'}, {"..", 'I'}, {".---", 'J'}, {"-.-", 'K'}, {".-..", 'L'},
        {"--", 'M'}, {"-.", 'N'}, {"---", 'O'}, {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'},
        {"...", 'S'}, {"-", 'T'}, {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'},
        {"-.--", 'Y'}, {"--..", 'Z'},
        {".----", '1'}, {"..---", '2'}, {"...--", '3'}, {"....-", '4'}, {".....", '5'},
        {"-....", '6'}, {"--...", '7'}, {"---..", '8'}, {"----.", '9'}, {"-----", '0'},
        {".-.-.-", '.'}, {"--..--", ','}, {"..--..", '?'}, {"-..-.", '/'}, {".--.-.", '@'}
    };

    for (const auto &e : table) {
        if (s == e.p) {
            return e.c;
        }
    }

    return '\0';
}

void CwEnvelopeDecoder::process_block(const float *samples, size_t count) {
    if (sample_rate == 0 || !samples || count == 0) {
        return;
    }

    const float alpha = 0.03f;

    for (size_t i = 0; i < count; ++i) {
        const float abs_sample = std::fabs(samples[i]);
        env += alpha * (abs_sample - env);

        const bool now_on = envelope_detect(env);
        run_samples++;

        if (now_on != tone_on) {
            const float min_run_factor = std::max(5.0f, static_cast<float>(dsp::dsp_config.cw_transition_min_dot_percent)) * 0.01f;
            const uint32_t min_run = static_cast<uint32_t>(std::max(1.0f, min_run_factor * dot_samples));
            if (run_samples >= min_run) {
                handle_standard_transition(now_on, static_cast<float>(run_samples), 0.75f);
                tone_on = now_on;
                run_samples = 0;
            }
        }
    }

    handle_standard_idle_gap(run_samples);
}

void CwGoertzelDecoder::configure(uint32_t sample_rate_hz, uint32_t tone_hz) {
    CwDecoderBase::configure(sample_rate_hz, tone_hz);

    if (sample_rate == 0) {
        return;
    }

    const float bin_hz = static_cast<float>(sample_rate) / static_cast<float>(GOERTZEL_WINDOW);
    const std::array<float, 3> freqs = {
        static_cast<float>(expected_tone_hz) - bin_hz,
        static_cast<float>(expected_tone_hz),
        static_cast<float>(expected_tone_hz) + bin_hz,
    };

    for (size_t i = 0; i < freqs.size(); ++i) {
        const float f = std::max(30.0f, freqs[i]);
        const float w = 2.0f * PI_F * f / static_cast<float>(sample_rate);
        goertzel_coeff[i] = 2.0f * std::cos(w);
    }

    reset_goertzel();
}

void CwGoertzelDecoder::reset() {
    CwDecoderBase::reset();
    reset_goertzel();
}

void CwGoertzelDecoder::reset_goertzel() {
    goertzel_s1 = {0.0f, 0.0f, 0.0f};
    goertzel_s2 = {0.0f, 0.0f, 0.0f};
    goertzel_count = 0;
    tone_gate_open = false;
    tone_snr_db = 0.0f;
}

void CwGoertzelDecoder::process_goertzel_sample(float x) {
    for (size_t i = 0; i < goertzel_coeff.size(); ++i) {
        const float s0 = x + goertzel_coeff[i] * goertzel_s1[i] - goertzel_s2[i];
        goertzel_s2[i] = goertzel_s1[i];
        goertzel_s1[i] = s0;
    }

    if (++goertzel_count >= GOERTZEL_WINDOW) {
        finalize_goertzel_window();
    }
}

void CwGoertzelDecoder::finalize_goertzel_window() {
    std::array<float, 3> power{};
    for (size_t i = 0; i < power.size(); ++i) {
        power[i] = goertzel_s1[i] * goertzel_s1[i] + goertzel_s2[i] * goertzel_s2[i] - goertzel_coeff[i] * goertzel_s1[i] * goertzel_s2[i];
    }

    const float tone = std::max(power[0], std::max(power[1], power[2]));
    const float sum = power[0] + power[1] + power[2];
    const float noise = std::max(1e-12f, (sum - tone) * 0.5f);
    tone_snr_db = 10.0f * std::log10((tone + 1e-12f) / noise);
    tone_gate_open = tone_snr_db > static_cast<float>(dsp::dsp_config.cw_goertzel_snr_db) && tone > 1e-5f;

    goertzel_s1 = {0.0f, 0.0f, 0.0f};
    goertzel_s2 = {0.0f, 0.0f, 0.0f};
    goertzel_count = 0;
}

void CwGoertzelDecoder::process_block(const float *samples, size_t count) {
    if (sample_rate == 0 || !samples || count == 0) {
        return;
    }

    const float alpha = 0.03f;

    for (size_t i = 0; i < count; ++i) {
        const float raw = samples[i];
        const float abs_sample = std::fabs(raw);
        env += alpha * (abs_sample - env);
        process_goertzel_sample(raw);

        bool now_on = envelope_detect(env);
        if (!tone_gate_open) {
            now_on = false;
        }

        run_samples++;

        if (now_on != tone_on) {
            const float min_run_factor = std::max(5.0f, static_cast<float>(dsp::dsp_config.cw_transition_min_dot_percent)) * 0.01f;
            const uint32_t min_run = static_cast<uint32_t>(std::max(1.0f, min_run_factor * dot_samples));
            if (run_samples >= min_run) {
                const float snr_conf = clampf((tone_snr_db - 2.0f) / 10.0f, 0.0f, 1.0f);
                handle_standard_transition(now_on, static_cast<float>(run_samples), snr_conf);
                tone_on = now_on;
                run_samples = 0;
            }
        }
    }

    handle_standard_idle_gap(run_samples);
}

void CwMayhemDecoder::reset_mayhem() {
    mayhem_time_unit_ms = 120.0f;
}

void CwMayhemDecoder::reset() {
    CwDecoderBase::reset();
    reset_mayhem();
}

void CwMayhemDecoder::update_time_unit(float duration_ms, bool dash) {
    if (duration_ms <= 0.0f) {
        return;
    }

    const float candidate = dash ? (duration_ms / 3.0f) : duration_ms;
    mayhem_time_unit_ms = 0.88f * mayhem_time_unit_ms + 0.12f * candidate;
    mayhem_time_unit_ms = clampf(mayhem_time_unit_ms, 25.0f, 400.0f);
}

void CwMayhemDecoder::handle_pulse_ms(float duration_ms) {
    if (duration_ms <= 0.0f) {
        return;
    }

    const bool dash = duration_ms >= (2.0f * mayhem_time_unit_ms);
    const float target = dash ? (3.0f * mayhem_time_unit_ms) : mayhem_time_unit_ms;
    const float timing_conf = clampf(1.0f - (std::fabs(duration_ms - target) / std::max(1.0f, target)), 0.0f, 1.0f);

    add_symbol_confidence(timing_conf);
    current_symbol.push_back(dash ? '-' : '.');
    update_time_unit(duration_ms, dash);
}

void CwMayhemDecoder::handle_gap_ms(float duration_ms) {
    const float letter_gap_ms = 2.5f * mayhem_time_unit_ms;
    const float word_gap_ms = 6.0f * mayhem_time_unit_ms;

    if (duration_ms >= word_gap_ms) {
        finalize_symbol();
        if (!pending_text.empty() && pending_text.back() != ' ') {
            pending_text.push_back(' ');
        }
    } else if (duration_ms >= letter_gap_ms) {
        finalize_symbol();
    }
}

void CwMayhemDecoder::process_block(const float *samples, size_t count) {
    if (sample_rate == 0 || !samples || count == 0) {
        return;
    }

    const float alpha = 0.03f;

    for (size_t i = 0; i < count; ++i) {
        const float abs_sample = std::fabs(samples[i]);
        env += alpha * (abs_sample - env);

        const bool now_on = envelope_detect(env);
        run_samples++;

        if (now_on != tone_on) {
            const float ref_dot_samples = (mayhem_time_unit_ms * static_cast<float>(sample_rate)) / 1000.0f;
            const float min_run_factor = std::max(5.0f, static_cast<float>(dsp::dsp_config.cw_transition_min_dot_percent)) * 0.01f;
            const uint32_t min_run = static_cast<uint32_t>(std::max(1.0f, min_run_factor * ref_dot_samples));
            if (run_samples >= min_run) {
                const float duration_ms = (static_cast<float>(run_samples) * 1000.0f) / static_cast<float>(sample_rate);
                if (!now_on) {
                    handle_pulse_ms(duration_ms);
                } else {
                    handle_gap_ms(duration_ms);
                }
                tone_on = now_on;
                run_samples = 0;
            }
        }
    }

    if (!tone_on && !current_symbol.empty()) {
        const float duration_ms = (static_cast<float>(run_samples) * 1000.0f) / static_cast<float>(sample_rate);
        if (duration_ms >= (6.0f * mayhem_time_unit_ms)) {
            finalize_symbol();
            if (!pending_text.empty() && pending_text.back() != ' ') {
                pending_text.push_back(' ');
            }
            run_samples = 0;
        }
    }
}

CwDecoder::CwDecoder() {
    select_algorithm(dsp::dsp_config.cw_decode_algorithm);
}

CwDecoder::~CwDecoder() {
    destroy_current();
}

void CwDecoder::destroy_current() {
    if (!decoder_) {
        return;
    }

    switch (algorithm_) {
        case dsp::CW_DECODE_ENVELOPE:
            reinterpret_cast<CwEnvelopeDecoder *>(&storage_)->~CwEnvelopeDecoder();
            break;
        case dsp::CW_DECODE_MAYHEM:
            reinterpret_cast<CwMayhemDecoder *>(&storage_)->~CwMayhemDecoder();
            break;
        case dsp::CW_DECODE_GOERTZEL:
        default:
            reinterpret_cast<CwGoertzelDecoder *>(&storage_)->~CwGoertzelDecoder();
            break;
    }

    decoder_ = nullptr;
}

void CwDecoder::select_algorithm(dsp::CwDecodeAlgorithm algorithm) {
    if (decoder_ && algorithm == algorithm_) {
        return;
    }

    destroy_current();

    algorithm_ = algorithm;
    switch (algorithm_) {
        case dsp::CW_DECODE_ENVELOPE:
            decoder_ = new (&storage_) CwEnvelopeDecoder();
            break;
        case dsp::CW_DECODE_MAYHEM:
            decoder_ = new (&storage_) CwMayhemDecoder();
            break;
        case dsp::CW_DECODE_GOERTZEL:
        default:
            decoder_ = new (&storage_) CwGoertzelDecoder();
            break;
    }

    if (sample_rate_ != 0) {
        decoder_->configure(sample_rate_, tone_hz_);
    }
}

void CwDecoder::configure(uint32_t sample_rate_hz, uint32_t tone_hz) {
    sample_rate_ = sample_rate_hz;
    tone_hz_ = tone_hz;
    select_algorithm(dsp::dsp_config.cw_decode_algorithm);
    decoder_->configure(sample_rate_hz, tone_hz);
}

void CwDecoder::reset() {
    select_algorithm(dsp::dsp_config.cw_decode_algorithm);
    decoder_->reset();
}

void CwDecoder::process_block(const float *samples, size_t count) {
    select_algorithm(dsp::dsp_config.cw_decode_algorithm);
    decoder_->process_block(samples, count);
}

std::string CwDecoder::take_text() {
    if (!decoder_) {
        return {};
    }
    return decoder_->take_text();
}

} // namespace cw_decode
