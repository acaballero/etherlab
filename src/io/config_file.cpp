//
// Created by Angel Dust on 05/09/2025.
//
#include "config_file.h"
#include "fatfs/fatfs.h"
#include "config.h"
#include "printf.h"
#include "types.h"
#include <cstdio>
#include <cstring>

bool ConfigFile::save(const char *filename, const st_config &cfg) {

    FIL *file = &FatFSFileHandle;

    if (!lock_sd_card()) {
        return false;
    }

    if (f_open(file, filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        return false;
    }

    char buf[256];
    UINT bw;

#define WRITE_FIELD(fmt, ...)                                                                                                                                  \
    snprintf(buf, sizeof(buf), fmt "\n", __VA_ARGS__);                                                                                                         \
    f_write(file, buf, strlen(buf), &bw)

    WRITE_FIELD("version=%s", cfg.version);
    WRITE_FIELD("debug=%d", cfg.debug);
    WRITE_FIELD("power_ctrl=%d", cfg.power_ctrl);
    WRITE_FIELD("filter=%d", cfg.filter);
    WRITE_FIELD("if_filter=%d", cfg.if_filter);
    WRITE_FIELD("frontend_path=%d", cfg.frontend_path);
    WRITE_FIELD("modulation=%d", cfg.modulation);
    WRITE_FIELD("mode=%d", cfg.mode);
    WRITE_FIELD("band=%d", cfg.band);
    WRITE_FIELD("hpa_enabled=%d", cfg.hpa_enabled);
    WRITE_FIELD("max_power_dbm=%d", cfg.max_power_dbm);
    WRITE_FIELD("lo_injection=%d", cfg.lo_injection);
    WRITE_FIELD("power_save_period_seconds=%d", cfg.power_save_period_seconds);
    WRITE_FIELD("f_1st_if=%u", cfg.f_1st_if);
    WRITE_FIELD("f_if_fm_tx=%u", cfg.f_if_fm_tx);

    WRITE_FIELD("vfo_ix=%d", cfg.vfo_ix);
    for (int i = 0; i < 2; ++i) {
        WRITE_FIELD("vfo[%d].freq=%u", i, cfg.vfo[i].freq);
        WRITE_FIELD("vfo[%d].step=%u", i, cfg.vfo[i].step);
        WRITE_FIELD("vfo[%d].rit=%d", i, cfg.vfo[i].rit);
    }

    WRITE_FIELD("memory_mode=%d", cfg.memory_mode);
    WRITE_FIELD("f_carrier=%lu", cfg.f_carrier);
    WRITE_FIELD("f_step=%lu", cfg.f_step);
    WRITE_FIELD("f_max=%lu", cfg.f_max);
    WRITE_FIELD("f_min=%lu", cfg.f_min);

    WRITE_FIELD("repeater_offset=%u", cfg.repeater_offset);
    WRITE_FIELD("repeater_mode=%d", cfg.repeater_mode);

    WRITE_FIELD("fft.span=%u", cfg.fft.span);
    WRITE_FIELD("fft.bw=%u", cfg.fft.bw);
    WRITE_FIELD("fft.min_db=%d", cfg.fft.min_db);
    WRITE_FIELD("fft.max_db=%d", cfg.fft.max_db);
    WRITE_FIELD("fft.view_mode=%d", cfg.fft.view_mode);
    WRITE_FIELD("fft.smooth_factor=%f", cfg.fft.smooth_factor);
    WRITE_FIELD("fft.enabled=%d", cfg.fft.enabled);
    WRITE_FIELD("fft.refresh_period_ms=%d", cfg.fft.refresh_period_ms);
    WRITE_FIELD("fft.max_slices=%d", cfg.fft.max_slices);
    WRITE_FIELD("fft.maxAmpl=%d", cfg.fft.maxAmpl);
    WRITE_FIELD("fft.view_IQBalance=%d", cfg.fft.view_IQBalance);
    WRITE_FIELD("fft.enable_iq_balance=%d", cfg.fft.enable_iq_balance);
    WRITE_FIELD("fft.conversion_time_us=%d", cfg.fft.conversion_time_us);
    WRITE_FIELD("fft.waterfall_pixels_per_second=%d", cfg.fft.waterfall_pixels_per_second);
    WRITE_FIELD("fft.DCOffset_I=%d", cfg.fft.DCOffset_I);
    WRITE_FIELD("fft.DCOffset_Q=%d", cfg.fft.DCOffset_Q);
    WRITE_FIELD("fft.iq_balance_estimate_period_ms=%d", cfg.fft.iq_balance_estimate_period_ms);
    WRITE_FIELD("fft.removeDC=%d", cfg.fft.removeDC);
    WRITE_FIELD("fft.window=%d", cfg.fft.window);
    WRITE_FIELD("fft.sample_rate=%u", cfg.fft.sample_rate);
    WRITE_FIELD("fft.max_sample_rate=%u", cfg.fft.max_sample_rate);
    WRITE_FIELD("fft.dsp_max_sample_rate=%u", cfg.fft.dsp_max_sample_rate);
    WRITE_FIELD("fft.min_sample_rate=%u", cfg.fft.min_sample_rate);
    WRITE_FIELD("fft.max_decimation_factor=%d", cfg.fft.max_decimation_factor);
    WRITE_FIELD("fft.spectrum_style=%d", cfg.fft.spectrum_style);
    WRITE_FIELD("fft.spectrum_line_color=%d", cfg.fft.spectrum_line_color);
    WRITE_FIELD("fft.spectrum_fill_color=%d", cfg.fft.spectrum_fill_color);

    WRITE_FIELD("dsp.gain=%d", cfg.dsp.gain);
    WRITE_FIELD("dsp.audio_compressor_enabled=%d", cfg.dsp.audio_compressor_enabled);
    WRITE_FIELD("dsp.audio_compressor_threshold=%d", cfg.dsp.audio_compressor_threshold);
    WRITE_FIELD("dsp.test_signal.pulse_duty=%d", cfg.dsp.test_signal.pulse_duty);
    WRITE_FIELD("dsp.test_signal.baseband_frequency=%u", cfg.dsp.test_signal.baseband_frequency);
    WRITE_FIELD("dsp.test_signal.modulation_frequency=%u", cfg.dsp.test_signal.modulation_frequency);

    for (int i = 0; i < FREQ_MEM_SIZE; ++i) {
        WRITE_FIELD("freq_mem[%d].group=%u", i, cfg.freqs[i].group);
        WRITE_FIELD("freq_mem[%d].id=%d", i, cfg.freqs[i].id);
        WRITE_FIELD("freq_mem[%d].freq=%u", i, cfg.freqs[i].freq);
        WRITE_FIELD("freq_mem[%d].mode=%d", i, cfg.freqs[i].mode);
        WRITE_FIELD("freq_mem[%d].name=%s", i, cfg.freqs[i].name);
    }

    WRITE_FIELD("coupler_0db_mv=%d", cfg.coupler_0db_mv);
    WRITE_FIELD("f_correction=%d", cfg.f_correction);
    WRITE_FIELD("if_correction=%d", cfg.if_correction);
    WRITE_FIELD("squelch_auto=%d", cfg.squelch_auto);
    WRITE_FIELD("squelch_level=%f", cfg.squelch_level);
    WRITE_FIELD("agc_enabled=%d", cfg.agc_enabled);
    WRITE_FIELD("enable_quadrature=%d", cfg.enable_quadrature);

    f_close(file);

    unlock_sd_card();
    return true;
}

bool ConfigFile::load(const char *filename, st_config &cfg) {
    FIL *file = &FatFSFileHandle;

    if (!lock_sd_card()) {
        return false;
    }

    if (f_open(file, filename, FA_READ) != FR_OK) {
        return false;
    }

    char buf[256];
    char fmt[50];

#define READ_FIELD(fmt, ...)                                                                                                                                   \
    {                                                                                                                                                          \
        f_gets(buf, sizeof(buf), file);                                                                                                                        \
        if (sscanf(buf, fmt, __VA_ARGS__) != 1)                                                                                                                \
            return false;                                                                                                                                      \
    }

#define READ_U(fmt, ...)                                                                                                                                       \
    {                                                                                                                                                          \
        unsigned int tmp;                                                                                                                                      \
        READ_FIELD(fmt, &tmp)                                                                                                                                  \
        __VA_ARGS__ = tmp;                                                                                                                                     \
    }

#define READ_UL(fmt, ...)                                                                                                                                      \
    {                                                                                                                                                          \
        f_gets(buf, sizeof(buf), file);                                                                                                                        \
        uint64_t val = strtoull(buf + strlen(fmt), nullptr, 10);                                                                                               \
        __VA_ARGS__ = val;                                                                                                                                     \
        \     
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
    }

    READ_FIELD("version=%s", cfg.version);
    READ_FIELD("debug=%d", cfg.debug);
    READ_FIELD("power_ctrl=%d", cfg.power_ctrl);
    READ_FIELD("filter=%d", cfg.filter);
    READ_FIELD("if_filter=%d", cfg.if_filter);
    READ_FIELD("frontend_path=%d", cfg.frontend_path);
    READ_FIELD("modulation=%d", cfg.modulation);
    READ_FIELD("mode=%d", cfg.mode);
    READ_FIELD("band=%d", cfg.band);
    READ_FIELD("hpa_enabled=%d", cfg.hpa_enabled);
    READ_FIELD("max_power_dbm=%d", cfg.max_power_dbm);
    READ_FIELD("lo_injection=%d", cfg.lo_injection);
    READ_FIELD("power_save_period_seconds=%d", cfg.power_save_period_seconds);
    READ_U("f_1st_if=%u", cfg.f_1st_if);
    READ_U("f_if_fm_tx=%u", cfg.f_if_fm_tx);
    READ_U("vfo_ix=%u", cfg.vfo_ix);

    for (int i = 0; i < 2; ++i) {

        READ_UL("vfo[x].freq=", cfg.vfo[i].freq);
        sprintf(fmt, "vfo[%d].step=%%u", i);
        READ_U(fmt, cfg.vfo[i].step);
        sprintf(fmt, "vfo[%d].rit=%%d", i);
        READ_FIELD(fmt, &cfg.vfo[i].rit);
    }

    READ_FIELD("memory_mode=%d", cfg.memory_mode);
    READ_UL("f_carrier=", cfg.f_carrier);
    READ_U("f_step=%u", cfg.f_step);
    READ_UL("f_max=", cfg.f_max);
    READ_UL("f_min=", cfg.f_min);

    READ_U("repeater_offset=%u", cfg.repeater_offset);
    READ_FIELD("repeater_mode=%d", cfg.repeater_mode);

    READ_U("fft.span=%u", cfg.fft.span);
    READ_U("fft.bw=%u", cfg.fft.bw);
    READ_FIELD("fft.min_db=%d", cfg.fft.min_db);
    READ_FIELD("fft.max_db=%d", cfg.fft.max_db);
    READ_FIELD("fft.view_mode=%d", cfg.fft.view_mode);
    READ_FIELD("fft.smooth_factor=%f", cfg.fft.smooth_factor);
    READ_FIELD("fft.enabled=%d", cfg.fft.enabled);
    READ_FIELD("fft.refresh_period_ms=%d", cfg.fft.refresh_period_ms);
    READ_FIELD("fft.max_slices=%d", cfg.fft.max_slices);
    READ_FIELD("fft.maxAmpl=%d", cfg.fft.maxAmpl);
    READ_FIELD("fft.view_IQBalance=%d", cfg.fft.view_IQBalance);
    READ_FIELD("fft.enable_iq_balance=%d", cfg.fft.enable_iq_balance);
    READ_FIELD("fft.conversion_time_us=%d", cfg.fft.conversion_time_us);
    READ_FIELD("fft.waterfall_pixels_per_second=%d", cfg.fft.waterfall_pixels_per_second);
    READ_FIELD("fft.DCOffset_I=%d", cfg.fft.DCOffset_I);
    READ_FIELD("fft.DCOffset_Q=%d", cfg.fft.DCOffset_Q);
    READ_FIELD("fft.iq_balance_estimate_period_ms=%d", cfg.fft.iq_balance_estimate_period_ms);
    READ_FIELD("fft.removeDC=%d", cfg.fft.removeDC);
    READ_FIELD("fft.window=%d", cfg.fft.window);
    READ_U("fft.sample_rate=%u", cfg.fft.sample_rate);
    READ_U("fft.max_sample_rate=%u", cfg.fft.max_sample_rate);
    READ_U("fft.dsp_max_sample_rate=%u", cfg.fft.dsp_max_sample_rate);
    READ_U("fft.min_sample_rate=%u", cfg.fft.min_sample_rate);
    READ_FIELD("fft.max_decimation_factor=%d", cfg.fft.max_decimation_factor);
    READ_FIELD("fft.spectrum_style=%d", cfg.fft.spectrum_style);
    READ_FIELD("fft.spectrum_line_color=%d", cfg.fft.spectrum_line_color);
    READ_FIELD("fft.spectrum_fill_color=%d", cfg.fft.spectrum_fill_color);

    READ_FIELD("dsp.gain=%d", cfg.dsp.gain);
    READ_FIELD("dsp.audio_compressor_enabled=%d", cfg.dsp.audio_compressor_enabled);
    READ_FIELD("dsp.audio_compressor_threshold=%d", cfg.dsp.audio_compressor_threshold);
    READ_FIELD("dsp.test_signal.pulse_duty=%d", cfg.dsp.test_signal.pulse_duty);
    READ_U("dsp.test_signal.baseband_frequency=%u", cfg.dsp.test_signal.baseband_frequency);
    READ_U("dsp.test_signal.modulation_frequency=%u", cfg.dsp.test_signal.modulation_frequency);

    // Read frequency memory array

    for (int i = 0; i < FREQ_MEM_SIZE; ++i) {
        sprintf(fmt, "freq_mem[%d].group=%%u", i);
        READ_U(fmt, cfg.freqs[i].group);
        sprintf(fmt, "freq_mem[%d].id=%%d", i);
        READ_FIELD(fmt, cfg.freqs[i].id);
        sprintf(fmt, "freq_mem[%d].freq=", i);
        READ_UL(fmt, cfg.freqs[i].freq);
        sprintf(fmt, "freq_mem[%d].mode=%%d", i);
        READ_FIELD(fmt, cfg.freqs[i].mode);
        sprintf(fmt, "freq_mem[%d].name=%%s", i);
        READ_FIELD(fmt, cfg.freqs[i].name);
    }

    READ_FIELD("coupler_0db_mv=%d", cfg.coupler_0db_mv);
    READ_FIELD("f_correction=%d", cfg.f_correction);
    READ_FIELD("if_correction=%d", cfg.if_correction);
    READ_FIELD("squelch_auto=%d", cfg.squelch_auto);
    READ_FIELD("squelch_level=%f", cfg.squelch_level);
    READ_FIELD("agc_enabled=%d", cfg.agc_enabled);
    READ_FIELD("enable_quadrature=%d", cfg.enable_quadrature);

    f_close(file);

    unlock_sd_card();
    return true;
}
