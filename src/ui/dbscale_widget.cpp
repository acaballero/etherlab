//
// Created by Angel Dust on 17/04/2021.
//

#include "dbscale_widget.h"
#include "config.h"
#include "view.h"

void DbScaleWidget::paint_callback() {

    int maxNticks = 6;
    int step = 0;
    int min = config.fft.min_db;
    int max = config.fft.max_db;
    int db_amp = 0;
    int py = 0;
    int db;
    char buf[5];
    int nTicks = 0;
    uint16_t h = this->size().height();
    display->setFont((FontDef *)&Font_Micro4x6);
    // Find the minimum step to get maxNticks

    step = 0;
    do {

        // step += step<10 ? 5 : 10;
        step += 5;
        nTicks = (max - min) / step;

    } while (nTicks > maxNticks);

    min =
        min + (((9 - (min % step)) + 1) % step); // Next multiple of 10 starting from the minimum. Slower than the commented one, but works for negative numbers
    max = (max / step) * step;                   // Previous multiple of 10 of the maximum of the scale

    db_amp = config.fft.max_db - config.fft.min_db;

    display->clear();

    for (db = min; db < max; db += step) {

        py = h - (uint8_t)(((float)(db - config.fft.min_db) / (float)db_amp) * (float)h);

        sprintf(buf, "%d", db);
        int px = area.box.width - (strlen(buf) * display->getFont()->width) - 1;
        display->gotoXY(px, py - 3); // -3 to center vertically center the text
        display->write(buf);
    }
}

void DbScaleWidget::before_paint() {

    st_scale scale = {config.fft.max_db, config.fft.min_db};

    if (this->dirty() || !(this->current_scale == scale)) { // draw only if needed

        this->current_scale = scale;

        display->setColor(C565_GREY_LIGHT);
        display->setBgColor(C565_TRANSPARENT);

        this->set_dirty();
    }
}
