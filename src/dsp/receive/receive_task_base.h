//
// Created by Angel Dust on 04/04/2025.
//

#ifndef TRX_FRONTEND_RECEIVE_TASK_BASE_H
#define TRX_FRONTEND_RECEIVE_TASK_BASE_H

#include <memory>

#include "dsp/buffer.hpp"
#include "dsp/decimation/dsp_decimator.h"
#include "dsp/decimation/dsp_iir_decimator.h"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/task.h"
#include "radio.h"
#include "types.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "dsp/decimation/dsp_fir_decimator_float_complex.h"
#include "dsp/modulation/dsp_demodulate.h"
#include "dsp/blocks/dc_block.h"
#include "dsp/audio/audio_compressor.hpp"
#include "memory_allocator.h"

class ReceiveTaskBase : public Task {

  public:
    ReceiveTaskBase(void (*on_success)(), void (*on_error)(DSP_ERROR)) : Task(on_success, on_error) {
        // Allocate memory
        tmp_buff_data = (float32_t *)CCMMemoryAllocator::alloc(samples_per_batch * 8 * sizeof(float32_t));

        bi1_p = tmp_buff_data;
        bq1_p = tmp_buff_data + samples_per_batch;
        bi2_p = tmp_buff_data + samples_per_batch * 2;
        bq2_p = tmp_buff_data + samples_per_batch * 3;
        half_accum_buff_f32_p = tmp_buff_data + samples_per_batch * 4;
        out_accum_buff_f32_p = tmp_buff_data + samples_per_batch * 6;
    };

    ~ReceiveTaskBase() override {
        CCMMemoryAllocator::free(tmp_buff_data);
    }

    static constexpr uint8_t max_decimators = 3; // Max number of cascaded decimators

    void work() override;

    bool start() override;

    void stop() override;

    bool get_baseband_echo() {
        return baseband_echo;
    }

    void set_baseband_echo(bool v) {
        baseband_echo = v;
    }

  protected:
    // Cascaded decimators
    std::unique_ptr<DspDecimator<float32_t>>
        decimators[max_decimators - 1]; // DspFIRDecimatorFloat<FIR_DECIMATOR_1ST_HALFBAND_TAPS, complex_t_f32> decimators[max_decimators - 1];
    // Signal decimators. Last narrowband signal decimators
    // Either complex for assymmetric band-pass filters or real, for symmetric low-pass
    std::unique_ptr<DspDecimator<float32_t>> signal_decimator;

    DCBlock dc_block_i{0.999};
    DCBlock dc_block_q{0.999};

    std::unique_ptr<dsp::demodulator> demodulator;
    std::unique_ptr<dsp::demodulator> get_modulator();

    static constexpr int samples_per_batch =
        DSP_BLOCK; // Note all decimators are configured for a block size of DSP_BLOCK. Don't use bigger blocks or memory will be corrupted
    static constexpr int bytes_per_batch = samples_per_batch * 2 * 2;  // complex int16 samples
    static constexpr int bytes_per_batch_real = samples_per_batch * 2; // complex int16 samples

    float32_t *tmp_buff_data; // [samples_per_batch * 8];

    // 4 temp buffers are used to purposedly avoid overlapping buffers or in-place decimation processing in the hope (is it worth it?) that the compiler
    // is able to fully optimize the loops with instruction reordering
    float32_t *bi1_p;
    float32_t *bq1_p;
    float32_t *bi2_p;
    float32_t *bq2_p;
    float32_t *half_accum_buff_f32_p; // Pre-modulation accumulator buffer
    float32_t *out_accum_buff_f32_p;  // Output accumulator buffer

    bool init_decimators(MODULATION_MODE mod);
    uint8_t n_decimators;
    uint8_t n_pre_decimators;

    uint32_t modulation_bandwidth_hz; // Minimum bandwidth for demodulation (measured as double sideband)
    uint32_t demodulation_sample_rate;

    // Demodulated samples accumulated
    uint32_t demod_samples_count = 0;

    // Pointers to the half-decimation buffer and output buffer
    float32_t *half_accum_p;
    float32_t *out_accum_p;

    virtual MODULATION_MODE get_modulation_mode() const = 0;
    virtual bool init() = 0;
    virtual void process_audio(buffer_t<float32_t> &buff_out_f32) = 0;

    // Skips demodulation step for testing purposes, echoing the baseband signal
    bool baseband_echo = false;

    /* Bandwidth of the output audio stream (IIR LPF config will match this) */
    virtual uint32_t get_audio_bw_hz() const {
        return get_modulation_mode() == WFM ? 15000 : 4000;
    };

    /* Sample rate of the output audio stream */
    virtual uint32_t get_audio_sample_rate() const {
        return get_modulation_mode() == WFM ? 24000 : 12000;
    };

    /* Bandwidth of the modulation */
    virtual uint32_t get_modulation_bw_hz() const {
        return radio::get_bandwidth_hz();
    };
};

#endif // TRX_FRONTEND_RECEIVE_TASK_BASE_H
