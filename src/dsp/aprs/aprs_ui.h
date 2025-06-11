//
// Created by Angel Dust on 18/05/2025.
//

#ifndef __APRS_UI_H__
#define __APRS_UI_H__

#include "Display_afb.h"
#include "Signal.h"
#include "aprs_packet.h"
#include <cstddef>
#include <functional>
#include <stdio.h>
#include <string>
#include "dsp/aprs/aprs_rx_task.h"
#include "dsp/afsk/afsk_tx_task.h"
#include "dsp/dsp.h"
#include "main_board.h"
#include "menuBase.h"
#include "os/periodic_task.h"
#include "ring_buffer.hpp"
#include "types.h"
#include "ui/ui_types.h"
#include "ui/view.h"
#include "ui/button_widget.h"
#include "ui/console_widget.h"
#include "os/task_manager.h"

namespace dsp_ui {

#define APRS_DEBUG 1
#define EU_APRS_FREQ 144800000

using namespace dsp;

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

    aprs_pos pos{0, 0, 0, 0};
    bool has_position = false;
};

class APRSTableWidget : public Widget {
  public:
    APRSTableWidget(Rect parent_rect, int max_rows) : Widget(parent_rect, &lcd) {
        set_max_rows(max_rows);
        init();
    }

    void paint_callback() override;
    int on_packet(APRSPacket *packet);
    bool on_touch(const st_inputEvent) override;
    void set_max_rows(int n) {
        assert(n <= MAX_ROWS);
        max_sources = n;
    }

    std::function<void(APRSSource &)> on_select = nullptr;

  private:
    static constexpr int MAX_ROWS = 8;

    RingBuffer<APRSSource, MAX_ROWS> sources;

    uint8_t max_sources = 1;

    bool send_updates{false};
    void before_paint() override;
    void init();
    int find_free_id();
};

class APRSView : public View {
  public:
    APRSView(Rect parent_rect) : View(parent_rect) {
        init();
    }

    void on_packet(APRSPacket *packet);
    void send_packet(std::string info);

  private:
    static constexpr int title_height = 20;
    static constexpr int panel_sep = 4;
    static constexpr int max_sources = 7;
    static constexpr int table_width = DISPLAY_X_PIXELS / 2 - 60;
    static constexpr int console_width = DISPLAY_X_PIXELS - table_width;

    void on_source_selected(APRSSource &source);
    bool reset_console = false;

    Label title_widget{{0, 0, DISPLAY_X_PIXELS, title_height}, C565_WHITE, C565_GREY_DARKER, ButtonStyle::BUTTON_STYLE_FLAT};

    APRSTableWidget table_view{{0, title_height + panel_sep, table_width - panel_sep / 2, 90 + title_height}, max_sources};
    ConsoleWidget console{{table_width + panel_sep / 2, title_height + panel_sep, console_width - panel_sep, 90 + title_height}, &lcd};

    Menu::menu_action_st menu_actions[5] = {{"Pause",
                                             [this]() {
                                                 if (!paused) {
                                                     stop();
                                                 } else {
                                                     resume();
                                                 }
                                             }},
                                            {"Send",
                                             [this]() {
                                                 send_packet("Beacon");
                                             }},
                                            {"Beacon",
                                             [this]() {
                                                 toggle_beacon();
                                             },
                                             C565_TEXT_FG, C565_BG_DISABLED},
                                            {"Text",
                                             [this]() {
                                                 settings();
                                             }},
                                            {"Exit", [this]() {
                                                 exit();
                                             }}};

    Menu::menu_actions_st actions = {menu_actions, sizeof(menu_actions) / sizeof(Menu::menu_action_st)};

    SignalToken aprs_signal_token;
    APRSTask aprs_task{dspSuccess, dspError};
    AFSKTXTask aprs_tx_task{dspSuccess, dspError};
    APRSSource current_source;

    int beacon_task_id{0};

    MODE previous_mode;

    bool on_input(const st_inputEvent event) override;
    void before_paint() override;
    void init();
    void stop();
    void resume();
    void exit();
    void settings();
    void start_rx();
    void toggle_beacon();
    bool paused{false};
};

} // namespace dsp_ui

#endif
