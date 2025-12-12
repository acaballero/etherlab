//
// Created by Angel Dust on 30/11/2025.
//

#ifndef __BEACON_SETTINGS_VIEW_H__
#define __BEACON_SETTINGS_VIEW_H__

#include "Display_afb.h"
#include <functional>
#include <stdio.h>
#include "dsp/dsp_common.h"
#include "ips_font.h"
#include "menuBase.h"
#include "ui/button_widget.h"
#include "ui/number_field_widget.h"
#include "ui/ui_types.h"
#include "ui/view.h"
#include "ui/menu_actions.h"
#include "ui/widget.h"

namespace dsp_ui {

class BeaconSettingsView : public View {
  public:
    BeaconSettingsView(std::function<void(bool)> on_select)
        : View({(DISPLAY_X_PIXELS - width) / 2, (DISPLAY_Y_PIXELS - height) / 2, width, height}, "Beacon settings"), on_select{on_select} {
        init();
    }

    bool on_touch(const st_inputEvent) override;
    bool on_input(const st_inputEvent e) override;

    Menu::menu_actions_st *get_quick_actions() override {
        return &quick_actions;
    };

  private:
    static constexpr uint8_t c_width = 11; // Make it match font width
    static constexpr uint8_t c_height = 20;
    static constexpr uint16_t height = 140;
    static constexpr uint16_t width = DISPLAY_X_PIXELS - 40;
    static constexpr uint8_t margin = 20;

    void before_paint() override{};
    void init();

    Label gainLabel{{1 * c_width, 1 * c_height + margin}, "Gain:", C565_TEXT_FG};
    NumberField gainField{{9 * c_width, 1 * c_height + margin}, 5, {DSP_MIN_TX_GAIN_DB, DSP_MAX_TX_GAIN_DB}, 1, " db"};
    Label periodLabel{{1 * c_width, 2 * c_height + margin}, "Period:", C565_TEXT_FG};
    NumberField periodField{{9 * c_width, 2 * c_height + margin}, 5, {5, 20}, 1, " s."};

    Button button_ok{{}, &lcd, "OK", C565_BUTTON_TEXT_FG, C565_GREY_DARK, BUTTON_STYLE_3D, ALIGN_CENTER};

    Button button_cancel{{}, &lcd, "Cancel", C565_BUTTON_TEXT_FG, C565_GREY_DARK, BUTTON_STYLE_3D, ALIGN_CENTER};

    const std::function<void(bool)> on_select{nullptr};

    Menu::menu_actions_st quick_actions = {Menu::get_navigation_actions().actions, 4};
};

} // namespace dsp_ui

#endif
