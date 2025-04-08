//
// Created by Angel Dust on 12/07/2024.
//

#include "option_buttons_view.h"
#include "Display_afb.h"
#include "ips_font.h"
#include "status.h"
#include "ui/button_widget.h"
#include "ui/main_view.h"
#include "ui/menu_options.h"
#include "ui/ui_types.h"
#include "ui/widget.h"
#include "utils.hpp"
#include <functional>
#include <sys/_stdint.h>

bool OptionButtonsView::update_focus(int button_index) {

    bool first_focus = focused_button == -1;
    focused_button = button_index;
    if (focused_button < 0) {
        focused_button = index - 1;
    } else if (focused_button >= index) {
        focused_button = 0;
    }

    uint16_t page_size = cols * (rows - 1);
    uint16_t new_offset = offset;

    if ((focused_button - offset) >= page_size) {
        // Page down
        new_offset = focused_button - page_size + cols;
        new_offset -= (new_offset % cols);
    } else if (focused_button < offset) {
        // Page up
        new_offset = (focused_button / cols) * cols;
    }

    bool update = new_offset != offset || first_focus;
    offset = new_offset;
    update_buttons(update);

    if (this->visible()) {
        buttons[focused_button].set_focus(true);
    }

    return true;
}

bool OptionButtonsView::on_input(const st_inputEvent event) {

    bool consumed = Widget::on_input(event);

    if (consumed) {
        return consumed;
    }

    switch (event.type) {
        case INPUT_EVENT_TYPE_ENCODER:

            consumed = update_focus(focused_button + event.value);
            break;
        case INPUT_EVENT_TYPE_TOUCH_START:

            consumed = true;
            break;

        case INPUT_EVENT_TYPE_BUTTON_PRESS:
        case INPUT_EVENT_TYPE_BUTTON_DBL_PRESS:

            switch (event.value) {
                case KEY_BACK:
                    this->set_visible(false);
                    break;
                case FPANEL_DISPLAY_BUTTON_1:
                    consumed = update_focus(focused_button + event.value);
                    break;
                case FPANEL_DISPLAY_BUTTON_2:
                    consumed = update_focus(focused_button + event.value);
                    break;
                case FPANEL_DISPLAY_BUTTON_3:
                    break;
                case FPANEL_DISPLAY_BUTTON_5:
                    break;
                case FPANEL_DISPLAY_BUTTON_6:
                    this->set_visible(false);
                    break;
                default:
                    button_close.set_focus(true);
                    break;
            }

            consumed = true;
            break;
        default:
            consumed = false;
            break;
    }

    return consumed;
}

void OptionButtonsView::set_enabled(uint16_t index, bool b) {
    Button *button = &buttons[index];
    button->set_enabled(b);
}

void OptionButtonsView::add_item(const char *text, std::function<void(Button &, st_inputEvent)> on_select_handler, bool selected, uint16_t fg_color,
                                 uint16_t text_bg_color) {

    if (index < MAX_BUTTONS) {
        Button *button = &buttons[index];

        char name[8];
        sprintf(name, "opt-%d", index);
        button->set_name(name);
        button->set_visible(false);
        add_child(button);
        button->id = index;
        index++;

        button->on_highlight = [this](Button &button) { update_focus(button.id); };

        if (on_select_handler) {
            button->action = on_select_handler;
        } else if (on_select) {
            button->action = [this](Button &button, st_inputEvent) {
                ;
                on_select(button.id);
            };
        }

        button->set_style(BUTTON_STYLE_3D);
        button->set_aling(ALIGN_CENTER);
        button->set_fg(fg_color);
        button->set_text_bg(text_bg_color);
        button->set_text(text);

        if (index <= 4) {
            cols = 2;
        } else if (index <= 9) {
            cols = 3;
        }

        rows = ((index - 1) / cols) + 1 + 1; // Add one for back and arrow buttons

        if (rows > max_rows) {
            rows = max_rows;
        }

        button->set_focus(selected);

        if (selected) {
            update_focus(button->id);
        } else {
            update_buttons(true);
        }
    } else {
        status::handleError(status::ST_ERROR, "OptionButtonsView: maxed items");
    }
}

void OptionButtonsView::set_title(const char *text) { label_widget.set_label(text); }

void OptionButtonsView::clear() {
    for (Button &button : buttons) {
        remove_child(&button);
    }
    index = 0;
    offset = 0;
    focused_button = -1;
}

void OptionButtonsView::update_buttons(bool update_layout = false) {

    if (!index) {
        return;
    }

    int sep = 2;

    // Calculate optimum sizes
    button_w = WIDTH / cols;

    uint16_t height = (button_h * rows) + STATUS_HEIGHT + TITLE_HEIGHT + sep;
    if (update_layout) {
        set_parent_rect({0, DISPLAY_Y_PIXELS - height, WIDTH, height});
    }

    int page_size = cols * (rows - 1);

    for (int i = 0; i < index; i++) {
        buttons[i].set_visible(i >= offset && i - offset < page_size);

        if (update_layout) {
            buttons[i].set_parent_rect({((i - offset) % (cols)) * button_w, (((i - offset) / (cols)) * button_h) + TITLE_HEIGHT + sep, button_w, button_h});
        }
    }

    bool all_visible = page_size >= index;
    bool need_arrows = !all_visible && show_arrows;

    button_next.set_visible(need_arrows);
    button_prev.set_visible(need_arrows);

    if (need_arrows) {
        button_next.set_enabled(offset + page_size < index);
        button_prev.set_enabled(offset > 0);
    }

    if (update_layout) {
        button_prev.set_parent_rect({0, ((rows - 1) * button_h) + TITLE_HEIGHT + sep, button_w, button_h});
        button_next.set_parent_rect({button_w, ((rows - 1) * button_h) + TITLE_HEIGHT + sep, button_w, button_h});

        button_close.set_parent_rect({(cols - 1) * button_w, ((rows - 1) * button_h) + TITLE_HEIGHT + sep, button_w, button_h});
        display_panel_buttons.set_parent_rect({0, height - STATUS_HEIGHT, DISPLAY_X_PIXELS, STATUS_HEIGHT});
    }
}

void OptionButtonsView::set_show_arrows(bool b) { show_arrows = b; }

void OptionButtonsView::init() {

    label_widget.set_font((FontDef *)&Font_7x10);
    label_widget.set_aling(ALIGN_CENTER);

    label_widget.set_name("op-lb");
    button_close.set_name("op-bc");
    button_next.set_name("op-nx");
    button_prev.set_name("op-pr");
    display_panel_buttons.set_name("op-BT");

    add_child(&label_widget);

    add_children({&button_close, &button_next, &button_prev, &display_panel_buttons});

    button_close.action = [this](Button &, st_inputEvent) { this->set_visible(false); };

    display_panel_buttons.set_labels(display_buttons_labels);
}

void OptionButtonsView::on_focus() { update_focus(focused_button); }

void OptionButtonsView::before_paint() {}
