//
// Created by Angel Dust on 12/07/2024.
//
#include "view_manager.h"
#include "Display_afb.h"
#include "status.h"
#include "stm32f4xx_hal.h"
#include "ui/keyboard_view.h"
#include "ui/keypad_view.h"
#include "ui/number_edit_view.h"
#include "ui/option_buttons_view.h"
#include "ui/splash_view.h"

namespace view_manager {

static const uint8_t MAX_VIEWS = 6;

MainView mainView;
SplashView splashView;
KeypadView keypadView{{0, HEADER_HEIGHT, DISPLAY_X_PIXELS, KeypadView::HEIGHT}};
KeyboardView keyboardView{{0, HEADER_HEIGHT, KeyboardView::WIDTH, KeyboardView::HEIGHT}};
NumberEditView numberEditView{{0, DISPLAY_Y_PIXELS - NumberEditView::HEIGHT, DISPLAY_X_PIXELS, NumberEditView::HEIGHT}};
// OptionButtonsView optionButtonsView{{0, HEADER_HEIGHT, DISPLAY_X_PIXELS, OptionButtonsView::HEIGHT}};
View *breadcrumb[MAX_VIEWS];
View *currentView;
int view_index = -1;

void push(View *view) {
    if (view_index < MAX_VIEWS) {
        if (currentView) {
            currentView->set_visible(false);
        }
        currentView = view;
        breadcrumb[++view_index] = view;
        currentView->set_visible(true);
        currentView->set_dirty();
        currentView->paint();
    }
}

void pop() {
    if (view_index >= 0) {
        currentView->set_visible(false);
        currentView = breadcrumb[--view_index];

        currentView->set_visible(true);
        currentView->set_dirty();
        currentView->set_focus(true);
        currentView->paint();
    }
}

void main_view_warning_callback(void *thisptr, void *args) {

    status::Status *st = (status::Status *)args;

    view_manager::mainView.Message()->set_visible(true);

    MessageWidget *widget = ((MessageWidget *)view_manager::mainView.Message());
    widget->set_title(st->code == status::ST_ERROR ? "WARNING" : "INFO", st->code == status::ST_ERROR ? C565_RED : C565_YELLOW);
    widget->set_msg(st->msg);
}

void init() {
    status::status_signal.add(NULL, main_view_warning_callback);

    splashView.paint();
    HAL_Delay(1500);

    push(&mainView);
    keypadView.on_hide_fn = pop;
    keyboardView.on_hide_fn = pop;
    // optionButtonsView.on_hide_fn = pop;
    numberEditView.on_hide_fn = pop;
}
} // namespace view_manager
