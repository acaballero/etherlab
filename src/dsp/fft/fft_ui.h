//
// Created by Angel Dust on 16/06/2021.
//

#ifndef TRX_FRONTEND_FFT_UI_H
#define TRX_FRONTEND_FFT_UI_H

#include "../../../lib/Menu/src/menu.h"
#include "fft_types.h"

namespace fftUI {
extern Menu::menu fftMenu;
extern const char *fftWindowNames[];
void set_spectrum_style(FFT_SPECTRUM_STYLE style);
void set_spectrum_colors(uint16_t line, uint16_t fill);
uint16_t get_waterfall_period();
uint8_t get_waterfall_step_size();
void init_waterfall();
void initIQorWaterfall();
void open_waterfall_config();
void open_dbscale_config();
void open_span_config();
} // namespace fftUI

#endif // TRX_FRONTEND_FFT_UI_H
