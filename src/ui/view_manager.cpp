//
// Created by Angel Dust on 12/07/2024.
//
#include "view_manager.h"
#include "Display_afb.h"
#include "os/periodic_task.h"
#include "status.h"
#include "stm32f4xx_hal.h"
#include "ui/keyboard_view.h"
#include "ui/keypad_view.h"
#include "ui/number_edit_view.h"
#include "ui/option_buttons_view.h"
#include "ui/splash_view.h"
#include "os/task_manager.h"
#include "dsp/aprs/aprs_ui.h"
#include "ui/ui_types.h"

namespace view_manager {

static const uint8_t MAX_VIEWS = 6;

MainView mainView;
SplashView splashView;
KeypadView keypadView{{0, HEADER_HEIGHT, DISPLAY_X_PIXELS, KeypadView::HEIGHT}};
KeyboardView keyboardView{{0, HEADER_HEIGHT, KeyboardView::WIDTH, KeyboardView::HEIGHT}};
// NumberEditView numberEditView{{0, DISPLAY_Y_PIXELS - NumberEditView::HEIGHT, DISPLAY_X_PIXELS, NumberEditView::HEIGHT}};
// OptionButtonsView optionButtonsView{{0, HEADER_HEIGHT, DISPLAY_X_PIXELS, OptionButtonsView::HEIGHT}};
View *breadcrumb[MAX_VIEWS];
View *currentView;
int view_index = -1;

void view_loop();
os::periodic_task task(250, view_loop);

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

void main_view_warning_callback(void *, void *args) {

    status::Status *st = (status::Status *)args;
    MessageWidget *w = ((MessageWidget *)view_manager::mainView.Message());
    if (w->visible()) {

    } else {
        w->set_visible(true);
        w->set_focus(true);
        os::task_manager.set_timeout(4000, []() {
            view_manager::mainView.Message()->set_visible(false);
        });
    }

    w->set_title(st->code == status::ST_ERROR ? "WARNING" : "INFO", st->code == status::ST_ERROR ? C565_RED : C565_YELLOW);
    w->set_msg(st->msg);
}

void init() {
    status::status_signal.add(NULL, main_view_warning_callback);

    splashView.paint();
    HAL_Delay(1500);

    push(&mainView);
    keypadView.on_hide_fn = pop;
    keyboardView.on_hide_fn = pop;
    // optionButtonsView.on_hide_fn = pop;
    // numberEditView.on_hide_fn = pop;
}

void view_loop() {
    // TODO: Delegate dirty state manaegnment to the widget itself based on
    // information change messages and refresh rate

    mainView.TuneInfo()->set_dirty();
    mainView.FFTInfo()->set_dirty();

    currentView->paint();
}

std::unique_ptr<dsp_ui::APRSView> view;
void open_aprs() {

    view.reset();

    view = std::make_unique<dsp_ui::APRSView>(Rect{0, MENU_START_Y - 50, DISPLAY_X_PIXELS, METERS_HEIGHT + 80});

    auto *view_ptr = view.get();
    view->on_hide_fn = [view_ptr]() {
        view_manager::mainView.remove_child(view_ptr);
        view.reset();
        Menu::menu_exit();
    };

    view->set_visible(true);
    view->set_z_index(200);
    view->set_focus(true);
    view_manager::mainView.add_child(view.get());
}

} // namespace view_manager
