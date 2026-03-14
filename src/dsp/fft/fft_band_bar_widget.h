//
// Created by Angel Dust on 14/03/2026.
//

#ifndef TRX_FFT_BAND_BAR_WIDGET_H
#define TRX_FFT_BAND_BAR_WIDGET_H

#include "../../types.h"
#include "../../ui/widget.h"
#include "input/inputEvent.h"
#include <vector>

class FFTBandBarWidget : public Widget {
  public:
    FFTBandBarWidget(const Rect &parentRect, Display *display);

    bool paint_callback() override;

  protected:
    void before_paint() override;

    bool on_touch(const st_inputEvent) override;

    struct band_segment {
        uint64_t start_hz{0};
        uint64_t end_hz{0};
        char name[FREQ_MEM_NAME_SIZE + 1] = "";
    };

    void fetch_bands_in_range();
    void build_segments();

    std::vector<st_freq_mem> band_markers;
    std::vector<band_segment> segments;

    bool refresh_all{true};
    bool db_error{false};
};

#endif // TRX_FFT_BAND_BAR_WIDGET_H
