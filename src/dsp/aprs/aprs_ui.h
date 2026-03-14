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
#include <memory>
#include <stdio.h>
#include <string>

#include "arm_math.h"
#include "dsp/aprs/aprs_rx_task.h"
#include "dsp/afsk/afsk_tx_task.h"
#include "dsp/aprs/aprs_settings.h"
#include "dsp/dsp.h"
#include "types.h"
#include "ui/gain_info.h"
#include "ui/menu_options.h"
#include "ui/ui_types.h"
#include "ui/view.h"
#include "ui/button_widget.h"
#include "ui/console_widget.h"
#include "os/task_manager.h"
#include "aprs_table_widget.h"
#include "io/log_file.h"
#include "ui/map_view.h"

namespace dsp_ui {

#define APRS_DEBUG 1
#define EU_APRS_FREQ 144800000

using namespace dsp;

class APRSView : public View {
  public:
    APRSView(Rect parent_rect) : View(parent_rect) {
        init();
    }

    void on_packet(APRSPacket *packet);
    void send_packet(std::string info);

  private:
    static constexpr int title_height = 20;
    static constexpr int button_collapse_width = 30;
    static constexpr int panel_sep = 4;
    static constexpr int max_sources = 7;
    static constexpr int table_width = DISPLAY_X_PIXELS / 2 - 56;
    static constexpr int console_width = DISPLAY_X_PIXELS - table_width;

    void on_source_selected(APRSSource &source);
    bool reset_console = false;

    std::unique_ptr<LogFile> logger;

    Label title_widget{{0, 0, DISPLAY_X_PIXELS - button_collapse_width - 2, title_height}, C565_WHITE, C565_GREY_DARKER, ButtonStyle::BUTTON_STYLE_FLAT};

    GainInfoWidget gain{{0, 0, DISPLAY_X_PIXELS / 3, title_height}, display};
    APRSTableWidget table_view{{0, title_height + panel_sep, table_width - panel_sep / 2, 90 + title_height}, max_sources};
    ConsoleWidget console{{table_width + panel_sep / 2, title_height + panel_sep, console_width - panel_sep, 90 + title_height}, display};

    Button button_collapse{{DISPLAY_X_PIXELS - button_collapse_width - 1, 0, button_collapse_width, title_height}, display, "<<"};
    bool collapsed{false};

    Menu::menu_action_st menu_actions[6] = {{"Pause",
                                             [this]() {
                                                 if (!paused) {
                                                     stop();
                                                 } else {
                                                     resume();
                                                 }
                                             }},
                                            {"Send",
                                             [this]() {
                                                 send_packet(aprs_settings.message);
                                             }},
                                            {"Beacon",
                                             [this]() {
                                                 toggle_beacon();
                                             },
                                             C565_BUTTON_TEXT_FG, C565_BG_DISABLED},
                                            {"Text",
                                             [this]() {
                                                 settings();
                                             }},
                                            {"Settings",
                                             []() {
                                                 Menu::open();
                                             }},
                                            {"Exit", [this]() {
                                                 exit();
                                             }}};

    Menu::menu_actions_st actions = {menu_actions, sizeof(menu_actions) / sizeof(Menu::menu_action_st)};

    Menu::menu_actions_st *get_quick_actions() override {
        return &actions;
    }

    SignalToken aprs_signal_token;

    APRSSource current_source;

    std::unique_ptr<ui::MapView> map;

    aprs::settings aprs_settings{};

    int beacon_task_id{0};

    MODE previous_mode;
    uint16_t previous_waterfall_speed;

    bool on_input(const st_inputEvent event) override;
    void before_paint() override;
    void init();
    void stop();
    void resume();
    void exit();
    void settings();
    void start_rx();
    void toggle_beacon();
    void threshold();
    bool paused{false};
};

} // namespace dsp_ui

#endif
