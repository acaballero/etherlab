//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_MAIN_VIEW_H
#define TRX_FRONTEND_MAIN_VIEW_H

#include "config.h"
#include "dsp/fft/snr_widget.h"
#include "hw/hw_config.h"
#include "number_edit_view.h"
#include "option_buttons_view.h"
#include "ui/ui_types.h"
#include "view.h"
#include "menu_widget.h"
#include "status_widget.h"
#include "titlebar_widget.h"
#include "tune_widget.h"
#include "message_view.h"
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

class MainView : public View {
  public:
    MainView();

    WaterfallWidget *Waterfall();

    Widget *Spectrum();

    Widget *IQBalance();

    Widget *TuneInfo();

    Widget *FFTInfo();

    Widget *FFT();

    Widget *Menu();

    OptionButtonsView *OptionButtons();

    bool on_input(const st_inputEvent event) override;

  protected:
    TitleBarWidget header_w = {{0, 0, DISPLAY_X_PIXELS / 2 - 50, HEADER_HEIGHT}};
    StatusWidget status_w{{0, DISPLAY_Y_PIXELS - STATUS_HEIGHT, DISPLAY_X_PIXELS, STATUS_HEIGHT}};
    DbScaleWidget dbscale_w{{DISPLAY_X_PIXELS - DBSCALE_WIDTH, HEADER_HEIGHT, DBSCALE_WIDTH, FFT_HEIGHT}, &lcd};
    TuneWidget tune_w{{0, HEADER_HEIGHT + FFT_WIDGET_HEIGHT + FFT_WATERFALL_HEIGHT, METER_WIDTH, TUNE_INFO_HEIGHT}, &lcd};

    RadioStatusWidget radio_w{{METER_WIDTH, HEADER_HEIGHT + FFT_WIDGET_HEIGHT + FFT_WATERFALL_HEIGHT, DISPLAY_X_PIXELS - METER_WIDTH, INFO_HEIGHT}};
    SMeterWidget smeter_w{{0, HEADER_HEIGHT + FFT_WIDGET_HEIGHT + FFT_WATERFALL_HEIGHT, METER_WIDTH, METERS_HEIGHT - 20}, &lcd};
    SNRWidget snr_w{{15, HEADER_HEIGHT + FFT_WIDGET_HEIGHT + FFT_WATERFALL_HEIGHT + METERS_HEIGHT - 5, METER_WIDTH - 15, SNRWidget::height}};
    PowerMeterWidget powmeter_w{{0, HEADER_HEIGHT + FFT_WIDGET_HEIGHT + FFT_WATERFALL_HEIGHT, METER_WIDTH, METERS_HEIGHT}, &lcd};
    InfoWidget info_w{{0, HEADER_HEIGHT + FFT_WIDGET_HEIGHT + FFT_WATERFALL_HEIGHT + TUNE_INFO_HEIGHT, METER_WIDTH, FFT_INFO_HEIGHT}, &lcd};
    FrequencyWidget frequency_w{{DISPLAY_X_PIXELS / 2 - 40, 0, DISPLAY_X_PIXELS / 2 + 40, HEADER_HEIGHT}};
    FFTWidget fft_w{{0, HEADER_HEIGHT, FFT_ZONE_WIDTH, FFT_WIDGET_HEIGHT}, &lcd, config.fft.spectrum_style};
    WaterfallWidget waterfall_w{{0, HEADER_HEIGHT + FFT_WIDGET_HEIGHT, DISPLAY_X_PIXELS, FFT_WATERFALL_HEIGHT}, &lcd};
    IQBalanceWidget iqbal_w{{0, HEADER_HEIGHT + FFT_WIDGET_HEIGHT, DISPLAY_X_PIXELS, FFT_WATERFALL_HEIGHT}, &lcd};
    MenuWidget menu_w{{0, MENU_START_Y, DISPLAY_X_PIXELS, INFO_HEIGHT}, &lcd};
    OptionButtonsView optionButtonsView{{0, HEADER_HEIGHT, DISPLAY_X_PIXELS, OptionButtonsView::HEIGHT}};

    void before_paint() override;

    void on_child_update(Widget *) override;
};

#endif // TRX_FRONTEND_MAIN_VIEW_H
