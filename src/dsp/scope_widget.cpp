//
// Created by Angel Dust on 19/04/2021.
//

#include "scope_widget.h"

void ScopeWidget::paint_callback() {

    /* lcd.clear();

   int height = FFT_WATERFALL_HEIGHT / 2;
   // double halfAmp = config.fft.maxAmpl >> 1;

   for (uint16_t i = 0; i < FFT_N; i++) {


       uint8_t y = start_y + height / 2 +
                   (int) (((vData[i].r) / (float) config.fft.maxAmpl) * (float) height);
       uint8_t y2 = start_y + height + height / 2 +
                    (int) (((vData[i].i) / (float) config.fft.maxAmpl) * (float) height);

#if LCD_ENABLED

       lcd.writeLine(i, y, i, y);
       lcd.writeLine(i, y2, i, y2);
#endif

   }*/

}

void ScopeWidget::do_paint() {
    display->drawArea(&this->area, this);
}
