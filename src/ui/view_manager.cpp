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
#include "ui/view.h"

namespace view_manager {

static const uint8_t MAX_VIEWS = 6;

MainView mainView;
SplashView splashView;
KeypadView keypadView{{0, HEADER_HEIGHT, DISPLAY_X_PIXELS, KeypadView::HEIGHT}};
KeyboardView keyboardView{{0, HEADER_HEIGHT, KeyboardView::WIDTH, KeyboardView::HEIGHT}};
MessageView msg_w{
    {6, DISPLAY_Y_PIXELS * 2 / 3, DISPLAY_X_PIXELS - 12, INFO_HEIGHT - 6}, (FontDef *)&Font_11x18, (FontDef *)&Font_7x10, C565_GREY_DARK, C565_RED, C565_WHITE};
// NumberEditView numberEditView{{0, DISPLAY_Y_PIXELS - NumberEditView::HEIGHT, DISPLAY_X_PIXELS, NumberEditView::HEIGHT}};
// OptionButtonsView optionButtonsView{{0, HEADER_HEIGHT, DISPLAY_X_PIXELS, OptionButtonsView::HEIGHT}};
View *breadcrumb[MAX_VIEWS];
View *currentView;
int view_index = -1;

std::unique_ptr<View> aprs_view_p;
std::unique_ptr<View> view_p;

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
    if (view_index > 0) {

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

    static os::periodic_task *t;

    if (t) {
        os::task_manager.remove(t);
    }

    View *current = breadcrumb[view_index];

    t = os::task_manager.set_timeout(4000, [current]() {
        current->remove_child(&msg_w);
        // msg_w.set_visible(false);
    });

    current->add_child(&msg_w); // does nothing if the child already has a parent
    current->to_top(&msg_w);
    //  msg_w.set_focus(true);
    msg_w.add_msg(st->code == status::ST_ERROR ? "W" : "I", st->msg);
}

void init() {
    status::status_signal.add(NULL, main_view_warning_callback);

    keypadView.on_hide_fn = pop;
    keyboardView.on_hide_fn = pop;
    msg_w.set_name("msg");
    mainView.set_visible(false);
    // optionButtonsView.on_hide_fn = pop;
    // numberEditView.on_hide_fn = pop;
    LOG("Initializing view manager\n");
    push(&splashView);

    lcd.backlight(true);

    os::task_manager.set_timeout(1500, []() {
        pop();
        push(&mainView);
    });
}

void view_loop() {
    // TODO: Delegate dirty state manaegnment to the widget itself based on
    // information change messages and refresh rate

    mainView.TuneInfo()->set_dirty();
    mainView.FFTInfo()->set_dirty();

    currentView->paint();
}

void open_aprs() {

    aprs_view_p.reset();

    aprs_view_p = std::make_unique<dsp_ui::APRSView>(Rect{0, MENU_START_Y - 50, DISPLAY_X_PIXELS, METERS_HEIGHT + 80});

    auto *view_ptr = aprs_view_p.get();
    aprs_view_p->on_hide_fn = [view_ptr]() {
        view_manager::mainView.remove_child(view_ptr);
        aprs_view_p.reset();
        Menu::close();
    };

    aprs_view_p->set_visible(true);
    aprs_view_p->set_z_index(200);
    aprs_view_p->set_focus(true);
    view_manager::mainView.add_child(aprs_view_p.get());
}

void open(std::unique_ptr<View> v) {
    view_p = move(v);

    auto *view_ptr = view_p.get();
    view_p->on_hide_fn = [view_ptr]() {
        view_manager::mainView.remove_child(view_ptr);
        view_p.reset();
    };

    view_manager::mainView.add_child(view_ptr);
    view_manager::mainView.to_top(view_ptr);
    view_ptr->set_focus(true);
}

} // namespace view_manager
