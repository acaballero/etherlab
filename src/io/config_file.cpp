//
// Created by Angel Dust on 05/09/2025.
//
#include "config_file.h"
#include "dsp/aprs/aprs_settings.h"
#include "fatfs/fatfs.h"
#include "config.h"
#include "ff.h"
#include "printf.h"
#include "types.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

template class ConfigFile<st_config>;
template class ConfigFile<st_freq_mem>;

template <> bool ConfigFile<st_config>::save(const st_config *cfg) {

    UINT bw;

    WRITE_FIELD("version=%s", cfg->version);
    WRITE_FIELD("debug=%d", cfg->debug);
    WRITE_FIELD("power_ctrl=%d", cfg->power_ctrl);
    WRITE_FIELD("filter=%d", cfg->filter);
    WRITE_FIELD("if_filter=%d", cfg->if_filter);
    WRITE_FIELD("frontend_path=%d", cfg->frontend_path);
    WRITE_FIELD("modulation=%d", cfg->modulation);
    WRITE_FIELD("mode=%d", cfg->mode);
    WRITE_FIELD("band=%d", cfg->band);
    WRITE_FIELD("hpa_enabled=%d", cfg->hpa_enabled);
    WRITE_FIELD("max_power_dbm=%d", cfg->max_power_dbm);
    WRITE_FIELD("lo_injection=%d", cfg->lo_injection);
    WRITE_FIELD("power_save_period_seconds=%d", cfg->power_save_period_seconds);
    WRITE_FIELD("f_1st_if=%u", cfg->f_1st_if);
    WRITE_FIELD("f_if_fm_tx=%u", cfg->f_if_fm_tx);

    WRITE_FIELD("vfo_ix=%d", cfg->vfo_ix);
    for (int i = 0; i < 3; ++i) {
        WRITE_FIELD("vfo[%d].freq=%u", i, cfg->vfo[i].freq);
        WRITE_FIELD("vfo[%d].step=%u", i, cfg->vfo[i].step);
        WRITE_FIELD("vfo[%d].rit=%d", i, cfg->vfo[i].rit);
        WRITE_FIELD("vfo[%d].modulation=%d", i, cfg->vfo[i].mode);
    }

    WRITE_FIELD("memory_mode=%d", cfg->memory_mode);
    WRITE_FIELD("f_carrier=%lu", cfg->f_carrier);
    WRITE_FIELD("f_step=%lu", cfg->f_step);
    WRITE_FIELD("f_max=%lu", cfg->f_max);
    WRITE_FIELD("f_min=%lu", cfg->f_min);

    WRITE_FIELD("repeater_offset=%u", cfg->repeater_offset);
    WRITE_FIELD("repeater_mode=%d", cfg->repeater_mode);

    WRITE_FIELD("fft.span=%u", cfg->fft.span);
    WRITE_FIELD("fft.bw=%u", cfg->fft.bw);
    WRITE_FIELD("fft.min_db=%d", cfg->fft.min_db);
    WRITE_FIELD("fft.max_db=%d", cfg->fft.max_db);
    WRITE_FIELD("fft.min_db_auto=%d", cfg->fft.min_db_auto);
    WRITE_FIELD("fft.view_mode=%d", cfg->fft.view_mode);
    WRITE_FIELD("fft.smooth_factor=%f", cfg->fft.smooth_factor);
    WRITE_FIELD("fft.enabled=%d", cfg->fft.enabled);
    WRITE_FIELD("fft.refresh_period_ms=%d", cfg->fft.refresh_period_ms);
    WRITE_FIELD("fft.max_slices=%d", cfg->fft.max_slices);
    WRITE_FIELD("fft.maxAmpl=%d", cfg->fft.maxAmpl);
    WRITE_FIELD("fft.view_IQBalance=%d", cfg->fft.view_IQBalance);
    WRITE_FIELD("fft.enable_iq_balance=%d", cfg->fft.enable_iq_balance);
    WRITE_FIELD("fft.conversion_time_us=%d", cfg->fft.conversion_time_us);
    WRITE_FIELD("fft.waterfall_pixels_per_second=%d", cfg->fft.waterfall_pixels_per_second);
    WRITE_FIELD("fft.waterfall_mode=%d", cfg->fft.waterfall_mode);
    WRITE_FIELD("fft.DCOffset_I=%d", cfg->fft.DCOffset_I);
    WRITE_FIELD("fft.DCOffset_Q=%d", cfg->fft.DCOffset_Q);
    WRITE_FIELD("fft.iq_balance_estimate_period_ms=%d", cfg->fft.iq_balance_estimate_period_ms);
    WRITE_FIELD("fft.removeDC=%d", cfg->fft.removeDC);
    WRITE_FIELD("fft.window=%d", cfg->fft.window);
    WRITE_FIELD("fft.sample_rate=%u", cfg->fft.sample_rate);
    WRITE_FIELD("fft.max_sample_rate=%u", cfg->fft.max_sample_rate);
    WRITE_FIELD("fft.dsp_max_sample_rate=%u", cfg->fft.dsp_max_sample_rate);
    WRITE_FIELD("fft.min_sample_rate=%u", cfg->fft.min_sample_rate);
    WRITE_FIELD("fft.max_decimation_factor=%d", cfg->fft.max_decimation_factor);
    WRITE_FIELD("fft.spectrum_style=%d", cfg->fft.spectrum_style);
    WRITE_FIELD("fft.spectrum_line_color=%d", cfg->fft.spectrum_line_color);
    WRITE_FIELD("fft.spectrum_fill_color=%d", cfg->fft.spectrum_fill_color);

    write_bin("fft.iq_balance_meanZ=", (uint8_t *)cfg->fft.iq_balance_meanZ, sizeof(cfg->fft.iq_balance_meanZ));
    write_bin("fft.iq_balance_precZ=", reinterpret_cast<const uint8_t *>(cfg->fft.iq_balance_precZ), sizeof(cfg->fft.iq_balance_precZ));

    WRITE_FIELD("dsp.gain=%d", cfg->dsp.gain);
    WRITE_FIELD("dsp.audio_compressor_enabled=%d", cfg->dsp.audio_compressor_enabled);
    WRITE_FIELD("dsp.deemphasis_enabled=%d", cfg->dsp.deemphasis_enabled);
    WRITE_FIELD("dsp.audio_bpf_enabled=%d", cfg->dsp.audio_bpf_enabled);
    WRITE_FIELD("dsp.audio_compressor_threshold=%d", cfg->dsp.audio_compressor_threshold);
    WRITE_FIELD("dsp.test_signal.pulse_duty=%d", cfg->dsp.test_signal.pulse_duty);
    WRITE_FIELD("dsp.test_signal.baseband_frequency=%u", cfg->dsp.test_signal.baseband_frequency);
    WRITE_FIELD("dsp.test_signal.modulation_frequency=%u", cfg->dsp.test_signal.modulation_frequency);
    WRITE_FIELD("dsp.test_signal.shape=%d", cfg->dsp.test_signal.shape);

    WRITE_FIELD("hw.cmx973_vga=%d", cfg->hw.cmx973_vga);
    WRITE_FIELD("hw.cmx973_vgb=%d", cfg->hw.cmx973_vgb);
    WRITE_FIELD("hw.sd_write_max_kbps=%u", cfg->hw.sd_write_max_kbps);
    WRITE_FIELD("hw.offset=%d", cfg->hw.dac_offset);
    WRITE_FIELD("hw.dac_off_balance=%d", cfg->hw.dac_off_balance);
    WRITE_FIELD("hw.dac_amp_balance=%f", cfg->hw.dac_amp_balance);

    WRITE_FIELD("coupler_0db_mv=%d", cfg->coupler_0db_mv);
    WRITE_FIELD("f_correction=%d", cfg->f_correction);
    WRITE_FIELD("if_correction=%d", cfg->if_correction);
    WRITE_FIELD("squelch_auto=%d", cfg->squelch_auto);
    WRITE_FIELD("squelch_level=%f", cfg->squelch_level);
    WRITE_FIELD("agc_enabled=%d", cfg->agc_enabled);
    WRITE_FIELD("enable_quadrature=%d", cfg->enable_quadrature);

    return true;
}

template <> bool ConfigFile<st_config>::load(st_config *cfg) {

    char fmt[50];

    read_string("version=", cfg->version);
    read_bool("debug=", &cfg->debug);
    read_uint8("power_ctrl=", &cfg->power_ctrl);
    read_int("filter=", (int32_t *)&cfg->filter);
    read_int("if_filter=", (int32_t *)&cfg->if_filter);
    read_int("frontend_path=", (int32_t *)&cfg->frontend_path);
    read_int("modulation=", (int32_t *)&cfg->modulation);
    read_int("mode=", (int32_t *)&cfg->mode);
    read_int("band=", (int32_t *)&cfg->band);
    read_bool("hpa_enabled=", &cfg->hpa_enabled);
    read_uint8("max_power_dbm=", &cfg->max_power_dbm);
    read_int("lo_injection=", (int32_t *)&cfg->lo_injection);
    read_uint8("power_save_period_seconds=", &cfg->power_save_period_seconds);
    read_uint("f_1st_if=", &cfg->f_1st_if);
    read_uint("f_if_fm_tx=", &cfg->f_if_fm_tx);
    read_uint8("vfo_ix=", &cfg->vfo_ix);

    for (int i = 0; i < 3; ++i) {
        sprintf(fmt, "vfo[%d].freq=", i);
        read_uint(fmt, &cfg->vfo[i].freq);
        sprintf(fmt, "vfo[%d].step=", i);
        read_uint(fmt, &cfg->vfo[i].step);
        sprintf(fmt, "vfo[%d].rit=", i);
        read_int(fmt, &cfg->vfo[i].rit);
        sprintf(fmt, "vfo[%d].modulation=", i);
        read_int(fmt, (int32_t *)&cfg->vfo[i].mode);
    }

    read_bool("memory_mode=", &cfg->memory_mode);
    read_uint("f_carrier=", &cfg->f_carrier);
    read_uint("f_step=", &cfg->f_step);
    read_uint("f_max=", &cfg->f_max);
    read_uint("f_min=", &cfg->f_min);

    read_uint("repeater_offset=", &cfg->repeater_offset);
    read_int("repeater_mode=", (int32_t *)&cfg->repeater_mode);

    read_uint("fft.span=", &cfg->fft.span);
    read_uint("fft.bw=", &cfg->fft.bw);
    read_int16("fft.min_db=", &cfg->fft.min_db);
    read_int16("fft.max_db=", &cfg->fft.max_db);
    read_bool("fft.min_db_auto=", &cfg->fft.min_db_auto);
    read_uint8("fft.view_mode=", &cfg->fft.view_mode);
    read_float("fft.smooth_factor=", &cfg->fft.smooth_factor);
    read_bool("fft.enabled=", &cfg->fft.enabled);
    read_uint8("fft.refresh_period_ms=", &cfg->fft.refresh_period_ms);
    read_uint8("fft.max_slices=", &cfg->fft.max_slices);
    read_int("fft.maxAmpl=", &cfg->fft.maxAmpl);
    read_bool("fft.view_IQBalance=", &cfg->fft.view_IQBalance);
    read_bool("fft.enable_iq_balance=", &cfg->fft.enable_iq_balance);
    read_uint8("fft.conversion_time_us=", &cfg->fft.conversion_time_us);
    read_uint16("fft.waterfall_pixels_per_second=", &cfg->fft.waterfall_pixels_per_second);
    read_int("fft.waterfall_mode=", (int32_t *)&cfg->fft.waterfall_mode);

    read_int16("fft.DCOffset_I=", &cfg->fft.DCOffset_I);
    read_int16("fft.DCOffset_Q=", &cfg->fft.DCOffset_Q);
    read_uint8("fft.iq_balance_estimate_period_ms=", &cfg->fft.iq_balance_estimate_period_ms);
    read_bool("fft.removeDC=", &cfg->fft.removeDC);
    read_uint8("fft.window=", &cfg->fft.window);
    read_uint("fft.sample_rate=", &cfg->fft.sample_rate);
    read_uint("fft.max_sample_rate=", &cfg->fft.max_sample_rate);
    read_uint("fft.dsp_max_sample_rate=", &cfg->fft.dsp_max_sample_rate);
    read_uint("fft.min_sample_rate=", &cfg->fft.min_sample_rate);
    read_uint8("fft.max_decimation_factor=", &cfg->fft.max_decimation_factor);
    read_int("fft.spectrum_style=", (int32_t *)&cfg->fft.spectrum_style);
    read_uint16("fft.spectrum_line_color=", &cfg->fft.spectrum_line_color);
    read_uint16("fft.spectrum_fill_color=", &cfg->fft.spectrum_fill_color);

    read_bin("fft.iq_balance_meanZ=", reinterpret_cast<uint8_t *>(cfg->fft.iq_balance_meanZ), sizeof(cfg->fft.iq_balance_meanZ));
    read_bin("fft.iq_balance_precZ=", reinterpret_cast<uint8_t *>(cfg->fft.iq_balance_precZ), sizeof(cfg->fft.iq_balance_precZ));

    read_int8("dsp.gain=", &cfg->dsp.gain);
    read_bool("dsp.audio_compressor_enabled=", &cfg->dsp.audio_compressor_enabled);
    read_bool("dsp.deemphasis_enabled=", &cfg->dsp.deemphasis_enabled);
    read_bool("dsp.audio_bpf_enabled=", &cfg->dsp.audio_bpf_enabled);
    read_int("dsp.audio_compressor_threshold=", &cfg->dsp.audio_compressor_threshold);
    read_int8("dsp.test_signal.pulse_duty=", &cfg->dsp.test_signal.pulse_duty);
    read_uint("dsp.test_signal.baseband_frequency=", &cfg->dsp.test_signal.baseband_frequency);
    read_uint("dsp.test_signal.modulation_frequency=", &cfg->dsp.test_signal.modulation_frequency);
    read_uint8("dsp.test_signal.shape=", &cfg->dsp.test_signal.shape);

    read_int("hw.cmx973_vga=", (int32_t *)&cfg->hw.cmx973_vga);
    read_int("hw.cmx973_vgb=", (int32_t *)&cfg->hw.cmx973_vgb);
    read_uint("hw.sd_write_max_kbps=", &cfg->hw.sd_write_max_kbps);
    read_uint16("hw.offset=", &cfg->hw.dac_offset);
    read_int16("hw.dac_off_balance=", &cfg->hw.dac_off_balance);
    read_float("hw.dac_amp_balance=", &cfg->hw.dac_amp_balance);

    read_uint16("coupler_0db_mv=", &cfg->coupler_0db_mv);
    read_int("f_correction=", &cfg->f_correction);
    read_int("if_correction=", &cfg->if_correction);
    read_bool("squelch_auto=", &cfg->squelch_auto);
    read_float("squelch_level=", &cfg->squelch_level);
    read_bool("agc_enabled=", &cfg->agc_enabled);
    read_bool("enable_quadrature=", &cfg->enable_quadrature);

    return true;
}
