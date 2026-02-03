//
// Created by Angel Dust on 04/04/2025.
//

#include "receive_task_base.h"
#include "arm_math.h"
#include "dsp/blocks/dc_block.h"
#include "dsp/buffer.hpp"
#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "dsp/decimation/dsp_fir_decimator_float_complex.h"
#include "dsp/dsp_buffers.h"
#include "dsp/fft/fft.h"
#include "dsp/fft/fft_params.h"
#include "dsp/fft/fft_types.h"
#include "dsp/fir_filter.h"
#include "dsp/dsp_common.h"
#include "dsp/modulation/dsp_demodulate.h"
#include "main_board.h"
#include "radio.h"
#include "status.h"
#include "hw/stm32f4xx/timers.h"
#include "config.h"
#include "FIFO.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "tinyusb/usb_audio_dsp_bridge.h"
#include "types.h"
#include "ui/view.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include <cstddef>
#include <cstring>
#include <memory>

#include "printf.h"
#include "utils.hpp"

/* Should be defined in the HW abstraction layer */
extern TIM_HandleTypeDef TASKS_TIMER_HANDLE;

HOT_FUNCTION
void ReceiveTaskBase::work() {
    if (info.status == DSP_STATUS_RUNNING) {

        char *in_start;
        char *in_p;
        char *out_p;

        uint32_t free = output_stream.free(&out_p);
        uint32_t av = input_stream.available(&in_p);
        uint32_t output_samples = 0;

        if (av >= DSP_FIFO_BLOCK_BYTES && free >= (DSP_FIFO_BLOCK_BYTES / info.decimation_factor)) {

            info.processed_blocks++;
            av = DSP_FIFO_BLOCK_BYTES;
            in_start = in_p;

            // Process blocks (DSP_BLOCK items each)
            while (av >= bytes_per_batch) {

                int dec_phase = 0; // Decimation phase
                int curr_dec_factor = 1;
                int factor = 0;
                uint16_t block_size_in = samples_per_batch;

                dsp::s16_to_f32((const adc_type *)in_p, bi2_p, block_size_in << 1);

                // buffer_t<float32_t> bb = {(float32_t *)bi2_p, DSP_BLOCK * 2};
                // dc_block_i.filter(bb, 2, 0);
                // dc_block_q.filter(bb, 2, 1);

                // dsp::rotate_fs4_f32((const float32_t *)bi2_p, (float32_t *)bi2_p, DSP_BLOCK);

                dsp::unzip_f32((const float32_t *)bi2_p, bi1_p, bq1_p, block_size_in);

                // Pre-demodulation decimation: decimate as much as the demoulation bandwidth allows
                while (dec_phase < n_pre_decimators) {

                    bool last_of_phase = dec_phase == n_pre_decimators - 1;
                    bool last = dec_phase == n_decimators - 1;
                    float32_t *dec_out_i = last_of_phase ? half_accum_p : bi2_p;
                    float32_t *dec_out_q = dec_out_i + samples_per_batch;

                    if (last) {
                        // Output decimator. This is the final nawrrowband decimator
                        signal_decimator->decimate(bi1_p, bq1_p, dec_out_i, dec_out_q, block_size_in);
                        factor = signal_decimator->get_factor();
                    } else {
                        // Half-band decimators
                        decimators[dec_phase]->decimate(bi1_p, bq1_p, dec_out_i, dec_out_q, block_size_in);

                        factor = decimators[dec_phase]->get_factor();
                    }

                    SWAP_PTR(bi1_p, bi2_p);
                    SWAP_PTR(bq1_p, bq2_p);
                    dec_phase++;
                    block_size_in /= factor;
                    curr_dec_factor *= factor;
                }

                half_accum_p += block_size_in;
                demod_samples_count += block_size_in;

                if (demod_samples_count == samples_per_batch) {

                    half_accum_p = half_accum_buff_f32_p;

                    // DC block
                    // buffer_t<float32_t> bb = {(float32_t *)half_accum_buff_f32_p, (size_t)samples_per_batch << 1};
                    // dc_block_i.filter(bb, 2, 0);
                    // dc_block_q.filter(bb, 2, 1);

                    demodulator->work_real(half_accum_buff_f32_p, half_accum_buff_f32_p + samples_per_batch, bi1_p, samples_per_batch);

                    if (dec_phase < n_decimators) {

                        // Post-demoulation decimation
                        // Note these are decimated as real-typed buffers
                        // (vs the complex-typed done before demodulation)

                        block_size_in = samples_per_batch;

                        while (dec_phase < n_decimators) {

                            buffer_t<float32_t> b1 = {bi1_p, block_size_in, REAL};

                            if (dec_phase < n_decimators - 1) {

                                buffer_t<float32_t> b2 = {bi2_p, block_size_in, REAL};
                                decimators[dec_phase]->decimate(b1, b2);
                                block_size_in /= decimators[dec_phase]->get_factor();

                            } else {
                                buffer_t<float32_t> b2 = {out_accum_p, block_size_in, 0, REAL};
                                // Output decimator. This is the final nawrrowband decimator
                                signal_decimator->decimate(b1, b2);
                            }

                            SWAP_PTR(bi1_p, bi2_p);
                            dec_phase++;
                        }
                    }

                    uint16_t block_size_out = samples_per_batch / (info.decimation_factor / curr_dec_factor); // account for the already decimated factor
                    out_accum_p += block_size_out;
                    output_samples += block_size_out;

                    if (output_samples == samples_per_batch) {

                        out_accum_p = n_pre_decimators == n_decimators ? bi1_p : out_accum_buff_f32_p;

                        if (!baseband_echo) {
                            buffer_t<float32_t> buff_out_f32 = {out_accum_p, (size_t)samples_per_batch, info.sample_rate, REAL};
                            process_audio(buff_out_f32);
                        }

                        dsp::f32_to_s16((const float32_t *)out_accum_p, (adc_type *)out_p, samples_per_batch);

                        out_p += bytes_per_batch_real;
                        output_samples = 0;
                        output_stream.feed(bytes_per_batch_real);
                    }

                    demod_samples_count = 0;
                }

                in_p += bytes_per_batch;
                av -= bytes_per_batch;
            }

            uint32_t processed = DSP_FIFO_BLOCK_BYTES - av;

            input_stream.consume(processed, &in_start);

        } else {
            if (free < (DSP_FIFO_BLOCK_BYTES / info.decimation_factor)) {
                info.fifo_overruns++;
            }
            if (av < DSP_FIFO_BLOCK_BYTES) {
                info.fifo_underruns++;
            }
        }
    }
}

bool ReceiveTaskBase::init_decimators(MODULATION_MODE mod) {
    // First staes are half-band filters (https://en.wikipedia.org/wiki/Half-band_filter)
    uint8_t factor;
    uint8_t dec = info.decimation_factor;

    uint32_t stage_sr = config.fft.sample_rate;
    uint32_t next_stage_bandwidth = stage_sr;
    n_decimators = 0;
    n_pre_decimators = 0;
    demodulation_sample_rate = 0;

    bool ret;
    while (dec > 1) {

        LOG("Remaining dec factor: %d\n", dec);

        if (n_decimators == max_decimators - 1 || dec == 2) { // || (stage_fs / factor) > (status.bandwidth / 2)) {

            // Final narrowband signal decimator
            factor = dec;

            next_stage_bandwidth = info.bandwidth;

            switch (mod) {
                case SSB_USB:
                case SSB_LSB:

                    signal_decimator = std::make_unique<DspFIRDecimatorFloatComplex<FIR_DECIMATOR_SIGNAL_TAPS>>();
                    ret = signal_decimator->config(stage_sr, next_stage_bandwidth, factor);
                    break;
                case CW:
                    signal_decimator = std::make_unique<DspFIRDecimatorFloatComplex<FIR_DECIMATOR_SIGNAL_TAPS>>();
                    // TODO: Select pitch (offset center freq)
                    ret = signal_decimator->config(stage_sr, next_stage_bandwidth, factor, max2(CW_PITCH_HZ - (next_stage_bandwidth / 2), 0));
                    break;
                default:
                    signal_decimator = std::make_unique<DspFIRDecimatorFloat<FIR_DECIMATOR_SIGNAL_TAPS>>();
                    ret = signal_decimator->config(stage_sr, next_stage_bandwidth, factor); // Here the bandwidth is halved for double sideband modulations

                    break;
            }

            LOG("ReceiveTask::init_decimators: Signal decimator ==> \n");

        } else {

            if (stage_sr >= modulation_bandwidth_hz * 4 && modulation_bandwidth_hz > info.bandwidth) {
                // When the demodulation bandwidth is higher than the target bandwidth and the current sample rate can be decimated
                // before demodulation, we find the highest decimation factor we can apply before demodulating
                factor = 1;
                uint32_t next_stage_fs = stage_sr;
                while (next_stage_fs >= modulation_bandwidth_hz * 4) {
                    factor <<= 1;
                    next_stage_fs >>= 1;
                }

                // assign the output sample rate of this decimator as the demodulation sample rate so we configure the demodulator accordingly
                demodulation_sample_rate = next_stage_fs;
                next_stage_bandwidth = modulation_bandwidth_hz; // Filter just the signal bandwidth to demodulate
                n_pre_decimators++;
                decimators[n_decimators] = std::make_unique<DspFIRDecimatorFloat<FIR_DECIMATOR_1ST_HALFBAND_TAPS>>();
                LOG("ReceiveTask::init_decimators: Pre-demodulation decimator => \n");

            } else {

                // Prefer larger factors instead

                factor = dec >= 16 ? 8 : (dec >= 8 ? 4 : (dec >= 4 ? 2 : 2));
                next_stage_bandwidth = (stage_sr / (factor * 3)); // Low pass fiter to 1/3 sample rate
                decimators[n_decimators] = std::make_unique<DspFIRDecimatorFloat<FIR_DECIMATOR_1ST_HALFBAND_TAPS>>();

                LOG("ReceiveTask::init_decimators: Decimation step %d => \n", n_decimators);
            }

            auto volatile dec = (DspFIRDecimatorFloat<FIR_DECIMATOR_1ST_HALFBAND_TAPS> *)(decimators[n_decimators].get());
            ret = decimators[n_decimators]->config(stage_sr, next_stage_bandwidth, factor);
            auto f = dec->get_factor();
            LOG("Decimation step %d configured | factor: %u => \n", n_decimators, f);
        }

        if (!ret) {
            LOG("Error configuring decimator for mod:%d, fs:%d, bw:%d, dec:%d\n", mod, stage_sr, next_stage_bandwidth, factor);
            return false;
        }

        LOG("Rate %d / %d -> %d (filter: %d)\n", stage_sr, factor, stage_sr / factor, next_stage_bandwidth);

        n_decimators++;
        dec /= factor;
        stage_sr = stage_sr / factor;
    }

    LOG("ReceiveTask::init_decimators -> n_decimators: %d\n", n_decimators);
    if (demodulation_sample_rate == 0) { // All decimation is done before demodulation
        demodulation_sample_rate = info.sample_rate;
        n_pre_decimators = n_decimators;
    }

    return true;
}

std::unique_ptr<dsp::demodulator> ReceiveTaskBase::get_modulator() {

    std::unique_ptr<dsp::demodulator> demod;

    auto mode = get_modulation_mode();

    if (get_baseband_echo() || mode == NONE) {
        return std::make_unique<dsp::ssb_demodulator>();
    } else {
        switch (mode) {
            case AM:
                return std::make_unique<dsp::am_demodulator>();
            case CW:
            case SSB_LSB:
            case SSB_USB:
                return std::make_unique<dsp::ssb_demodulator>();
            case FM:
                demod = std::make_unique<dsp::fm_demodulator>();
                ((dsp::fm_demodulator *)demod.get())->configure(demodulation_sample_rate, config.dsp.fm_max_deviation);
                return demod;
            case WFM:
                demod = std::make_unique<dsp::fm_demodulator>();
                ((dsp::fm_demodulator *)demod.get())->configure(demodulation_sample_rate, config.dsp.wideband_fm_max_deviation);
                return demod;
            default:
                return std::make_unique<dsp::ssb_demodulator>();
        }
    }
}

bool ReceiveTaskBase::start_impl() {

    LOG("___ [START] Receive task ___\n");
    auto current_mute = main_board::get_mute();
    main_board::set_mute(GPIO_PIN_SET);

    // Stop task processing timer (in case this is a restart)
    HAL_TIM_Base_Stop_IT(&TASKS_TIMER_HANDLE);

    dsp_set_real_time(true);

    info.sample_rate = fft::fft_params.sample_freq;

    uint32_t dac_sample_rate = get_audio_sample_rate();

    modulation_bandwidth_hz = get_modulation_bw_hz();

    MODULATION_MODE mod = get_modulation_mode();

    set_baseband_echo(dsp::dsp_config.baseband_echo);

    main_board::set_modulation_mode(mod, false);

    if (mod != SSB_USB && mod != SSB_LSB && mod != CW) {
        modulation_bandwidth_hz /= 2; // Halved for double sideband modulations since modulation bandwidth represents the double sideband bandwidth
    }

    // Calculate decimation ratio to get as closest as possible to our target audio bandwidth
    // (while using decimation factors of 2^n)
    int dec_factor = 1;
    while (info.sample_rate > dac_sample_rate * 2 && dec_factor < MAX_DSP_DECIMATION_FACTOR) {
        dec_factor <<= 1;
        info.sample_rate /= 2;
    }

    if (modulation_bandwidth_hz > info.sample_rate) {
        // This may happen for example with broadband FW where the modulation bandwidth is higher that the demodulated audio bandwidth
        // In that case, we limit the bandwidth to a third of the sample rate.
        // However, this should't be allowed
        info.bandwidth = info.sample_rate / 3;
    } else {
        info.bandwidth = modulation_bandwidth_hz;
    }

    info.direction = DSP_DIRECTION_INOUT;

    info.decimation_factor = dec_factor;
    info.bits_per_sample = sizeof(adc_type) * 8;
    info.n_channels = 2;
    info.block_size_bytes = DSP_BLOCK * sizeof(complex_t);
    info.decimated_block_size = DSP_BLOCK / dec_factor;
    info.decimated_block_size_bytes = info.decimated_block_size * sizeof(complex_t);

    LOG("Bandwidth: %d | Sample rate: %d | Modulation bandwidth: %d\n", info.bandwidth, info.sample_rate, modulation_bandwidth_hz);

    bool ret = init_decimators(mod);

    if (!ret) {
        main_board::set_mute(current_mute);
        halt(DSP_ERR);
        return false;
    }

    demodulator = get_modulator();

    demod_samples_count = 0;
    half_accum_p = half_accum_buff_f32_p;
    out_accum_p = out_accum_buff_f32_p;

    ret = init();

    ret = ret && radio_config({.direction = RF_DIRECTION_RX,
                               .sample_freq = info.sample_rate,
                               .freq = 0,
                               .mode = DSP}); // Radio mode is DSP so the signal is routed to the audio amp

    if (!ret) {
        main_board::set_mute(current_mute);
        halt(DSP_ERR);
        return false;
    }

    main_board::set_mute(current_mute);

    // Start task processing timer
    // TODO: This should be done by the caller of this method and be generic for all tasks
    HAL_TIM_Base_Start_IT(&TASKS_TIMER_HANDLE);

    // Se the fifo processing frequency
    update_timer(TASKS_TIMER_TYPEDEF, 40, TASKS_TIMER_TYPEDEF_CLOCK_HZ / 100000);
    info.status = DSP_STATUS_RUNNING;
    return true;
}

void ReceiveTaskBase::stop() {

    if (info.status != DSP_STATUS_STOPPED) {
        // Stop task processing timer
        HAL_TIM_Base_Stop_IT(&TASKS_TIMER_HANDLE);

        // Free decimators memory (wish this wouldn't be necessary but there must be room for other allocations while stopped)
        for (auto &dec : decimators) {
            dec.reset();
        }
        signal_decimator.reset();

        info.status = DSP_STATUS_STOPPED;

        dsp_set_real_time(false);

        Task::stop(); // Let the base class finish

        radio_config({RF_DIRECTION_RX, 0});
    }
}
