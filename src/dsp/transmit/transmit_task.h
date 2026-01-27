//
// Created by Angel Dust on 21/01/2026.
//

#ifndef TRX_TRANSMIT_TASK_H
#define TRX_TRANSMIT_TASK_H

#include <memory>

#include "dsp/buffer.hpp"
#include "dsp/decimation/dsp_decimator.h"
#include "dsp/decimation/dsp_iir_decimator.h"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/interpolation/dsp_fir_interpolator_float.h"
#include "dsp/modulation/dsp_modulate.h"
#include "dsp/task.h"
#include "main_board.h"
#include "radio.h"
#include "types.h"
#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "memory_allocator.h"

class TransmitTask : public Task {

  public:
    TransmitTask(void (*on_success)(), void (*on_error)(DSP_ERROR)) : Task(on_success, on_error) {
        // Allocate memory
        tmp_buff_data = (float32_t *)CCMMemoryAllocator::alloc(samples_per_batch * 2 * sizeof(float32_t));

        bi1_p = tmp_buff_data;
        bq1_p = tmp_buff_data + samples_per_batch;
        out_accum_p = bq1_p;
    };

    ~TransmitTask() override {
        CCMMemoryAllocator::free(tmp_buff_data);
    }

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
    // std::unique_ptr<dsp::demodulator> demodulator;
    // std::unique_ptr<dsp::demodulator> get_modulator();

    static constexpr int samples_per_batch =
        DSP_BLOCK; // Note all decimators are configured for a block size of DSP_BLOCK. Don't use bigger blocks or memory will be corrupted
    static constexpr int bytes_per_batch = samples_per_batch * 2 * 2;  // complex int16 samples
    static constexpr int bytes_per_batch_real = samples_per_batch * 2; // single channel int16 samples

    float32_t *tmp_buff_data; // [samples_per_batch * 2];

    // 4 temp buffers are used to purposedly avoid overlapping buffers or in-place decimation processing in the hope (is it worth it?) that the compiler
    // is able to fully optimize the loops with instruction reordering
    float32_t *bi1_p;
    float32_t *bq1_p;

    float32_t *out_accum_p;

    std::unique_ptr<DspFIRDecimatorFloat<FIR_DECIMATOR_SIGNAL_TAPS>> decimator;
    std::unique_ptr<DspFIRInterpolatorFloat<FIR_DECIMATOR_SIGNAL_TAPS>> interpolator;
    std::unique_ptr<dsp::modulator> modulator;

    std::unique_ptr<dsp::modulator> get_modulator();

    bool init_resampler(MODULATION_MODE mod);

    uint32_t modulation_bandwidth_hz; // Minimum bandwidth for demodulation (measured as double sideband)
    uint32_t demodulation_sample_rate;

    virtual MODULATION_MODE get_modulation_mode() const {
        return main_board::get_modulation_mode();
    };
    virtual bool init() {
        return true;
    };

    // TODO: If this class is ever extened (as ReplayTaskBase is), use this for specific pre-modulation audio processing
    // virtual void process_audio(buffer_t<float32_t> &){};

    // Skips demodulation step for testing purposes, echoing the baseband signal
    bool baseband_echo = false;

    /* Bandwidth of the output audio stream (IIR LPF config will match this) */
    virtual uint32_t get_audio_bw_hz() const {
        return DSP_AUDIO_SAMPLE_RATE >> 2;
    };

    /* Sample rate of the output audio stream. Must be a factor/divisor of 48000  */
    virtual uint32_t get_audio_sample_rate() const {
        return DSP_AUDIO_SAMPLE_RATE;
    };

    /* Bandwidth of the modulation */
    virtual uint32_t get_modulation_bw_hz() const {
        return radio::get_bandwidth_hz();
    };

    DspIIRDecimator<2> high_pass_filter;
};

#endif // TRX_TRANSMIT_TASK_H
