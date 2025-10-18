
#include "radiosonde_task.hpp"
#include "agc.h"

void RadiosondeTask::process_audio(buffer_t<float32_t> &buff_out_f32) {

    for (size_t i = 0; i < buff_out_f32.count; i++) {
        if (mf.execute_once(buff_out_f32.p[i])) {
            clock_recovery_fsk_9600(mf.get_output());
            clock_recovery_fsk_4800(mf.get_output());
        }
    }
}

void RadiosondeTask::on_packet() {

    float rssi_ratio = (float)last_rssi / RSSI_CEILING;
    uint32_t beep_duration = 0;

    if (rssi_ratio <= PROPORTIONAL_BEEP_THRES) {
        beep_duration = BEEP_MIN_DURATION;
    } else if (rssi_ratio < 1) {
        beep_duration = rssi_ratio * BEEP_DURATION_RANGE + BEEP_MIN_DURATION;
    } else {
        beep_duration = BEEP_DURATION_RANGE + BEEP_MIN_DURATION;
    }

    beeper.init({beep_freq, 0, beep_duration, 0, 1, 0.7f, SIGNAL_SHAPE_SIN, 10});
}

void RadiosondeTask::update_rssi() {
    pitch_rssi_enabled = true;

    beep_freq = fft::dbm * RSSI_PITCH_WEIGHT + BEEP_BASE_FREQ;
    last_rssi = fft::dbm;
}
