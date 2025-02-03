//
// Created by Angel Dust on 12/07/2024.
//

#ifndef TRX_FRONTEND_KEYPAD_VIEW_H
#define TRX_FRONTEND_KEYPAD_VIEW_H

#include "Display_afb.h"
#include "display_panel_buttons_widget.h"
#include "stdio.h"
#include "ui/widget.h"
#include "view.h"
#include "button_widget.h"
#include "math.h"
#include "label_widget.h"
#include "main_view.h"

class KeypadView : public View {
  public:
    static constexpr uint16_t HEIGHT = DISPLAY_Y_PIXELS - HEADER_HEIGHT;
    static constexpr uint16_t WIDTH = DISPLAY_X_PIXELS;

    KeypadView() : View() { init(); }

    KeypadView(Rect parent_rect) : View(parent_rect) { init(); }

    static const uint8_t MAX_DIGITS = 12 + 3; // 3 for decimal and thousands separator

    std::function<void(double)> on_changed{};

    void on_focus() override;

    void init();

    double value() const;

    void with_multipliers(bool);

    void set_value(double value, uint8_t frac_digits, const char *units, const char *label, double min = 0, double max = 0);

    bool on_input(const st_inputEvent event) override;

    void before_paint() override;

  private:
    bool show_multipliers = true;
    uint16_t focused_button = 0;
    static constexpr int cols = 4;
    static constexpr int button_w = WIDTH / cols;
    static constexpr int button_h = (HEIGHT - STATUS_HEIGHT) / 5;

    static constexpr char thousand_separator = ' ';
    static constexpr char decimal_separator = '.';

    uint8_t frac_digits = 6;
    Button buttons[12 + 3];
    uint8_t index = 0;
    char buff[MAX_DIGITS];
    double min;
    double max;

    Button button_M{{button_w * (cols - 1), button_h * 1, button_w, button_h}, display, "M", C565_YELLOW, C565_GREY_DARK, BUTTON_STYLE_3D, ALIGN_CENTER};
    Button button_K{{button_w * (cols - 1), button_h * 2, button_w, button_h}, display, "k", C565_YELLOW, C565_GREY_DARK, BUTTON_STYLE_3D, ALIGN_CENTER};
    Button button_1{{button_w * (cols - 1), button_h * 3, button_w, button_h}, display, "x1", C565_YELLOW, C565_GREY_DARK, BUTTON_STYLE_3D, ALIGN_CENTER};

    Button button_close{
        {button_w * (cols - 1), button_h * 4, button_w, button_h}, display, "Cancel", C565_WHITE, C565_GREY_DARKER, BUTTON_STYLE_3D, ALIGN_CENTER};

    Label text_widget{{0, 6, 3 * button_w, button_h - 7}, C565_BLACK, C565_WHITE, ButtonStyle::BUTTON_STYLE_FLAT};
    Label label_widget{{button_w * 3, 6, button_w, button_h - 7}, C565_BLACK, C565_GREY_LIGHT, ButtonStyle::BUTTON_STYLE_FLAT};

    const char *display_buttons_labels[6] = {"M", "k", "x1", "", "<", "Cancel"};
    const char *display_buttons_labels_no_mult[6] = {"x1", "", "", "", "<", "Cancel"};

    DisplayPanelButtonsWidget display_panel_buttons = {{0, DISPLAY_Y_PIXELS - STATUS_HEIGHT, DISPLAY_X_PIXELS, STATUS_HEIGHT}};

    void on_button(Button &button);

    void update_text();
    void del_char();
};

#endif // TRX_FRONTEND_KEYPAD_VIEW_H
