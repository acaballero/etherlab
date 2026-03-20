#ifndef TRX_FRONTEND_CW_DECODER_H
#define TRX_FRONTEND_CW_DECODER_H

#include "Signal.h"
#include "dsp/dsp_common.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <type_traits>

namespace cw_decode {

struct text_event {
    char text[48];
};

extern Signal text_signal;

class CwDecoderBase {
  public:
    virtual ~CwDecoderBase() = default;

    virtual void configure(uint32_t sample_rate_hz, uint32_t tone_hz);
    virtual void reset();
    virtual void process_block(const float *samples, size_t count) = 0;

    std::string take_text();

  protected:
    void reset_common();
    bool envelope_detect(float abs_sample);
    void handle_standard_transition(bool now_on, float run_samples_f, float snr_conf);
    void handle_standard_idle_gap(uint32_t run_samples);
    void finalize_symbol();
    char decode_symbol(const std::string &symbol) const;
    void append_colored_char(char c, float confidence);
    void add_symbol_confidence(float confidence);

    uint32_t sample_rate = 0;
    uint32_t expected_tone_hz = 0;

    float env = 0.0f;
    float noise_floor = 0.002f;
    float signal_floor = 0.03f;

    bool tone_on = false;
    uint32_t run_samples = 0;

    float dot_samples = 0.0f;
    std::string current_symbol;
    std::string pending_text;

    float symbol_conf_accum = 0.0f;
    uint8_t symbol_conf_count = 0;
};

class CwEnvelopeDecoder : public CwDecoderBase {
  public:
    void process_block(const float *samples, size_t count) override;
};

class CwGoertzelDecoder : public CwDecoderBase {
  public:
    void configure(uint32_t sample_rate_hz, uint32_t tone_hz) override;
    void reset() override;
    void process_block(const float *samples, size_t count) override;

  private:
    void reset_goertzel();
    void process_goertzel_sample(float x);
    void finalize_goertzel_window();

    static constexpr uint32_t GOERTZEL_WINDOW = 512;
    std::array<float, 3> goertzel_coeff{};
    std::array<float, 3> goertzel_s1{};
    std::array<float, 3> goertzel_s2{};
    uint32_t goertzel_count = 0;
    bool tone_gate_open = false;
    float tone_snr_db = 0.0f;
};

class CwMayhemDecoder : public CwDecoderBase {
  public:
    void reset() override;
    void process_block(const float *samples, size_t count) override;

  private:
    void reset_mayhem();
    void handle_pulse_ms(float duration_ms);
    void handle_gap_ms(float duration_ms);
    void update_time_unit(float duration_ms, bool dash);

    float mayhem_time_unit_ms = 120.0f;
};

class CwDecoder {
  public:
    CwDecoder();
    ~CwDecoder();

    void configure(uint32_t sample_rate_hz, uint32_t tone_hz);
    void reset();
    void process_block(const float *samples, size_t count);
    std::string take_text();

  private:
    template <size_t A, size_t B> struct static_max {
        static constexpr size_t value = A > B ? A : B;
    };

    static constexpr size_t storage_size = static_max<sizeof(CwEnvelopeDecoder),
                                                      static_max<sizeof(CwGoertzelDecoder), sizeof(CwMayhemDecoder)>::value>::value;
    static constexpr size_t storage_align = static_max<alignof(CwEnvelopeDecoder),
                                                       static_max<alignof(CwGoertzelDecoder), alignof(CwMayhemDecoder)>::value>::value;

    using storage_t = typename std::aligned_storage<storage_size, storage_align>::type;

    void destroy_current();
    void select_algorithm(dsp::CwDecodeAlgorithm algorithm);

    storage_t storage_{};
    CwDecoderBase *decoder_ = nullptr;
    dsp::CwDecodeAlgorithm algorithm_ = dsp::CW_DECODE_GOERTZEL;
    uint32_t sample_rate_ = 0;
    uint32_t tone_hz_ = 0;
};

} // namespace cw_decode

#endif // TRX_FRONTEND_CW_DECODER_H
