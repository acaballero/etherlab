//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_REPLAY_TASK_H
#define TRX_FRONTEND_REPLAY_TASK_H

#include <memory>
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/interpolation/dsp_fir_interpolator_float.h"
#include "dsp/interpolation/dsp_fir_interpolator_q15.h"
#include "dsp/replay/dsp_replay_processor.h"
#include "dsp/task.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"

class ReplayTask : public Task {

  public:
    ReplayTask() : Task() {
        // Allocate memory
        f32_in = (float32_t *)CCMMemoryAllocator::alloc(samples_per_batch * 2 * sizeof(float32_t));
        f32_out = (float32_t *)CCMMemoryAllocator::alloc(samples_per_batch * 2 * sizeof(float32_t));
    };

    ~ReplayTask() override {
        CCMMemoryAllocator::free(f32_in);
        CCMMemoryAllocator::free(f32_out);
    }

    void work() override;

    bool start_impl() override;

    void stop() override;

    void setFile(File *);

    bool getLoop() const;

    void setLoop(bool loop);

  protected:
    static constexpr int samples_per_batch =
        DSP_BLOCK; // Note  decimators are configured for a block size of DSP_BLOCK. Don't use bigger blocks or memory will be corrupted
    static constexpr int bytes_per_batch = samples_per_batch * 2 * 2; // complex int16 samples

    std::unique_ptr<DspProcessor> create_processor() override {
        return std::make_unique<DspReplayProcessor>();
    }

    float32_t *f32_in;
    float32_t *f32_out;

    uint32_t consumed_in_bytes = 0;

    std::unique_ptr<DspFIRInterpolatorFloat<FIR_INTERPOLATOR_BASEBAND_TAPS>> interpolator;

  private:
    bool loop{0};
    File *m_file;

    void fetch();
    void produce();
};

#endif // TRX_FRONTEND_REPLAY_TASK_H
