//
// Created by Angel Dust on 16/06/2025.
//

#include "lock_view.h"
#include "hw/stm32f4xx/usb.h"
#include "ips_font.h"
#include "tinyusb/usb_composite_device.h"
#include "ui/widget.h"

void LockView::init() {

    lblTitle.set_font((FontDef *)&Font_11x18);
    lblTitle.set_color(C565_GREY_LIGHT);
    lblTitle.set_label("USB Mass Storage Device");
    lblTitle.set_aling(Align::ALIGN_CENTER);
    lblTitle.set_has_border(false);

    add_child(&lblTitle);

    lblText1.set_font((FontDef *)&Font_Tiny8x8);
    lblText1.set_fg(C565_GREY_LIGHT);
    lblText1.set_text("Unplug USB or press any key to exit");
    lblText1.set_aling(Align::ALIGN_CENTER);

    add_child(&lblText1);
}

void LockView::before_paint() {
}

bool LockView::on_input(const st_inputEvent e) {
    if (!e.is_touch()) {
        char msg_err[128];
        if (!usb_set_msc_enabled(false, msg_err)) {
            LOG("Error disabling sass storage USB mode: %s", msg_err);
        }
        return true;
    } else {
        return false;
    }
}
