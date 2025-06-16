//
// Created by Angel Dust on 16/06/2025.
//

#include "lock_view.h"
#include "ips_font.h"
#include "ui/widget.h"

void LockView::init() {

    lblTitle.set_font((FontDef *)&Font_11x18);
    lblTitle.set_color(C565_GREY_LIGHT);
    lblTitle.set_label("USB Mass Storage Device");
    lblTitle.set_aling(Align::ALIGN_CENTER);
    lblTitle.set_has_border(false);

    add_child(&lblTitle);

    lblText1.set_font((FontDef *)&Font_Tiny8x8);
    lblText1.set_color(C565_GREY_LIGHT);
    lblText1.set_label("Unplug USB to exit");
    lblText1.set_aling(Align::ALIGN_CENTER);
    lblText1.set_has_border(false);

    add_child(&lblText1);
}

void LockView::before_paint() {
}
