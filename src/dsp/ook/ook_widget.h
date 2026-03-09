//
// OOK Widget - Draws OOK sequence timing diagram with proportional mark/space
//

#ifndef TRX_FRONTEND_OOK_WIDGET_H
#define TRX_FRONTEND_OOK_WIDGET_H

#include "ui/widget.h"
#include "dsp/dsp_common.h"
#include <vector>
#include <cstdint>

class OOKWidget : public Widget {
  public:
    using Widget::Widget;

    bool paint_callback() override;

    void set_sequence(const std::vector<uint8_t> *seq) {
        sequence = seq;
    }

    void set_task_status(st_dsp_params *status) {
        task_status = status;
    }

    /**
     * Update timing parameters for proportional waveform drawing.
     * Call this whenever mark/space durations change in the UI.
     */
    void set_timing(uint32_t mark_us, uint32_t space_us) {
        mark_duration_us = mark_us;
        space_duration_us = space_us;
    }

    void set_current_bit(size_t bit) {
        current_bit = bit;
    }

    void set_progress(uint16_t current_rep, uint16_t total_reps) {
        this->current_rep = current_rep;
        this->total_reps = total_reps;
    }

  protected:
    void before_paint() override;

    const std::vector<uint8_t> *sequence{nullptr};
    st_dsp_params *task_status{nullptr};
    size_t current_bit{0};
    uint16_t current_rep{0};
    uint16_t total_reps{1};
    uint32_t mark_duration_us{500};
    uint32_t space_duration_us{500};

    void draw_waveform();
    void draw_progress_bar();
};

#endif // TRX_FRONTEND_OOK_WIDGET_H
