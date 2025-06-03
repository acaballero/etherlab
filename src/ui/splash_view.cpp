//
// Created by Angel Dust on 07/01/2025.
//

#include "splash_view.h"
#include "ips_font.h"
#include "ui/widget.h"

#ifndef BUILD_VERSION
#define BUILD_VERSION "unknown"
#endif

void SplashView::init() {

    lblTitle.set_font((FontDef *)&Font_11x18);
    lblTitle.set_color(C565_GREY_LIGHT);
    lblTitle.set_label("etherlab EL24");
    lblTitle.set_aling(Align::ALIGN_CENTER);
    lblTitle.set_has_border(false);

    add_child(&lblTitle);

    lblText1.set_font((FontDef *)&Font_Tiny8x8);
    lblText1.set_color(C565_GREY_LIGHT);
    lblText1.set_label(config.callsign);
    lblText1.set_aling(Align::ALIGN_CENTER);
    lblText1.set_has_border(false);

    add_child(&lblText1);

    lblText2.set_font((FontDef *)&Font_Tiny8x8);
    lblText2.set_color(C565_GREY_LIGHT);
    lblText2.set_label(config.version);
    lblText2.set_aling(Align::ALIGN_CENTER);
    lblText2.set_has_border(false);

    add_child(&lblText2);

    lblText3.set_font((FontDef *)&Font_Tiny8x8);
    lblText3.set_color(C565_GREY_LIGHT);
    lblText3.set_label(BUILD_VERSION);
    lblText3.set_aling(Align::ALIGN_CENTER);
    lblText3.set_has_border(false);

    add_child(&lblText3);
}

void SplashView::before_paint() {
}
