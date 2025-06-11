//
// Created by Angel Dust on 15/01/2025.
//

#ifndef TRX_FRONTEND_KEYBOARD_VIEW_H
#define TRX_FRONTEND_KEYBOARD_VIEW_H

#include "Display_afb.h"
#include "field_widget.h"
#include "stdio.h"
#include "types.h"
#include "ui/display_panel_buttons_widget.h"
#include "ui/ui_types.h"
#include "view.h"
#include "button_widget.h"
#include <cmath>
#include "label_widget.h"
#include "main_view.h"

class KeyboardView : public View {
  public:
    static constexpr uint16_t HEIGHT = DISPLAY_Y_PIXELS - HEADER_HEIGHT;
    static constexpr uint16_t WIDTH = DISPLAY_X_PIXELS;

    KeyboardView() : View() {
        init();
    }

    KeyboardView(Rect parent_rect) : View(parent_rect) {
        init();
    }

    std::function<void(char *)> on_changed = nullptr;

    void on_focus() override;

    void init();

    char *text();

    void set_text(const char *text);
    void set_label(const char *text);
    void set_size(uint8_t size);

    bool on_input(const st_inputEvent event) override;

    void before_paint() override;

  private:
    enum ShiftMode {
        NONE,
        SHIFT,
        LOCK,
    };

    enum Mode {
        ALPHA,
        NUMERIC,
    };

    uint16_t focused_button = 0;
    static constexpr int cols = 6;
    static constexpr int button_w = WIDTH / cols;
    static constexpr int button_h = (HEIGHT - STATUS_HEIGHT) / 7;
    static constexpr int key_count = 29;

    const char *const keys_lower = "abcdefghijklmnopqrstuvwxyz, .";
    const char *const keys_upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ, .";
    const char *const keys_digit = "1234567890()'`\"+-*/=<>_\\!?, .";
    const char *const keys_symbl = "!@#$%^&*()[]'`\"{}|:;<>-_~?, .";

    Button buttons[key_count];
    Mode mode = Mode::ALPHA;
    ShiftMode shift;

    void set_mode(Mode, ShiftMode = NONE);

    const char *display_buttons_labels[6] = {"abc", "^", "del", "", "Enter", "Cancel"};

    Button button_mode{{button_w * (cols - 1), button_h * 1, button_w, button_h}, display, "123", C565_VIOLET, C565_GREY_DARK};
    Button button_shift{{button_w * (cols - 1), button_h * 2, button_w, button_h}, display, "^", C565_VIOLET, C565_GREY_DARK};
    Button button_del{{button_w * (cols - 1), button_h * 3, button_w, button_h}, display, "del", C565_VIOLET, C565_GREY_DARK};
    Button button_ok{{button_w * (cols - 1), button_h * 4, button_w, button_h}, display, "Enter", C565_VIOLET, C565_GREY_DARK};
    Button button_close{{button_w * (cols - 1), button_h * 5, button_w, button_h}, display, "Cancel", C565_BLACK, C565_GREY_DARK};

    Field text_widget{{0, 4, 6 * button_w, button_h - 5}, "", C565_BLACK};

    Label label_widget{{button_w * 5, 4, button_w, button_h - 5}, C565_GREY_DARK, C565_TRANSPARENT, BUTTON_STYLE_FLAT};

    DisplayPanelButtonsWidget display_panel_buttons = {{0, HEIGHT - STATUS_HEIGHT, DISPLAY_X_PIXELS, STATUS_HEIGHT}};

    void on_button(Button &button);
    void on_ok();
    void on_shift();
    void refresh_keys();
};

#endif // TRX_FRONTEND_KEYBOARD_VIEW_H
