//
// Created by Angel Dust on 2/02/2025.
//

#ifndef TRX_FRONTEND_NUMBER_EDIT_VIEW_H
#define TRX_FRONTEND_NUMBER_EDIT_VIEW_H

#include "Display_afb.h"
#include "display_panel_buttons_widget.h"
#include "stdio.h"
#include "ui/widget.h"
#include "view.h"
#include "button_widget.h"
#include "math.h"
#include "label_widget.h"
#include <cstdint>

class NumberEditView : public View {
  public:
    static constexpr uint16_t COLS = 4;
    static constexpr uint16_t MARGIN = 3;
    static constexpr uint16_t BUTTON_H = 50;
    static constexpr uint16_t BUTTONS_Y = HEADER_HEIGHT + BUTTON_H + MARGIN * 3;
    static constexpr uint16_t HEIGHT = BUTTONS_Y + 2 * BUTTON_H + STATUS_HEIGHT;
    static constexpr uint16_t WIDTH = DISPLAY_X_PIXELS;
    static constexpr int BUTTON_W = WIDTH / COLS;

    NumberEditView() : View() {
        init();
    }

    NumberEditView(Rect parent_rect) : View(parent_rect) {
        init();
    }

    std::function<void(double)> on_changed{};

    void on_focus() override;

    void init();

    double get_value() const;

    void set_value(double value, uint8_t frac_digits, const char *units, const char *label, double min = 0, double max = 0, double step = 1,
                   double step_big = 10);

    bool on_input(const st_inputEvent event) override;

    void before_paint() override;

    void set_update_on_changes(bool b) {
        update_on_changes = b;
    }

  private:
    uint16_t focused_button = 0;

    static constexpr char thousand_separator = ' ';
    static constexpr char decimal_separator = '.';
    static constexpr uint8_t max_length = 20;
    enum BUTTONS { DECR_BIG, DECR, INCR, INCR_BIG, OK, CANCEL };

    uint8_t frac_digits = 6;

    double min;
    double max;
    double step;
    double step_big;
    double value;
    double initial_value;

    // Call callback on every value change
    bool update_on_changes{true};

    void update_value(double);

    Button buttons[6]{{{0, BUTTONS_Y, BUTTON_W, BUTTON_H}, display, "<<", C565_BLACK, C565_GREY_DARK, BUTTON_STYLE_3D, ALIGN_CENTER, DECR_BIG},
                      {{BUTTON_W, BUTTONS_Y, BUTTON_W, BUTTON_H}, display, "<", C565_BLACK, C565_GREY_DARK, BUTTON_STYLE_3D, ALIGN_CENTER, DECR},
                      {{BUTTON_W * 2, BUTTONS_Y, BUTTON_W, BUTTON_H}, display, ">", C565_BLACK, C565_GREY_DARK, BUTTON_STYLE_3D, ALIGN_CENTER, INCR},
                      {{BUTTON_W * 3, BUTTONS_Y, BUTTON_W, BUTTON_H}, display, ">>", C565_BLACK, C565_GREY_DARK, BUTTON_STYLE_3D, ALIGN_CENTER, INCR_BIG},
                      {{0, BUTTONS_Y + BUTTON_H, BUTTON_W *COLS / 2, BUTTON_H}, display, "OK", C565_BLACK, C565_GREY_DARK, BUTTON_STYLE_3D, ALIGN_CENTER, OK},
                      {{COLS / 2 * BUTTON_W, BUTTONS_Y + BUTTON_H, BUTTON_W *COLS / 2, BUTTON_H},
                       display,
                       "Cancel",
                       C565_WHITE,
                       C565_GREY_DARKER,
                       BUTTON_STYLE_3D,
                       ALIGN_CENTER,
                       CANCEL}};

    Label label_widget{{3 * BUTTON_W, HEADER_HEIGHT + MARGIN * 2, BUTTON_W, BUTTON_H}, C565_GREY_DARKER, C565_GREY_LIGHT, ButtonStyle::BUTTON_STYLE_FLAT};
    Label text_widget{{0, HEADER_HEIGHT + MARGIN * 2, 3 * BUTTON_W, BUTTON_H}, C565_BLACK, C565_WHITE, ButtonStyle::BUTTON_STYLE_FLAT};
    Label title{{0, MARGIN, WIDTH, HEADER_HEIGHT}, C565_WHITE, C565_GREY_DARKER, ButtonStyle::BUTTON_STYLE_FLAT};

    const char *display_buttons_labels[6] = {"<<", "<", ">", ">>", "OK", "Cancel"};

    DisplayPanelButtonsWidget display_panel_buttons = {{0, HEIGHT - STATUS_HEIGHT, DISPLAY_X_PIXELS, STATUS_HEIGHT}};

    void on_button(Button &button);
};

#endif // TRX_FRONTEND_NUMBER_EDITVIEW_H
