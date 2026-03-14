/*
 * Filename: toggle_button.h
 * Author: Angel Dust
 * Date: 2025-02-11
 */

#ifndef UI_TOGGLE_BUTTON
#define UI_TOGGLE_BUTTON

#include "Display_afb.h"
#include "button_widget.h"
#include <functional>

class ToggleButton : public Button {

  public:
    ToggleButton(Rect rect, char *label, std::function<void(bool)> on_change, uint16_t fg_color, Display *display = Widget::default_display(),
                 uint16_t bg_color = C565_GREY_LIGHT,
                 ButtonStyle style = BUTTON_STYLE_FLAT, Align aling = ALIGN_LEFT, uint32_t id = 0)
        : Button(rect, display, label, fg_color, bg_color, style, aling, id) {

        this->on_change = on_change;
        action = [this](Button &, st_inputEvent) {
            value = !value;
            this->on_change(value);
        };
    };

  private:
    std::function<void(bool)> on_change;
    bool value{false};
};

#endif // UI_TOGGLE_BUTTON
