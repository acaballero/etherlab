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
#include <memory>

MainView::MainView() : View({0, 0, DISPLAY_X_PIXELS + DISPLAY_PADDING * 2, DISPLAY_Y_PIXELS + DISPLAY_PADDING * 2}) {

    // Set names for debuggin purposes
    // TODO: Remove when not required if saving a few bytes is worth it (hope not)
    // this->set_name("main");
    // this->tune_w.set_name("tune");
    // this->fft_w.set_name("fft");
    // this->waterfall_w.set_name("waterfall");
    // this->radio_w.set_name("radio");
    // this->info_w.set_name("info");
    // this->menu_w.set_name("menu");
    // this->smeter_w.set_name("smeter");
    // this->powmeter_w.set_name("powmeter");
    // this->optionButtonsView.set_name("options");
    // this->numberEditView.set_name("numedt");
    // this->msg_w.set_name("msg");
    // this->iqbal_w.set_name("iqbal");
    // this->status_w.set_name("status");
    // this->header_w.set_name("header");

    this->fft_w.set_show_fps(true);
    this->fft_w.set_z_index(20);
    this->dbscale_w.set_z_index(10);
    // this->waterfall_w.set_show_fps(true);
    this->tune_w.set_visible(config.debug);
    this->radio_w.set_visible(!config.debug);
    this->info_w.set_visible(config.debug);
    this->info_w.set_show_fps(config.debug);
    this->menu_w.set_show_fps(config.debug);
    this->menu_w.set_z_index(100);
    this->smeter_w.set_visible(false);
    this->powmeter_w.set_visible(false);
    this->optionButtonsView.set_visible(false);
    this->optionButtonsView.set_z_index(30);
    this->numberEditView.set_visible(false);
    this->numberEditView.set_z_index(30);
    this->msg_w.set_z_index(1000);
    this->msg_w.set_visible(false);
    this->msg_w.get_display()->setPadding(8, 8);
    this->msg_w.get_display()->setVerticalLineSpacing(1);
    this->iqbal_w.set_visible(false);

    this->children_.reserve(40);

    add_children({&this->menu_w, &this->header_w, &this->tune_w, &this->smeter_w, &this->snr_w, &this->radio_w, &this->powmeter_w, &this->info_w,
                  &this->status_w, &this->dbscale_w, &this->frequency_w, &this->iqbal_w, &this->waterfall_w, &this->fft_w, &this->msg_w,
                  &this->optionButtonsView, &this->numberEditView});
}

void MainView::before_paint() {

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
    return &this->waterfall_w;
}

Widget *MainView::Spectrum() {
    return &this->fft_w;
}

Widget *MainView::IQBalance() {
    return &this->iqbal_w;
}

Widget *MainView::TuneInfo() {
    return &this->tune_w;
}

Widget *MainView::FFTInfo() {
    return &this->info_w;
}

Widget *MainView::FFT() {
    return &this->fft_w;
}

Widget *MainView::Menu() {
    return &this->menu_w;
}

Widget *MainView::Message() {
    return &this->msg_w;
}

OptionButtonsView *MainView::OptionButtons() {
    return &this->optionButtonsView;
}

NumberEditView *MainView::NumberEdit() {
    return &this->numberEditView;
}

void MainView::on_child_update(Widget *w) {

    View::on_child_update(w);

    if (!w->visible()) {
        // If a children disapears, repaint all to recover the background
        // TODO: Should this be like this for all views?
        this->set_dirty();
    }
}

bool MainView::on_input(const st_inputEvent event) {

    bool consumed{false};

    if (event.is_touch()) { // If touch, consumption by one widget is preferred before the menu is brought to front
        consumed = View::on_input(event);
    }

    if (!consumed) {
        if (!focused_widget()) {

            consumed = menu_w.on_input(event); // First try to consume it by the menu

            if (consumed) {
                to_top(menu_w);
            }
        } else if (!event.is_touch()) {
            consumed = View::on_input(event);
        }

        if (!consumed) { // Let's see if the status bar can consume it
            consumed = status_w.on_input(event);
        }
    }

    return consumed;
}
