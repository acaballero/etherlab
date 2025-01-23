//
// Created by Angel Dust on 08/06/2024.
//

#ifndef TRX_FRONTEND_SMETER_WIDGET_H
#define TRX_FRONTEND_SMETER_WIDGET_H

#include "widget.h"
#include "types.h"

#define MINOR_TICK_GAP 1
#define S_LEVELS 9
#define DB_LEVELS 6
#define MAX_S_LEVEL (S_LEVELS + DB_LEVELS)
#define S_METER_LINE_HEIGHT 10
#define MAJOR_TICK_GAP 1

struct st_meter_widget_state {
    float s_level = 0;
    float peak_s_level = 0;

    bool operator==(const st_meter_widget_state &st) const {
        return s_level == st.s_level && peak_s_level == st.peak_s_level; // or another approach as above
    }
};

class SMeterWidget : public Widget {
  public:
    using Widget::Widget;

    void paint_callback() override;

    bool on_input(const st_inputEvent event) override;

  protected:
    static constexpr int margin_top = 8;
    void before_paint() override;

    float get_s_level(float current, float smooth_factor);

    st_meter_widget_state get_state();

    st_meter_widget_state state;
};

#endif // TRX_FRONTEND_SMETER_WIDGET_H
