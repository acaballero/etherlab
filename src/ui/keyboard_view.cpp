//
// Created by Angel Dust on 15/01/2025.
//

#include "keyboard_view.h"
#include "Display_afb.h"
#include "input/inputEvent.h"
#include "ui/button_widget.h"
#include "ui/widget.h"

bool KeyboardView::on_input(const st_inputEvent event) {

    bool consumed = Widget::on_input(event);

    if (!consumed) {

        switch (event.type) {
            case INPUT_EVENT_TYPE_ENCODER:
                focused_button += event.value;
                if (focused_button < 0) {
                    focused_button = sizeof(buttons) - 1;
                } else if (focused_button >= sizeof(buttons)) {
                    focused_button = 0;
                }
                buttons[focused_button].set_focus(true);
                consumed = true;
                break;
            case INPUT_EVENT_TYPE_TOUCH_START:

                consumed = true;
                break;

            case INPUT_EVENT_TYPE_BUTTON_PRESS:
            case INPUT_EVENT_TYPE_BUTTON_DBL_PRESS:

                switch (event.value) {

                    case KEY_BACK:
                        text_widget.set_focus(true);
                        break;
                    case FPANEL_DISPLAY_BUTTON_1:
                        set_mode(mode == ALPHA ? NUMERIC : ALPHA);
                        break;
                    case FPANEL_DISPLAY_BUTTON_2:
                        on_shift();
                        break;
                    case FPANEL_DISPLAY_BUTTON_3:
                        text_widget.del_char();
                        break;
                    case FPANEL_DISPLAY_BUTTON_5:
                        on_ok();
                        break;
                    case FPANEL_DISPLAY_BUTTON_6:
                        this->set_visible(false);
                        break;
                    default:
                        button_ok.set_focus(true);
                        break;
                }

                consumed = true;
                break;

            default:
                consumed = false;
                break;
        }
    }

    return consumed;
}

void KeyboardView::on_ok() {
    if (on_changed) {
        on_changed(text_widget.get_text());
    }

    this->set_visible(false);
}

void KeyboardView::on_shift() {
    switch (shift) {

        case SHIFT:
            shift = LOCK;
            break;
        case LOCK:
            shift = NONE;
            break;
        default:
            shift = SHIFT;
            break;
    }

    if ((mode == NUMERIC) && (shift == SHIFT)) {
        shift = LOCK;
    }

    refresh_keys();
}

void KeyboardView::init() {

    const auto button_fn = [this](Button &button) { this->on_button(button); };

    label_widget.set_font((FontDef *)&Font_7x10);
    label_widget.set_color(C565_GREY_DARKER);
    label_widget.set_aling(ALIGN_CENTER);
    label_widget.set_has_border(false);
    label_widget.set_canvas_bg_color(C565_GREY_LIGHT);

    text_widget.set_inserting(true);

    add_child(&label_widget);
    add_child(&text_widget);

    button_shift.on_select = [this](Button &) { on_shift(); };

    for (int n = 0; n < key_count; n++) {

        Button &button = buttons[n];
        add_child(&button);

        button.id = n;
        button.on_highlight = [this](Button &button) { focused_button = button.id; };
        button.on_select = button_fn;
        button.set_aling(Align::ALIGN_CENTER);
        button.set_style(ButtonStyle::BUTTON_STYLE_3D);
        button.set_parent_rect({(n % (cols - 1)) * button_w, (n / (cols - 1)) * button_h + button_h, button_w, button_h});
    }

    button_shift.set_aling(Align::ALIGN_CENTER);
    button_mode.set_aling(Align::ALIGN_CENTER);
    button_del.set_aling(Align::ALIGN_CENTER);
    button_ok.set_aling(Align::ALIGN_CENTER);
    button_close.set_aling(Align::ALIGN_CENTER);

    button_shift.set_style(BUTTON_STYLE_3D);
    button_mode.set_style(BUTTON_STYLE_3D);
    button_ok.set_style(BUTTON_STYLE_3D);
    button_del.set_style(BUTTON_STYLE_3D);
    button_close.set_style(BUTTON_STYLE_3D);

    button_shift.on_select = button_fn;

    button_del.on_select = [this](Button &) { text_widget.del_char(); };

    button_close.on_select = [this](Button &) { this->set_visible(false); };

    button_ok.on_select = [this](Button &) { on_ok(); };

    display_panel_buttons.set_labels(display_buttons_labels);
    display_panel_buttons.get_buttons()[4].set_fg(C565_GREEN_DARK);

    add_children({&button_shift, &button_mode, &button_del, &button_ok, &button_close, &display_panel_buttons});

    set_label(label_widget.get_label());

    set_mode(mode);

    button_mode.on_select = [this](Button &) { set_mode(mode == ALPHA ? NUMERIC : ALPHA); };

    text_widget.set_focus(true);
}

void KeyboardView::on_focus() { button_close.set_focus(true); }

void KeyboardView::set_mode(Mode m, ShiftMode sm) {

    shift = sm;
    mode = m;
    refresh_keys();

    if (mode == Mode::ALPHA) {
        button_mode.set_text("abc");
    } else {
        button_mode.set_text("123");
    }
}

void KeyboardView::refresh_keys() {

    auto key_list = shift == NONE ? (mode == ALPHA ? keys_lower : keys_digit) : (mode == ALPHA ? keys_upper : keys_symbl);

    size_t n = 0;
    for (auto &button : buttons) {
        if (n > strlen(key_list)) {
            button.set_text("-");
        } else {
            button.set_text(std::string{key_list[n]}.c_str());
        }
        n++;
    }

    switch (shift) {
        case NONE:
            button_shift.set_fg(C565_BLACK);
            break;
        case SHIFT:
            button_shift.set_fg(C565_BLUE);
            break;
        case LOCK:
            button_shift.set_fg(C565_MAGENTA);
            break;
    }
}

void KeyboardView::set_size(uint8_t s) { text_widget.set_size(s); }

char *KeyboardView::text() { return text_widget.get_text(); }

void KeyboardView::set_text(const char *text) {

    text_widget.set_text(text);
    label_widget.set_dirty();
}

void KeyboardView::set_label(const char *text) {
    label_widget.set_label(text);

    int x = label_widget.parent_rect().right() - (font->width) * strlen(text) - 10;
    int w = label_widget.parent_rect().right() - x + 1;

    text_widget.set_parent_rect({text_widget.parent_rect().left(), 4, x + 2, label_widget.parent_rect().height()});
    text_widget.set_max_shown((text_widget.parent_rect().width() / font->width) - 2);

    label_widget.set_parent_rect({x, 4, w, label_widget.parent_rect().height()});
}

void KeyboardView::on_button(Button &button) {
    const auto c = button.get_text()[0];
    if (c != '\0') {
        text_widget.add_char(c);
    }

    if (shift == SHIFT) {
        shift = NONE;
        refresh_keys();
    }
}

void KeyboardView::before_paint() {}
