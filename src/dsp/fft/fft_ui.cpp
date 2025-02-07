//
// Created by Angel Dust on 16/06/2021.
//

#include "fft_ui.h"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft.h"
#include "fft_types.h"
#include "../../settings.h"
#include "menuBase.h"
#include "ui/main_view.h"
#include "ui/menu_widget.h"
#include "ui/menu.h"
#include "ui/menu_prompts.h"
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

Menu::numberPrompt<uint8_t> iqBalancePeriodMenu((const char *)"IQ bal. estimate period", &config.fft.iq_balance_estimate_period_ms, 0, ' ', '.', "ms", nullptr,
                                                0, 255, 5, 10);

TOGGLE(config.fft.enable_iq_balance, setIQBalance, "Enable: ", doNothing, noEvent,
       noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent));

TOGGLE(config.fft.view_IQBalance, showIQBalance, "Show: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent));

MENU(menuIQBalance, "IQ Balance", doNothing, anyEvent, noStyle, SUBMENU(setIQBalance), SUBMENU(showIQBalance), OBJ(iqBalancePeriodMenu),
     OP("Reset", resetIQBalancer, enterEvent), EXIT("<Back"));

TOGGLE(config.fft.enabled, setEnableFFT, "Enabled: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent));

TOGGLE(config.fft.removeDC, fftRemoveDC, "Remove DC: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent));

TOGGLE(fft_min_db_auto, autoMinDbToggle, "Auto dB scale ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent));

menu_option_st<uint8_t> decimation_options[] = {{"1", 1}, {"2", 2}, {"4", 4}, {"8", 8}};

optionsPrompt<uint8_t> decimationMenu((const char *)"Max decimation", decimation_options, config.fft.max_decimation_factor,
                                      sizeof(decimation_options) / sizeof(decimation_options[0]));

Menu::numberPrompt<uint32_t> maxSampleRateMenu((const char *)"Max sample rate", &config.fft.max_sample_rate, 0, ' ', '.', "Hz", nullptr, FFT_MIN_SAMPLE_RATE,
                                               ADC_MAX_SAMPLE_RATE, 10000, 25000);
Menu::numberPrompt<uint32_t> maxDSPSampleRateMenu((const char *)"DSP min sample rate", &config.fft.dsp_max_sample_rate, 0, ' ', '.', "Hz",
                                                  [](uint32_t v) { fft_config(v); }, FFT_MIN_SAMPLE_RATE, ADC_MAX_SAMPLE_RATE, 10000, 25000);
Menu::numberPrompt<uint32_t> spanMenu((const char *)"Span", &config.fft.span, 0, ' ', '.', "Hz", [](uint32_t v) { fft_config(v); }, FFT_MIN_SPAN, FFT_MAX_SPAN,
                                      10000, 25000);
Menu::numberPrompt<float> smoothMenu((const char *)"Smooth", &config.fft.smooth_factor, 1, ' ', '.', "", [](float) { fftInit(); }, 0, 1, 0.1, 1);

Menu::numberPrompt<uint16_t> waterfallSpeedMenu((const char *)"Waterfall speed", &config.fft.waterfall_pixels_per_second, 0, ' ', '.', "pps",
                                                [](uint16_t) { fftInit(); }, 2,
                                                (1000 / FFT_WATERFALL_MIN_REFRESH_PERIOD_MS) * FFT_WATERFALL_MAX_PIXELS_PER_FRAME, 2, 5);

Menu::numberPrompt<int16_t> minDbMenu((const char *)"DB Min", &config.fft.min_db, 0, ' ', '.', "dB", nullptr, FFT_MIN_DB, FFT_MAX_DB, 1, 5);

Menu::numberPrompt<int16_t> maxDbMenu((const char *)"DB Max", &config.fft.max_db, 0, ' ', '.', "dB", nullptr, FFT_MIN_DB, FFT_MAX_DB, 1, 5);

Menu::numberPrompt<uint16_t> fftCalcNoisePeriodMenu((const char *)"Noise floor calc period", &fft_calc_noise_floor_period_ms, 0, ' ', '.', "ms", nullptr, 0,
                                                    1000, 10, 100);

Menu::numberPrompt<int> amplitudeMenu((const char *)"Amplitude", &config.fft.maxAmpl, 0, ' ', '.', "", nullptr, 0x00FF, 0xFFFF, 10, 100);

Menu::numberPrompt<int32_t> fCorrectionMenu((const char *)"Freq. correction", &config.f_correction, 0, ' ', '.', "kHz", nullptr, 0, 100000, 10, 100);

MENU(fftSamplingMenu, "Sampling", doNothing, anyEvent, noStyle, OBJ(maxSampleRateMenu), OBJ(maxDSPSampleRateMenu), OBJ(spanMenu))

MENU(fftUIMenu, "Style", doNothing, anyEvent, noStyle, OBJ(fftStyleMenu), OBJ(lineColorMenu), OBJ(fillColorMenu), OBJ(waterfallSpeedMenu));

MENU(fftMenu, "Spectrum", doNothing, anyEvent, noStyle, SUBMENU(setEnableFFT),
     FIELD(config.fft.max_slices, "Slices", "", 1, FFT_MAX_SLICES, 1, 0, doNothing, noEvent, noStyle), OBJ(decimationMenu), SUBMENU(fftSamplingMenu),
     OBJ(smoothMenu), SUBMENU(fftWindowMenu), SUBMENU(fftUIMenu), SUBMENU(fftViewMenu), SUBMENU(fftRemoveDC), SUBMENU(menuIQBalance), SUBMENU(autoMinDbToggle),
     OBJ(minDbMenu), OBJ(maxDbMenu), OBJ(fftCalcNoisePeriodMenu), OBJ(amplitudeMenu), OBJ(fCorrectionMenu));

// TOGGLE(fft_show_noise_floor, showNoiseFloorToggle, "Enable", doNothing, noEvent, noStyle//,doExit,enterEvent,noStyle
//, VALUE("Yes", true, doNothing, noEvent), VALUE("No", false, doNothing, noEvent)
//);

// menuField<int16_t> &minDbField = *new menuField<int16_t>(config.fft.min_db,"DB Min","dB",FFT_MIN_DB,FFT_MAX_DB,1,0,doNothing,noEvent);

} // namespace fftUI
