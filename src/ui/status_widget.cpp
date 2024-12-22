//
// Created by Angel Dust on 17/04/2021.
//

#include "status_widget.h"
#include "../config.h"
#include "../main_board.h"
#include <functional>

void StatusWidget::init() {

   btnBand.fn_writer = std::bind(&StatusWidget::band, this, &btnBand);
   btnFilter1.fn_writer = std::bind(&StatusWidget::filter1, this, &btnFilter1);
   btnFilter2.fn_writer = std::bind(&StatusWidget::filter2, this, &btnFilter2);

   add_children({&btnModulation, &btnFrontend, &btnAgc, &btnBand, &btnFilter1, &btnFilter2});

   for (Widget *btn : View::children()) {
      ((Button *)btn)->set_font((FontDef *)&Font_Tiny8x8);
   }
}

void StatusWidget::mode() { sprintf(buf, ISTX ? "TX" : "RX"); }

const char *StatusWidget::modulation() { return radio::modulationNames[config.modulation]; }

char *StatusWidget::frontend() {
   if (!ISTX) {
      switch (config.frontend_path) {
      case radio::FRONTEND_PATH_THRU:
         sprintf(buf, "ATT:0");
         break;
      case radio::FRONTEND_PATH_ATT:
         sprintf(buf, "ATT:10");
         break;
      case radio::FRONTEND_PATH_LNA:
         sprintf(buf, "LNA");
         break;
      }
   } else {
      sprintf(buf, "-");
   }
   return buf;
}

char *StatusWidget::agc_alc() {
   if (!ISTX) {

      btnAgc.set_enabled(config.agc_enabled);

      sprintf(buf, "AGC");

      btnAgc.set_fg(fg_color);

   } else {
      sprintf(buf, "ALC");
   }

   return buf;
}

void StatusWidget::band(Widget *) {
   if (config.filter < radio::BAND_AUTO) {
      sprintf(buf, "%.3s", radio::bandNames[config.filter]);
   } else {
      if (this->_status.filter == radio::BAND_ALL) {
         sprintf(buf, "*");
      } else {
         sprintf(buf, "%.3s", radio::bandNames[this->_status.filter]);
      }
   }

   display->print("}:", buf, "", dimm_color, config.filter < radio::BAND_AUTO ? fg_color : fg_color_auto, dimm_color);
}

void StatusWidget::filter1(Widget *) {
   if (config.if_filter != radio::IF_FILTER_AUTO) {
      sprintf(buf, "%s", radio::IFFilterNames[config.if_filter]);
   } else {
      sprintf(buf, "%s", radio::IFFilterNames[this->_status.if_filter]);
   }
   display->print("~:", buf, "", dimm_color, config.filter < radio::BAND_AUTO ? fg_color : fg_color_auto, dimm_color);
}

void StatusWidget::filter2(Widget *) {

   display->print("B:");

   if (config.band < radio::BAND_AUTO) {
      display->print(radio::bandNames[config.band]);
   } else {
      if (config.band == radio::BAND_NONE) {
         display->print("*");
      }

      if (this->_status.band < radio::BAND_AUTO) {
         display->setColor(config.filter < radio::BAND_AUTO ? fg_color : fg_color_auto);
         display->print(radio::bandNames[this->_status.band]);
      }
   }
}

void StatusWidget::paint_callback() { display->clear(); }

void StatusWidget::do_paint() {

   radio::BAND band = config.band == radio::BAND_AUTO ? radio::find_band(radio::get_frequency()) : config.band;

   st_status status = {
       config.modulation, ISTX, band, radio::filter, radio::if_filter, config.frontend_path, config.agc_enabled,
       _status.f_carrier // we won't show the frequency in the status bar, so use current_status value

   };

   if (this->dirty() || !(status == _status)) { // Update only if status has changed
      this->set_dirty();
      _status = status;

      if (ISTX) {
         fg_color = C565_GREY_LIGHT;
         bg_color = C565_WHITE;
         dimm_color = C565_BLACK;
         disabled_bg = C565_GREY_DARK;
         fg_color_auto = C565_WHITE;
      } else {
         fg_color = C565_BLACK;
         bg_color = C565_WHITE;
         dimm_color = C565_BLACK;
         disabled_color = C565_GREY_DARK;
         disabled_bg = C565_GREY_LIGHT;
         fg_color_auto = C565_MAGENTA;
      }

      display->setBgColor(bg_color);
      display->setColor(fg_color);

      display->setFont((FontDef *)&Font_Tiny8x8);
      display->setVerticalLineSpacing(6);
      display->setPadding(4, 4);
      display->gotoCharXY(0, 0);

      btnModulation.set_text(modulation());
      btnFrontend.set_text(frontend());
      btnAgc.set_text(agc_alc());

      for (Widget *btn : View::children()) {
         ((Button *)btn)->set_bg(bg_color);
         btn->set_aling(ALIGN_CENTER);
      }

      // Selectively paint all children.
      for (const auto child : this->children()) {
         if (child->visible()) {
            child->set_dirty();
            child->paint();
            child->set_clean();
         }
      }
   }
}
