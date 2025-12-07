//
// Created by Angel Dust on 18/05/2025.
//

#ifndef __APRS_TABLE_WIDGET_H__
#define __APRS_TABLE_WIDGET_H__

#include "Display_afb.h"
#include "aprs_packet.h"
#include <functional>
#include <stdio.h>
#include "dsp/aprs/aprs_rx_task.h"
#include "menuBase.h"
#include "ring_buffer.hpp"
#include "ui/ui_types.h"
#include "ui/view.h"
#include "ui/console_widget.h"

namespace dsp_ui {

struct APRSSource {

    static constexpr uint64_t invalid_key = 0xffffffffffffffff;
    static constexpr uint8_t source_length = 15;
    static constexpr uint8_t time_length = 8;

    int id{-1};
    uint16_t hits{0};
    uint32_t age{0};
    uint64_t source{0};
    char source_formatted[source_length + 1];
    char time_string[time_length + 1];

    dsp::aprs_pos pos{0, 0, 0, 0};
    bool has_position = false;
};

class APRSTableWidget : public Widget {
  public:
    APRSTableWidget(Rect parent_rect, int max_rows) : Widget(parent_rect, &lcd) {
        set_name("atbl");
        set_focusable(true);
        set_max_rows(max_rows);
        init();
    }

    bool paint_callback() override;
    int on_packet(dsp::APRSPacket *packet);
    bool on_touch(const st_inputEvent) override;
    bool on_input(const st_inputEvent e) override;
    void on_focus() override {
        LOG("APRS table focused\n");
    };
    void on_blur() override {
        LOG("APRS table blurred\n");
    };
    void set_max_rows(int n) {
        assert(n <= MAX_ROWS);
        max_sources = n;
    }

    std::function<void(APRSSource &)> on_select = nullptr;

  private:
    static constexpr int MAX_ROWS = 8;

    RingBuffer<APRSSource, MAX_ROWS> sources;

    uint8_t max_sources = 1;
    int selected_id{-1};
    int line_height{0};
    int title_height{0};
    int line_spacing{4};

    bool send_updates{false};
    void before_paint() override;
    void init();
    void select(int ix);
    int find_free_id();
};

} // namespace dsp_ui

#endif
