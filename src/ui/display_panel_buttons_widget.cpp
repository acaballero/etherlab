//
// Created by Angel Dust on 17/04/2021.
//

#include "display_panel_buttons_widget.h"
#include "../config.h"
#include "../main_board.h"
#include "Display_afb.h"
#include "ui/menu.h"
#include "ui/status_widget.h"
#include <functional>

void DisplayPanelButtonsWidget::init() {

    int w = DISPLAY_X_PIXELS / n_buttons;

    for (int i = 0; i < n_buttons; i++) {

        buttons[i].set_font((FontDef *)&Font_Tiny8x8);
        buttons[i].set_parent_rect({i * w, STATUS_MARGIN_TOP, w - (i < n_buttons - 1 ? separation : 0), this->parent_rect().height() - STATUS_MARGIN_TOP});
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

void DisplayPanelButtonsWidget::before_paint() {}
