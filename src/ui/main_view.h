//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_MAIN_VIEW_H
#define TRX_FRONTEND_MAIN_VIEW_H

#include "config.h"
#include "hw/hw_config.h"
#include "view.h"
#include "menu_widget.h"
#include "status_widget.h"
#include "titlebar_widget.h"
#include "tune_widget.h"
#include "message_widget.h"
#include "dbscale_widget.h"
#include "frequency_widget.h"
#include "dsp/fft/info_widget.h"
#include "../dsp/fft/fft_widget.h"
#include "../dsp/fft/iqbalance_widget.h"
#include "../dsp/fft/waterfall_widget.h"
#include "lcd.h"
#include "s_meter_widget.h"
#include "pow_meter_widget.h"
#include "radio_status_widget.h"

#define HEADER_HEIGHT 22
#define STATUS_HEIGHT 18
#define INFO_HEIGHT (DISPLAY_Y_PIXELS - HEADER_HEIGHT * 2 - FFT_WIDGET_HEIGHT - 5 - FFT_WATERFALL_HEIGHT)
#define TUNE_INFO_HEIGHT (INFO_HEIGHT / 3)
#define METERS_HEIGHT INFO_HEIGHT / 2
#define FFT_INFO_HEIGHT (2 * INFO_HEIGHT / 3)
#define METER_WIDTH (int)((float)DISPLAY_X_PIXELS / 1.5f)
#define MENU_START_Y (HEADER_HEIGHT + FFT_WIDGET_HEIGHT + FFT_WATERFALL_HEIGHT + 5)

class MainView : public View {
  public:
    MainView();

    WaterfallWidget *Waterfall();

    Widget *Spectrum();

    Widget *IQBalance();

    Widget *TuneInfo();

    Widget *FFTInfo();

    Widget *FFT();

    Widget *Message();

    Widget *Menu();

    bool on_input(const st_inputEvent event) override;

  protected:
    TitleBarWidget header_w = {{0, 0, DISPLAY_X_PIXELS / 2, HEADER_HEIGHT}, &lcd};
    StatusWidget status_w{{0, DISPLAY_Y_PIXELS - STATUS_HEIGHT, DISPLAY_X_PIXELS, STATUS_HEIGHT}};
    DbScaleWidget dbscale_w{{DISPLAY_X_PIXELS - DBSCALE_WIDTH, HEADER_HEIGHT, DBSCALE_WIDTH, FFT_HEIGHT}, &lcd};
    TuneWidget tune_w{{0, HEADER_HEIGHT + FFT_WIDGET_HEIGHT + FFT_WATERFALL_HEIGHT + 5, METER_WIDTH, TUNE_INFO_HEIGHT}, &lcd};
    RadioStatusWidget radio_w{{METER_WIDTH, HEADER_HEIGHT + FFT_WIDGET_HEIGHT + FFT_WATERFALL_HEIGHT + 8, DISPLAY_X_PIXELS - METER_WIDTH, INFO_HEIGHT}};
    SMeterWidget smeter_w{{0, HEADER_HEIGHT + FFT_WIDGET_HEIGHT + FFT_WATERFALL_HEIGHT + 18, METER_WIDTH, METERS_HEIGHT}, &lcd};
    PowerMeterWidget powmeter_w{{0, HEADER_HEIGHT + FFT_WIDGET_HEIGHT + FFT_WATERFALL_HEIGHT + 18, METER_WIDTH, METERS_HEIGHT}, &lcd};
    InfoWidget info_w{{0, HEADER_HEIGHT + FFT_WIDGET_HEIGHT + FFT_WATERFALL_HEIGHT + 5 + TUNE_INFO_HEIGHT, METER_WIDTH, FFT_INFO_HEIGHT}, &lcd};
    FrequencyWidget frequency_w{{DISPLAY_X_PIXELS / 2, 0, DISPLAY_X_PIXELS / 2, HEADER_HEIGHT}};
    FFTWidget fft_w{{0, HEADER_HEIGHT, FFT_ZONE_WIDTH, FFT_WIDGET_HEIGHT}, &lcd, config.fft.spectrum_style};
    WaterfallWidget waterfall_w{{0, HEADER_HEIGHT + FFT_WIDGET_HEIGHT, DISPLAY_X_PIXELS, FFT_WATERFALL_HEIGHT}, &lcd};
    IQBalanceWidget iqbal_w{{0, HEADER_HEIGHT + FFT_WIDGET_HEIGHT, DISPLAY_X_PIXELS, FFT_WATERFALL_HEIGHT}, &lcd};
    MenuWidget menu_w{{0, MENU_START_Y, DISPLAY_X_PIXELS, INFO_HEIGHT}, &lcd};

    MessageWidget msg_w{
        {6, MENU_START_Y, DISPLAY_X_PIXELS - 12, INFO_HEIGHT - 6}, &lcd, (FontDef *)&Font_11x18, (FontDef *)&Font_7x10, C565_GREY_DARK, C565_RED, C565_WHITE};

    void before_paint() override;
};

#endif // TRX_FRONTEND_MAIN_VIEW_H
