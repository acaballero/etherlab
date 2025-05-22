//
// Created by Angel Dust on 18/05/2025.
//

#ifndef __APRS_UI_H__
#define __APRS_UI_H__

#include "Display_afb.h"
#include "aprs_packet.h"
#include <functional>
#include <stdio.h>
#include <string>
#include <sys/_stdint.h>
#include "dsp/aprs/aprs_task.hpp"
#include "ui/view.h"
#include "ui/button_widget.h"
#include "ui/console_widget.h"

namespace dsp_ui {

#define APRS_DEBUG 1

using namespace dsp;

struct APRSSource {

    static constexpr uint64_t invalid_key = 0xffffffffffffffff;

    uint16_t hits{0};
    uint32_t age{0};
    uint64_t source{0};
    std::string source_formatted{"        "};
    std::string time_string{""};
    std::string info_string{""};

    aprs_pos pos{0, 0, 0, 0};
    bool has_position = false;
    APRSSource(uint64_t src) {
        source = src;
    }
};

class APRSTableWidget : public Widget {
  public:
    APRSTableWidget(Rect parent_rect, int max_sources) : Widget(parent_rect, &lcd) {
        this->max_sources = max_sources;
        init();
    }

    void paint_callback() override;
    uint8_t on_packet(APRSPacket *packet);
    bool on_touch(const st_inputEvent) override;
    void set_max_lines(int n) {
        max_sources = n;
    }

    std::function<void(APRSSource)> on_select;

  private:
    std::vector<APRSSource> sources;

    uint8_t max_sources = 5;

    bool send_updates{false};
    void before_paint() override;
    void init();
};

class APRSView : public View {
  public:
    APRSView(Rect parent_rect) : View(parent_rect) {
        init();
    }

    void on_packet(APRSPacket *packet);

  private:
    static constexpr int title_height = 20;
    static constexpr int panel_sep = 4;
    static constexpr int max_sources = 6;
    void on_source_selected(APRSSource &source);
    bool reset_console = false;

    Label title_widget{{0, 0, DISPLAY_X_PIXELS, title_height}, C565_WHITE, C565_GREY_DARKER, ButtonStyle::BUTTON_STYLE_FLAT};

    APRSTableWidget table_view{{0, title_height + panel_sep, DISPLAY_X_PIXELS / 2 - panel_sep / 2, 90 + title_height}, max_sources};
    ConsoleWidget console{{DISPLAY_X_PIXELS / 2 + panel_sep / 2, title_height + panel_sep, DISPLAY_X_PIXELS / 2, 90 + title_height}, &lcd};

    APRSTask receive_task{};
    void before_paint() override;
    void init();
};

} // namespace dsp_ui

#endif
