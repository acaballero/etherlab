//
// Created by Angel Dust on 12/07/2024.
//

#ifndef TRX_FRONTEND_KEYPAD_VIEW_H
#define TRX_FRONTEND_KEYPAD_VIEW_H

#include "stdio.h"
#include "view.h"
#include "button_widget.h"
#include "math.h"
#include "label_widget.h"
#include "main_view.h"

class KeypadView : public View {
public:

    static constexpr uint16_t HEIGHT = DISPLAY_Y_PIXELS - HEADER_HEIGHT * 2;
    static constexpr uint16_t WIDTH = DISPLAY_X_PIXELS;

    KeypadView() : View() { init(); }

    KeypadView(Rect parent_rect) : View(parent_rect) { init(); }

    static const uint8_t MAX_DIGITS = 12;

    std::function<void(double)> on_changed{};

    void on_focus() override;

    void init();

    double value() const;

    void set_value(double value, uint8_t frac_digits, const char *units, const char *label);

    bool on_input(const st_inputEvent event) override;

    void do_paint() override;

private:

    uint16_t focused_button = 0;
    static constexpr int cols = 4;
    static constexpr int button_w = WIDTH / cols;
    static constexpr int button_h = HEIGHT / 5;

    uint8_t frac_digits = 6;
    Button buttons[12 + 3];
    uint8_t index = 0;
    char buff[MAX_DIGITS];

    Button button_M{
            {button_w * (cols - 1), button_h * 1, button_w, button_h},
            display,
            "M", C565_YELLOW
    };
    Button button_K{
            {button_w * (cols - 1), button_h * 2, button_w, button_h},
            display,
            "k", C565_YELLOW
    };
    Button button_1{
            {button_w * (cols - 1), button_h * 3, button_w, button_h},
            display,
            "x1", C565_YELLOW
    };

    Button button_close{
            {button_w * (cols - 1), button_h * 4, button_w, button_h}, display,
            "Cancel", C565_BLACK
    };

    Label text_widget{
            {0, 6, 3 * button_w, button_h - 7}
    };

    Label label_widget{
            {button_w * 3, 6, button_w, button_h - 7}
    };

    void on_button(Button &button);

    void update_text();
};


#endif //TRX_FRONTEND_KEYPAD_VIEW_H
