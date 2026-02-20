/*
 * Sharebrained wrote in matched_filter.hpp that taps should be those of a complex low-pass filter combined with a complex sinusoid, so
 * that the filter shifts the spectrum where we want (signal of interest around 0Hz).
 *
 * In this baseband processor, after decim_0 and decim_1, the signal ends up being sampled at 38400Hz (2457600 / 8 / 8)
 * Since the applied shift in ui_sonde.cpp is -fs/4 = -2457600/4 = -614400Hz to avoid the DC spike, the FSK signal ends up being
 * shifted by 614400 / 8 / 8 = 9600Hz. So decim_1_out should look like this:
 *
 *   _______________|______/'\______
 * -C               A       B       C
 *
 * A is the DC spike at 0Hz
 * B is the FSK signal shifted right at 9600Hz
 * C is the bandwidth edge at 19200Hz
 *
 * Taps should be computed to shift the whole spectrum by -9600Hz ("left") so that it looks like this:
 *
 *   ______________/'\______________
 * -C               D               C
 *
 * Anything unwanted (like A) should have been filtered off
 * D is B around 0Hz now
 *
 * Then the clock_recovery function should be happy :)
 *
 * Mathworks.com says:
 * In the case of a single-rate FIR design, we simply multiply each set of coefficients by (aka 'heterodyne with') a complex exponential.
 *
 * Can SciPy's remez function be used for this ? See tools/firtest.py
 * GnuRadio's firdes only outputs an odd number of taps
 *
 * ---------------------------------------------------------------------
 *
 * Looking at the AIS baseband processor:
 *
 * Copied everything necessary to get decim_1_out (so same 8 * 8 = 64 decimation factor)
 * The samplerate is also the same (2457600)
 * After the matching filter, the data is decimated by 2 so the final samplerate for clock_recovery is 38400 / 2 = 19200Hz.
 * Like here, the shift used is fs/4, so decim_1_out should be looking similar.
 * The AIS signal deviates by 2400 (4800Hz signal width), the symbol rate is 9600.
 *
 * The matched filter's input samplerate is 38400Hz, to get a 9600Hz shift it must use 4 taps ?
 * To obtain unity gain, the sinusoid length must be / by the number of taps ?
 *
 *
 * */

#ifndef __PROC_SONDE_H__
#define __PROC_SONDE_H__

#include "dsp/receive/receive_task_base.h"
#include "radiosonde_task.hpp"

#include "../blocks/matched_filter.hpp"

#include "../receive/clock_recovery.hpp"
#include "../protocols/symbol_coding.hpp"
#include "../protocols/packet_builder.hpp"
#include "../protocols/baseband_packet.hpp"
#include "radiosonde_packet.hpp"
#include "dsp/task.h"
#include "dsp/blocks/beep_generator.h"

#include <cstdint>
#include <cstddef>
#include <bitset>

#define BEEP_MIN_DURATION 60
#define BEEP_DURATION_RANGE 100
#define BEEP_BASE_FREQ 400 // Lowest audible freq for some speakers
#define BEEP_MAX_FREQ 8000 // Highest audible freq for some speakers
#define BEEP_SIMPLE_FREQ 1000
#define RSSI_CEILING 1000
#define PROPORTIONAL_BEEP_THRES 0.8
#define RSSI_PITCH_WEIGHT (float(BEEP_MAX_FREQ - BEEP_BASE_FREQ) / RSSI_CEILING)
#define DEFAULT_AUDIO_SAMPLE_RATE 24000

extern Signal radiosonde_signal;
namespace dsp {

class RadiosondeTask : public ReceiveTaskBase {
  public:
    using ReceiveTaskBase::ReceiveTaskBase;

    ~RadiosondeTask() override;

    void update_rssi();

    void enable_beeper(bool b) {
        beeper_enabled = b;
        if (beeper_enabled) {
            set_beeper();
        }
    }

    bool get_beeper_enabled() {
        return beeper_enabled;
    }

  private:
    // Taps for a matched filter. They also shift frequency
    // Rectangular window filter
    // sample=38.4k, deviation=2400, symbol=9600
    // Length: 4 taps, 1 symbol, 1/4 cycle of sinusoid
    // Gain: 1.0 (sinusoid / len(taps))
    static constexpr std::array<std::complex<float>, 4> square_taps_38k4_1t_p{{
        {0.25000000f, 0.00000000f},
        {0.23096988f, 0.09567086f},
        {0.17677670f, 0.17677670f},
        {0.09567086f, 0.23096988f},
    }};

    bool pitch_rssi_enabled{false};

    uint32_t last_rssi{0};
    uint32_t beep_freq{0};

    dsp::matched_filter::MatchedFilter mf{square_taps_38k4_1t_p, 2};

    // Actually 480 0bits/s but the Manchester coding doubles the symbol rate
    clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery_fsk_9600{19200, 9600, {0.0555f}, [this](const float raw_symbol) {
                                                                                                const uint_fast8_t sliced_symbol = (raw_symbol >= 0.0f) ? 1 : 0;
                                                                                                this->packet_builder_fsk_9600_Meteomodem.execute(sliced_symbol);
                                                                                            }};
    PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_fsk_9600_Meteomodem{
        {0b00110011001100110101100110110011, 32, 1}, {}, {88 * 2 * 8}, [](const baseband::Packet &packet) {
            radiosonde_signal.emit((void *)&packet);
        }};

    clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery_fsk_4800{19200, 4800, {0.0555f}, [this](const float raw_symbol) {
                                                                                                const uint_fast8_t sliced_symbol = (raw_symbol >= 0.0f) ? 1 : 0;
                                                                                                this->packet_builder_fsk_4800_Vaisala.execute(sliced_symbol);
                                                                                            }};
    PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_fsk_4800_Vaisala{
        {0b00001000011011010101001110001000, 32,
         1}, // euquiq Header detects 4 of 8 bytes 0x10B6CA11 /this is in raw format) (these bits are not passed at the beginning of packet)
        //{ 0b0000100001101101010100111000100001000100011010010100100000011111, 64, 1 }, //euquiq whole header detection would be 8 bytes.
        {},
        {320 * 8},
        [&](const baseband::Packet &packet) {
            radiosonde_signal.emit((void *)&packet);
            on_packet();
        }};

    BeepGenerator beeper{};
    bool beeper_enabled{true};

    void set_beeper();
    void on_packet();
    //   void on_beep_message(const AudioBeepMessage &message);
    //  void on_pitch_rssi_config(const PitchRSSIConfigureMessage &message);

    bool init() override;
    void process_audio(buffer_t<float32_t> &buff_out_f32) override;

    MODULATION_MODE get_modulation_mode() const override {
        return NONE;
    };

    uint32_t get_audio_sample_rate() const override {
        return 12000;
    };

    uint32_t get_modulation_bw_hz() const override {
        return 12000;
    };

    /* Bandwidth of the output audio stream */
    uint32_t get_audio_bw_hz() const override {
        return 2800;
    };
};
} // namespace dsp
#endif /*__PROC_ERT_H__*/
