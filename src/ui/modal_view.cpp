//
// Created by Angel Dust on 09/07/2025.
//

#include "modal_view.h"
#include "Display_afb.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "ui/ui_types.h"
#include <functional>
#include <string>

ModalView::ModalView(

    const std::string &title, const std::string &message, modal_t type, std::function<void(bool)> callback)
    : message{message}, type{type}, on_select{callback} {

    set_bg(C565_DARKEST);

    int x = 20;

    std::string message_wrapped = display->fit_text(message, DISPLAY_X_PIXELS - x * 2, 40);
    Size dim = display->get_text_size(message_wrapped);

    int w = DISPLAY_X_PIXELS - x * 2;

    int button_width = w / 4;
    int button_height = 36;

    int title_height = 30;
    int vert_padding = 20;
    int text_top = title_height + vert_padding;
    int btn_top_margin = 30;
    int btn_top = text_top + dim.height() + btn_top_margin;

    int h = btn_top + button_height + vert_padding;
    h = min2(h, DISPLAY_Y_PIXELS);

    int y = (DISPLAY_Y_PIXELS - h) / 2;

    set_parent_rect({x, y, w, h});

    button_ok.set_aling(Align::ALIGN_CENTER);
    button_yes.set_aling(Align::ALIGN_CENTER);
    button_no.set_aling(Align::ALIGN_CENTER);

    if (type == INFO) {

        button_ok.set_parent_rect({(w - button_width) / 2, btn_top, button_width, button_height});

        add_child(&button_ok);
        button_ok.action = [this](Button &, st_inputEvent) {
            if (on_select) {
                on_select(true);
            }
            set_visible(false);
        };

    } else if (type == YESNO) {

        button_yes.set_parent_rect({button_width / 2, btn_top, button_width, button_height});
        button_no.set_parent_rect({5 * button_width / 2, btn_top, button_width, button_height});

        add_children({&button_yes, &button_no});

        button_yes.action = [this](Button &, st_inputEvent) {
            if (on_select) {
                on_select(true);
            }
            set_visible(false);
        };
        button_no.action = [this](Button &, st_inputEvent) {
            if (on_select) {
                on_select(false);
            }
            set_visible(false);
        };

    } else { // ABORT

        button_ok.set_parent_rect({(w - button_width) / 2, btn_top, button_width, button_height});
        add_child(&button_ok);

        button_ok.action = [this](Button &, st_inputEvent) {
            if (on_select) {
                on_select(true);
            }
            set_visible(false);
        };
    }

    text_w.set_font((FontDef *)&Font_7x10);
    text_w.set_parent_rect({(max2(0, w - dim.width()) / 2), text_top, dim.width(), dim.height()});
    text_w.set_text(message_wrapped);
    text_w.set_bg(this->bg_color);
    add_child(&text_w);

    title_w.set_parent_rect({0, 0, w, title_height});
    title_w.set_aling(Align::ALIGN_CENTER);
    title_w.set_label(title.c_str());
    add_child(&title_w);

    // actions_signal.emit(&quick_actions);
}

void ModalView::before_paint() {
}

void ModalView::on_focus() {
    if ((type == YESNO)) {
        button_yes.set_focus(true);
    } else {
        button_ok.set_focus(true);
    }
}
