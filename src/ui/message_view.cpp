//
// Created by Angel Dust on 29/06/2022.
//

#include "Display_afb.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "ui/console_widget.h"
#include "message_view.h"

void MessageView::init() {

    set_focusable(true);

    set_border_width(border_width);
    set_border_color(C565_GREY_DARK);
    set_shadow_width(shadow_width);
    console.set_bg(C565_DARKEST);
    console.set_padding(4, 4);
    console.set_font((FontDef *)&Font_7x10);
    add_child(&console);

    text_w.set_font(text_font);

    text_w.set_bg(C565_TRANSPARENT);
    add_child(&text_w);

    title_w.set_parent_rect({padding, padding, parent_rect().width() - padding * 2, title_height});
    title_w.set_border_radius(false, false, false, false);
    title_w.set_aling(Align::ALIGN_CENTER);

    add_child(&title_w);
}

bool MessageView::on_input(const st_inputEvent e) {

    bool consumed = false;

    if (visible()) {
        switch (e.type) {

            case INPUT_EVENT_TYPE_TOUCH_START:
                //    case INPUT_EVENT_TYPE_BUTTON_RELEASE:
                consumed = true; // swallow
                break;

            default:
                set_visible(false);
                display->draw_area(&this->area, this);
                consumed = true;
                break;
        }
    }

    return consumed;
}

void MessageView::before_paint() {
    if (this->dirty()) {
        set_focus(true);
    }
}

void MessageView::clear_log() {
    console.clear();
}
void MessageView::on_show() {
}

void MessageView::add_log(const char *header, const char *str) {
    st_datetime datetime = rtc_get_date_time();

    char buff[11];
    sprintf(buff, "[%02d:%02d:%02d] ", datetime.time.Hours, datetime.time.Minutes, datetime.time.Seconds);
    std::string msg = ConsoleWidget::color_mark + std::string(1, (char)1) + buff + ConsoleWidget::color_mark + std::string(1, (char)2) + header + ": " +
                      ConsoleWidget::color_mark + std::string(1, (char)1) + str + "\n";

    auto n_lines = console.get_line_count() + 1;

    console.set_visible(true);
    text_w.set_visible(false);
    console.set_rows(min2(n_lines, 5));
    // Adjusts height to the content

    set_height(console.parent_rect().height() + padding * 2 + title_height);

    console.write(msg);
}

void MessageView::show_msg(const char *header, const char *str) {

    int w = parent_rect().width() - 10;
    display->setFont(text_w.get_font());
    std::string message_wrapped = display->fit_text(str, w, 60);
    Size dim = display->get_text_size(message_wrapped);
    int text_margin = 10;

    // Also add it to the console
    add_log(header, str);

    text_w.set_parent_rect({(max2(0, w - dim.width()) / 2), padding + title_height + text_margin, dim.width() + font->width, dim.height()});
    set_height(text_w.parent_rect().bottom() + text_margin);

    text_w.set_text(str);
    title_w.set_label(header);

    console.set_visible(false);
    text_w.set_visible(true);
}
