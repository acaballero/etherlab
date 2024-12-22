//
// Created by Angel Dust on 18/04/2021.
//

#include "frequency_widget.h"
#include "config.h"
#include "scanner.h"
#include "view_manager.h"

void FrequencyWidget::paint_callback() {

   char buf[20], buf2[13];
   uint16_t fg_color;

   display->clear();

   if (config.repeater_mode != radio::RPT_MODE_OFF) {

      uint16_t fg = C565_YELLOW;
      uint16_t bg = C565_OLIVE;
      uint8_t h = 10;
      uint8_t y0 = 3;

      display->setBgColor(bg);
      display->setColor(fg);
      display->gotoXY(1, y0 + 1);
      display->setFont((FontDef *)&Font_7x10);
      display->fill(0, y0, 9, y0 + h, bg);

      if (config.repeater_mode == radio::RPT_MODE_NEGATIVE) {
         display->print("-");
      } else {
         display->print("+");
      }
   }

   format_long(radio::get_frequency(), buf2);
   sprintf(buf, "%12s", buf2);

   display->setBgColor(C565_TRANSPARENT);

   if (scanner::scanner_config.status == scanner::SCANNER_STATUS_RUNNING) {
      fg_color = C565_MAGENTA;
   } else {
      fg_color = C565_YELLOW;
   }

   display->writeString(0, 0, buf, (FontDef *)&Font_11x18, fg_color, C565_TRANSPARENT);
   display->setColor(C565_GREY_LIGHT);
   display->setFont((FontDef *)&Font_7x10);
   display->setPadding(0, 5);
   display->print("Hz");

   uint8_t dec_place = 10 - (uint8_t)log10((double)config.f_step);
   uint16_t start_line = (dec_place * 11);

   if (config.f_step > 100)
      start_line -= 6; // sip hundreds separator
   if (config.f_step > 100000)
      start_line -= 6; // skip thousands separator

   display->writeRect(start_line, 17, start_line + 10, 17);
}

void FrequencyWidget::do_paint() {

   st_freqInfo freqInfo = {(unsigned long)radio::get_frequency(), config.f_step, config.repeater_mode};

   if (this->dirty() || !(freqInfo == this->status)) {
      this->status = freqInfo;
      display->drawArea(&this->area, this);
      this->set_dirty();
   }
}

bool FrequencyWidget::on_input(const st_inputEvent event) {
   switch (event.type) {

   case INPUT_EVENT_TYPE_TOUCH_END:
      view_manager::keypadView.set_value(radio::get_frequency(), 0, "Hz", "Frequency");
      view_manager::keypadView.on_changed = [](double v) { radio::set_frequency((uint64_t)v); };
      view_manager::push(&view_manager::keypadView);
      return true;
   }

   return false;
}
