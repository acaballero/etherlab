//
// Created by Angel Dust on 17/04/2021.
//

#include "display_panel_buttons_widget.h"
#include "../config.h"
#include "../main_board.h"
#include "Display_afb.h"
#include "ui/menu.h"
#include <functional>

void DisplayPanelButtonsWidget::init() {

    int w = DISPLAY_X_PIXELS / n_buttons;

    for (int i = 0; i < n_buttons; i++) {

        buttons[i].set_font((FontDef *)&Font_Tiny8x8);
        buttons[i].set_parent_rect({i * w, DISPLAY_PADDING, w - 2, this->area.height});
        buttons[i].set_aling(Align::ALIGN_CENTER);
        buttons[i].set_fg(C565_BLACK);
        buttons[i].set_bg(C565_WHITE);

        add_child(&buttons[i]);
    }
}

void DisplayPanelButtonsWidget::set_labels(const char **labels) {

    for (int i = 0; i < n_buttons; i++) {
        buttons[i].set_text(labels[i]);
        if (labels[i][0] == 0) {
            buttons[i].set_visible(false);
        }
    }
}

Button *DisplayPanelButtonsWidget::get_buttons() { return buttons; }

void DisplayPanelButtonsWidget::paint_callback() {

    display->clear();

    for (const auto child : this->children()) {
        if (child->visible()) {
            uint16_t top = child->parent_rect().top();
            uint16_t left = child->parent_rect().left();
            uint16_t height = child->parent_rect().height();
            uint16_t width = child->parent_rect().width();

            display->setOffset(left, top, width, height);
            child->paint_callback();
            display->clearOffset();
        }
    }
}

void DisplayPanelButtonsWidget::before_paint() {}
