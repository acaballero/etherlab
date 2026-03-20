//
// Created by Angel Dust on 17/04/2021.
//
#include "input/inputEvent.h"
#include "power_amp.h"
#include "rf_coupler.h"
#include "main_view.h"
#include "lcd.h"
#include "menu.h"
#include "status.h"
#include "ui/number_edit_view.h"
#include "ui/option_buttons_view.h"
#include "ui/widget.h"
#include "view_manager.h"
#include "menu_options.h"
#include "dsp/receive/receive_ui.h"
#include <memory>

MainView::MainView() : View({0, 0, DISPLAY_X_PIXELS + DISPLAY_PADDING * 2, DISPLAY_Y_PIXELS + DISPLAY_PADDING * 2}) {

    // Set names for debuggin purposes
    // TODO: Remove when not required if saving a few bytes is worth it (hope not)
    this->set_name("main");
    tune_w.set_name("tune");
    fft_w.set_name("fft");
    fft_band_bar.set_name("band");
    waterfall_w.set_name("wate");
    radio_w.set_name("radio");
    info_w.set_name("info");
    menu_w.set_name("menu");
    cw_console_w.set_name("cwcon");
    smeter_w.set_name("smeter");
    powmeter_w.set_name("powr");
    optionButtonsView.set_name("opti");
    pow_metrics_w.set_name("powm");
    pow_metrics_w.set_z_index(200);

    iqbal_w.set_name("iqbal");
    status_w.set_name("status");
    header_w.set_name("head");

    fft_w.set_show_fps(true);
    fft_w.set_z_index(20);
    fft_band_bar.set_z_index(20);
    fft_x.set_z_index(20);
    dbscale_w.set_z_index(10);
    // waterfall_w.set_show_fps(true);
    tune_w.set_visible(config.debug);
    radio_w.set_visible(!config.debug);
    info_w.set_visible(config.debug);
    info_w.set_z_index(10);
    info_w.set_show_fps(config.debug);
    menu_w.set_show_fps(config.debug);
    menu_w.set_z_index(500);
    smeter_w.set_visible(false);
    powmeter_w.set_visible(false);
    optionButtonsView.set_visible(false);
    optionButtonsView.set_z_index(300);
    cw_console_w.set_z_index(15);
    cw_console_w.set_visible(false);

    frequency_w.set_z_index(400); // Top-most widget will receive the default focus

    iqbal_w.set_visible(false);

    dspReceiveUI::init(&cw_console_w, &waterfall_w, &iqbal_w);

    children_.reserve(40);

    add_children({&menu_w, &header_w, &pow_metrics_w, &tune_w, &smeter_w, &snr_w, &radio_w, &powmeter_w, &info_w, &status_w, &dbscale_w, &frequency_buttons_w,
                  &frequency_w, &iqbal_w, &waterfall_w, &cw_console_w, &fft_w, &fft_band_bar, &fft_x, &optionButtonsView});
}

MainView::~MainView() {
    dspReceiveUI::deinit();
}

void MainView::before_paint() {

    dspReceiveUI::before_paint();

    pow_metrics_w.set_visible(ISTX);

    if (Menu::menuStatus == Menu::IDLE) {
        if (config.debug) {
            smeter_w.set_visible(false);
            radio_w.set_visible(true);
            powmeter_w.set_visible(false);
            tune_w.set_visible(true);
            info_w.set_visible(true);
        } else {
            smeter_w.set_visible(!ISTX);
            snr_w.set_visible(!ISTX);
            radio_w.set_visible(true);
            powmeter_w.set_visible(ISTX);
            tune_w.set_visible(false);
            info_w.set_visible(false);
        }
        menu_w.set_visible(false);
    } else {
        tune_w.set_visible(false);
        radio_w.set_visible(false);
        smeter_w.set_visible(false);
        powmeter_w.set_visible(false);
        snr_w.set_visible(false);
        info_w.set_visible(false);
        menu_w.set_visible(true);
    }
}

WaterfallWidget *MainView::Waterfall() {
    return &waterfall_w;
}

Widget *MainView::Spectrum() {
    return &fft_w;
}

Widget *MainView::IQBalance() {
    return &iqbal_w;
}

Widget *MainView::TuneInfo() {
    return &tune_w;
}

Widget *MainView::FFTInfo() {
    return &info_w;
}

Widget *MainView::FFT() {
    return &fft_w;
}

Widget *MainView::Menu() {
    return &menu_w;
}

Widget *MainView::Status() {
    return &status_w;
}

OptionButtonsView *MainView::OptionButtons() {
    return &optionButtonsView;
}

void MainView::on_child_update(Widget *w) {

    View::on_child_update(w);

    if (!w->visible()) {
        // If a children disapears, repaint all to recover the background
        // TODO: Should this be like this for all views?
        set_dirty();
    }
}

void MainView::on_child_focus_changed(Widget *w, bool was_focused) {

    View::on_child_focus_changed(w, was_focused);

    // Finds the focused widget
    if (!w->is_focused()) {
        w = focused_widget();
    }

    Menu::menu_actions_st *actions = nullptr;

    if (w && w->is_focused()) {

        // Find first widget having quick actions starting from the focused widget up
        actions = w->get_quick_actions();
        while (w && !actions) {
            w = w->parent();
            if (w) {
                actions = w->get_quick_actions();
            }
        }
    }

    if (actions) {
        Menu::navigation_signal.emit(w);
    } else {
        Menu::navigation_signal.emit(nullptr);
    }
}

bool MainView::on_input(const st_inputEvent event) {

    bool consumed{false};

    if (event.is_touch()) { // If touch, consumption by one widget is preferred before the menu is brought to front
        consumed = View::on_input(event);
    }

    if (!consumed && !event.is_touch()) {
        consumed = View::on_input(event);
    }

    if (!consumed) {
        consumed = menu_w.on_input(event); // Try to consume it by the menu

        if (consumed) {
            if (!menu_w.visible()) {
                menu_w.set_visible(true);
                menu_w.set_focus(true);
            }
        }
    }

    if (!consumed) { // Let's see if the status bar can consume it
        consumed = status_w.on_input(event);
    }

    return consumed;
}
