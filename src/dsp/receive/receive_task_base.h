//
// Created by Angel Dust on 04/04/2025.
//

#ifndef TRX_FRONTEND_RECEIVE_TASK_BASE_H
#define TRX_FRONTEND_RECEIVE_TASK_BASE_H

#include <memory>
#include <sys/_stdint.h>
#include "dsp/buffer.hpp"
#include "dsp/decimation/dsp_iir_decimator.h"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/task.h"
#include "types.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "dsp/decimation/dsp_fir_decimator_float_complex.h"
#include "dsp/modulation/dsp_demodulate.h"
#include "dsp/blocks/dc_block.h"
#include "dsp/audio/audio_compressor.hpp"

class ReceiveTaskBase : public Task {

  public:
    static constexpr uint8_t max_decimators = 3; // Max number of cascaded decimators

    void work() override;

    bool start() override;

    void stop() override;

  protected:
    // Cascaded decimators
    DspFIRDecimatorFloat<FIR_DECIMATOR_1ST_HALFBAND_TAPS, complex_t_f32> decimators[max_decimators - 1];
    // Signal decimators. Last narrowband signal decimators
    // Either complex for assymmetric band-pass filters or real, for symmetric low-pass
    std::unique_ptr<IDspDecimatorFloat> signal_decimator;

    DCBlock dc_block_i{0.999};
    DCBlock dc_block_q{0.999};

    std::unique_ptr<dsp::demodulator> demodulator;
    std::unique_ptr<dsp::demodulator> get_modulator();

    float32_t tmp_buff_data[DSP_BLOCK * 4 * 2];

    // 4 temp buffers are used to purposedly avoid overlapping buffers or in-place decimation processing in the hope (is it worth it?) that the compiler
    // is able to fully optimize the loops with instruction reordering
    float32_t *bi1_p = tmp_buff_data;
    float32_t *bq1_p = tmp_buff_data + DSP_BLOCK * 2;
    float32_t *bi2_p = tmp_buff_data + DSP_BLOCK * 4;
    float32_t *bq2_p = tmp_buff_data + DSP_BLOCK * 6;

    buffer_t<float32_t> tmp_buff{bi1_p, DSP_BLOCK};

    bool init_decimators(MODULATION_MODE mod);
    uint8_t n_decimators;

    virtual MODULATION_MODE get_modulation_mode() = 0;
    virtual bool init() = 0;
    virtual void process_audio(buffer_t<float32_t> &buff_out_f32) = 0;
    virtual uint32_t get_audio_bw_hz() const {
        return 12000;
    };
};

#endif // TRX_FRONTEND_RECEIVE_TASK_BASE_H
