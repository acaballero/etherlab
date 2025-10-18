
#include "radiosonde_task.hpp"

RadiosondeTask::RadiosondeTask() {
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);

    baseband_thread.start();
}

void RadiosondeTask::execute(const buffer_c8_t &buffer) {
    /* 2.4576MHz, 2048 samples */

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto decimator_out = decim_1_out;

    /* 38.4kHz, 32 samples */
    feed_channel_stats(decimator_out);

    for (size_t i = 0; i < decimator_out.count; i++) {
        if (mf.execute_once(decimator_out.p[i])) {
            clock_recovery_fsk_9600(mf.get_output());
            clock_recovery_fsk_4800(mf.get_output());
        }
    }
}

void RadiosondeTask::on_message(const Message *const msg) {
    switch (msg->id) {
        case Message::ID::RequestSignal:
            on_signal_message(*reinterpret_cast<const RequestSignalMessage *>(msg));
            break;

        case Message::ID::AudioBeep:
            on_beep_message(*reinterpret_cast<const AudioBeepMessage *>(msg));
            break;

        case Message::ID::PitchRSSIConfigure:
            on_pitch_rssi_config(*reinterpret_cast<const PitchRSSIConfigureMessage *>(msg));
            break;

        default:
            break;
    }
}

void RadiosondeTask::on_signal_message(const RequestSignalMessage &message) {
    if (message.signal == RequestSignalMessage::Signal::RSSIBeepRequest) {
        float rssi_ratio = (float)last_rssi / RSSI_CEILING;
        uint32_t beep_duration = 0;

        if (rssi_ratio <= PROPORTIONAL_BEEP_THRES) {
            beep_duration = BEEP_MIN_DURATION;
        } else if (rssi_ratio < 1) {
            beep_duration = rssi_ratio * BEEP_DURATION_RANGE + BEEP_MIN_DURATION;
        } else {
            beep_duration = BEEP_DURATION_RANGE + BEEP_MIN_DURATION;
        }

        audio::dma::beep_start(beep_freq, DEFAULT_AUDIO_SAMPLE_RATE, beep_duration);
    }
}

void RadiosondeTask::on_beep_message(const AudioBeepMessage &message) {
    audio::dma::beep_start(message.freq, message.sample_rate, message.duration_ms);
}

void RadiosondeTask::on_pitch_rssi_config(const PitchRSSIConfigureMessage &message) {
    pitch_rssi_enabled = message.enabled;

    beep_freq = message.rssi * RSSI_PITCH_WEIGHT + BEEP_BASE_FREQ;
    last_rssi = message.rssi;
}
