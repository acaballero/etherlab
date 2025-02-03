//
// Created by Angel Dust on 16/06/2021.
//

#include "fft_ui.h"
#include "dsp/dsp_common.h"
#include "fft_types.h"
#include "../../settings.h"
#include "menuBase.h"
#include "ui/main_view.h"
#include "ui/menu_widget.h"
#include "ui/menu.h"
#include "ui/view_manager.h"
#include <sys/_stdint.h>

using namespace Menu;

namespace fftUI {

const char *fftWindowNames[] = {"None", "Hamming"};
const char *spectrumStyleNanes[] = {"Fill", "Line", "Line & Fill"};

uint8_t waterfall_step_size;
uint16_t waterfall_period;

void init_waterfall() {
    uint16_t pps = config.fft.waterfall_pixels_per_second;
    // Calculate the scroll step size considering the minimum refresh period
    waterfall_period = 1000 / pps;
    waterfall_step_size = 1;

    while (waterfall_period < FFT_WATERFALL_MIN_REFRESH_PERIOD_MS && waterfall_step_size < FFT_WATERFALL_MAX_PIXELS_PER_FRAME) {
        waterfall_step_size++;
        waterfall_period = waterfall_step_size * 1000 / pps;
    }

    view_manager::mainView.Waterfall()->set_step(waterfall_step_size);
}

uint16_t get_waterfall_period() { return waterfall_period; }

uint8_t get_waterfall_step_size() { return waterfall_step_size; }

result refresh_waterfall_params(eventMask) {
    init_waterfall();
    return proceed;
}

result set_sampling_params(eventMask) {
    fft_config(config.fft.span);
    return proceed;
}

void change_spectrum_colors() { set_spectrum_colors(config.fft.spectrum_line_color, config.fft.spectrum_fill_color); }

void set_spectrum_style(FFT_SPECTRUM_STYLE style) { ((FFTWidget *)view_manager::mainView.FFT())->set_style(style); }

void set_spectrum_colors(uint16_t line, uint16_t fill) { ((FFTWidget *)view_manager::mainView.FFT())->set_colors(line, fill); }

prompt *windowValues[] = {new Menu::menuValue<FFT_WINDOW_TYPES>(fftWindowNames[FFT_WINDOW_NONE], FFT_WINDOW_NONE, doNothing, noEvent),
                          new Menu::menuValue<FFT_WINDOW_TYPES>(fftWindowNames[FFT_WINDOW_HAMMING], FFT_WINDOW_HAMMING, doNothing, noEvent)};

Menu::menu_option_st<FFT_SPECTRUM_STYLE> spectrum_style_options[] = {{spectrumStyleNanes[FFT_SPECTRUM_STYLE_FILL], FFT_SPECTRUM_STYLE_FILL},
                                                                     {spectrumStyleNanes[FFT_SPECTRUM_STYLE_LINE], FFT_SPECTRUM_STYLE_LINE},
                                                                     {spectrumStyleNanes[FFT_SPECTRUM_STYLE_LINE_FILL], FFT_SPECTRUM_STYLE_LINE_FILL}};

Menu::select<uint8_t> &fftWindowMenu = *new Menu::select<uint8_t>("Window", config.fft.window, sizeof(windowValues) / sizeof(prompt *), windowValues);

prompt *fftViewValues[] = {new Menu::menuValue<FFT_VIEW_MODE>("Spectrum", FFT_VIEW_SPECTRUM, doNothing, noEvent),
                           new Menu::menuValue<FFT_VIEW_MODE>("Time domain", FFT_VIEW_TIME_DOMAIN, doNothing, noEvent)};

Menu::select<uint8_t> &fftViewMenu = *new Menu::select<uint8_t>("View", config.fft.view_mode, sizeof(fftViewValues) / sizeof(prompt *), fftViewValues);

Menu::optionsPrompt<FFT_SPECTRUM_STYLE> fftStyleMenu((const char *)"Style", spectrum_style_options, config.fft.spectrum_style,
                                                     sizeof(spectrum_style_options) / sizeof(spectrum_style_options[0]),
                                                     [](FFT_SPECTRUM_STYLE s) { set_spectrum_style(s); });

Menu::optionsPrompt<uint16_t> fillColorMenu((const char *)"Fill color", Menu::color_options, config.fft.spectrum_fill_color,
                                            sizeof(Menu::color_options) / sizeof(Menu::color_options[0]), [](uint16_t) { change_spectrum_colors(); });

Menu::optionsPrompt<uint16_t> lineColorMenu((const char *)"Line color", Menu::color_options, config.fft.spectrum_line_color,
                                            sizeof(Menu::color_options) / sizeof(Menu::color_options[0]), [](uint16_t) { change_spectrum_colors(); });

TOGGLE(config.fft.enable_iq_balance, setIQBalance, "Enable: ", doNothing, noEvent,
       noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent));

TOGGLE(config.fft.view_IQBalance, showIQBalance, "Show: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent));

MENU(menuIQBalance, "IQ Balance", doNothing, anyEvent, noStyle, SUBMENU(setIQBalance), SUBMENU(showIQBalance),
     FIELD(config.fft.iq_balance_estimate_period_ms, "IQ bal. estimate period", "ms.", 0, 255, 10, 0, doNothing, noEvent, wrapStyle),
     OP("Reset", resetIQBalancer, enterEvent), EXIT("<Back"));

TOGGLE(config.fft.enabled, setEnableFFT, "Enabled: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent));

TOGGLE(config.fft.removeDC, fftRemoveDC, "Remove DC: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent));

result changeAutoDbScale(eventMask e) {
    if (config.fft.min_db > config.fft.max_db) {
        config.fft.min_db = config.fft.max_db;
    }
    return proceed;
}

result changeMinDB(eventMask e) { // constraint min2 and max2 db values
    if (config.fft.min_db > config.fft.max_db) {
        config.fft.min_db = config.fft.max_db;
    }
    return proceed;
}

TOGGLE(fft_min_db_auto, autoMinDbToggle, "Auto dB scale ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent));

prompt *decimationValues[] = {new Menu::menuValue<uint8_t>("1", 1), new Menu::menuValue<uint8_t>("2", 2), new Menu::menuValue<uint8_t>("4", 4),
                              new Menu::menuValue<uint8_t>("8", 8)

};

Menu::select<uint8_t> &decimationMenu =
    *new Menu::select<uint8_t>("Decimation", config.fft.max_decimation_factor, sizeof(decimationValues) / sizeof(prompt *), decimationValues);

Menu::numberPrompt<uint32_t> maxSampleRateMenu((const char *)"Max sample rate", &config.fft.max_sample_rate, 0, ' ', '.', "Hz", doNothing, FFT_MIN_SAMPLE_RATE,
                                               ADC_MAX_SAMPLE_RATE, 10000, 25000);
Menu::numberPrompt<uint32_t> maxDSPSampleRateMenu((const char *)"DSP min sample rate", &config.fft.dsp_max_sample_rate, 0, ' ', '.', "Hz", set_sampling_params,
                                                  FFT_MIN_SAMPLE_RATE, ADC_MAX_SAMPLE_RATE, 10000, 25000);
Menu::numberPrompt<uint32_t> spanMenu((const char *)"Span", &config.fft.span, 0, ' ', '.', "Hz", set_sampling_params, FFT_MIN_SPAN, FFT_MAX_SPAN, 10000, 25000);

MENU(fftSamplingMenu, "Sampling", doNothing, anyEvent, noStyle, OBJ(maxSampleRateMenu), OBJ(maxDSPSampleRateMenu), OBJ(spanMenu))

MENU(fftUIMenu, "Style", doNothing, anyEvent, noStyle, OBJ(fftStyleMenu), OBJ(lineColorMenu), OBJ(fillColorMenu),
     FIELD(config.fft.waterfall_pixels_per_second, "Waterfall speed", "pps", 2,
           (1000 / FFT_WATERFALL_MIN_REFRESH_PERIOD_MS) * FFT_WATERFALL_MAX_PIXELS_PER_FRAME, 2, 0, refresh_waterfall_params, anyEvent, noStyle),
     EXIT("<Back"));

MENU(fftMenu, "Spectrum", doNothing, anyEvent, noStyle, SUBMENU(setEnableFFT),
     FIELD(config.fft.max_slices, "Slices", "", 1, FFT_MAX_SLICES, 1, 0, doNothing, noEvent, noStyle), SUBMENU(decimationMenu), SUBMENU(fftSamplingMenu),
     altFIELD(decPlaces<1>::menuField, config.fft.smooth_factor, "Smooth", " ", 0, 1, 0.1, 0, fftInit, exitEvent, noStyle), SUBMENU(fftWindowMenu),
     SUBMENU(fftUIMenu), SUBMENU(fftViewMenu), SUBMENU(fftRemoveDC), SUBMENU(menuIQBalance), SUBMENU(autoMinDbToggle),
     FIELD(config.fft.max_db, "DB Max", "dB", FFT_MIN_DB, FFT_MAX_DB, 1, 0, changeMinDB, exitEvent, noStyle),
     FIELD(config.fft.min_db, "DB Min", "dB", FFT_MIN_DB, FFT_MAX_DB, 1, 0, changeMinDB, exitEvent, noStyle),
     FIELD(fft_calc_noise_floor_period_ms, "Noise floor calc period", "ms", 0, 1000, 10, 0, doNothing, exitEvent, noStyle),
     FIELD(config.fft.maxAmpl, "Amplitude", "", 0x00FF, 0xFFFF, 10, 0, calcFFTRange, exitEvent, noStyle),
     FIELD(config.f_correction, "Freq. Correction", "kHz.", 0, 100000, 10, 1, doNothing, noEvent, noStyle), EXIT("<Back"));

// TOGGLE(fft_show_noise_floor, showNoiseFloorToggle, "Enable", doNothing, noEvent, noStyle//,doExit,enterEvent,noStyle
//, VALUE("Yes", true, doNothing, noEvent), VALUE("No", false, doNothing, noEvent)
//);

// menuField<int16_t> &minDbField = *new menuField<int16_t>(config.fft.min_db,"DB Min","dB",FFT_MIN_DB,FFT_MAX_DB,1,0,doNothing,noEvent);

} // namespace fftUI
