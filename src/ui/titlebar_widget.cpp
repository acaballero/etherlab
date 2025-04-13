//
// Created by Angel Dust on 17/04/2021.
//

#include <scanner.h>
#include <rf_coupler.h>
#include <stdint.h>
#include "Display_afb.h"
#include "dsp/dsp_common.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "stm32f4xx_hal.h"
#include "titlebar_widget.h"
#include "config.h"
#include "battery.h"
#include "power_amp.h"
#include "fatfs/fatfs.h"
#include "main_board.h"
#include "utils.hpp"

TitleBarWidgetInner::TitleBarWidgetInner(const Rect &parentRect, Display *display) : Widget(parentRect, display) {
    sdcard_signal.add(this, TitleBarWidgetInner::signal_static_callback);
    battery::battery_signal.add(this, TitleBarWidgetInner::signal_static_callback);
    power_amp::temp_signal.add(this, TitleBarWidgetInner::signal_static_callback);
    rf_coupler::rf_coupler_signal.add(this, TitleBarWidgetInner::signal_static_callback);
    rtc_signal.add(this, TitleBarWidgetInner::signal_static_callback);
}

void TitleBarWidgetInner::paint_callback() {

    FontDef *font = (FontDef *)&Font_Tiny8x8;

    uint16_t color = C565_BLACK;
    char buff[20];

    display->clear();

    display->gotoXY(0, MARGIN);

    display->setColor(C565_WHITE);
    display->setBgColor(C565_BLACK);
    display->setFont(font);

#if ENABLE_RTC
    st_datetime datetime = rtc_get_date_time();

    // sprintf(buff, "%02d/%02d %02d:%02d  ",date.Month,date.Date,time.Hours,time.Minutes);
    sprintf(buff, "%02d:%02d ", datetime.time.Hours, datetime.time.Minutes);
    display->print(buff);
#else
    display->print("TRX_100");
#endif

    if (battery::battery_info.status != battery::BATTERY_STATUS_UNDEFINED) {

        char c;

        switch (battery::battery_info.status) {
            case battery::BATTERY_STATUS_VERY_LOW:
                color = C565_RED;
                c = ICON_BATT_LOW;
                break;
            case battery::BATTERY_STATUS_LOW:
                color = C565_RED;
                c = ICON_BATT_MID;
                break;
            case battery::BATTERY_STATUS_MEDIUM:
                color = C565_GREENYELLOW;
                c = ICON_BATT_MID;
                break;
            case battery::BATTERY_STATUS_HIGH:
                color = C565_GREEN;
                c = ICON_BATT_FULL;
                break;
            case battery::BATTERY_STATUS_CHARGING:
                color = C565_MAGENTA;
                c = ICON_BATT_CHARGING;
                break;
            default:
            case battery::BATTERY_STATUS_UNDEFINED:
                c = ICON_BATT_MID;
                break;
        }

        display->setFont((FontDef *)&Font_Icons9x8);
        display->setColor(color);
        display->writeChar(c);
        display->setFont((FontDef *)&Font_Tiny8x8);
        display->setColor(C565_WHITE);
        // display->print(battery_info.voltage, 2);
        display->print(" ");
    }

    display->setFont((FontDef *)&Font_Icons9x8);

#if ENABLE_SD_CARD
    switch (sdcard_info.status) {
        case sdcard_STATUS::MountError:
            color = C565_YELLOW;
            break;
        case sdcard_STATUS::IOError:
            color = C565_RED;
            break;
        case sdcard_STATUS::Present:
            color = C565_WHITE;
            break;
        case sdcard_STATUS::NotPresent:
            color = C565_GREY_DARK;
            break;
        case sdcard_STATUS::Mounted:
            color = C565_GREEN;
            break;
    }

    display->setColor(color);
    display->writeChar(ICON_SD_CARD);
#else
    display->setColor(C565_GREY_DARKER);
    display->writeChar(ICON_SD_CARD);
#endif

    display->setFont((FontDef *)&Font_Tiny8x8);
    display->print(" ");
    display->setFont((FontDef *)&Font_Icons9x8);

#if USB_ENABLED
    switch (getConnectionStatus()) {
        case USB_CONN_STATUS_CONNECTED:
            color = C565_GREEN;
            break;
        case USB_CONN_STATUS_DISCONNECTED:
            color = C565_GREY_DARKER;
            break;
    }

    display->setColor(color);
    display->writeChar(ICON_USB);
#else
    display->setColor(C565_GREY_DARKER);
    display->writeChar(ICON_USB);
#endif

    display->setFont((FontDef *)&Font_Tiny8x8);

    if (power_amp::temp >= power_amp::params.MIN_TEMP) {

        if (power_amp::temp < power_amp::params.TEMP_LOW_THRESHOLD) {
            color = C565_GREEN;
        } else if (power_amp::temp > power_amp::params.TEMP_HIGH_THRESHOLD) {
            color = C565_RED;
        } else {
            color = C565_YELLOW;
        }

        display->setColor(color);
        display->print(" ");
        display->print(power_amp::temp);
        display->print("`"); // º is mapped to '
        display->setColor(C565_WHITE);
    }

    // if (ISTX) {
    //     if (rf_coupler::info.swr > 0) {

    //         if (rf_coupler::info.swr >= rf_coupler::HIGH_SWR) {
    //             display->setColor(C565_RED);
    //         }

    //         if (rf_coupler::info.swr >= rf_coupler::MAX_SWR) {
    //             sprintf(buff, " S:MAX");
    //         } else if (rf_coupler::info.swr > 0) {
    //             sprintf(buff, " S:%.1f", rf_coupler::info.swr);
    //         } else {
    //             sprintf(buff, " S:?", rf_coupler::info.swr);
    //         }
    //     } else {
    //         display->setColor(C565_GREY_LIGHT);
    //         sprintf(buff, " S:?");
    //     }
    //     display->print(buff);
    //     display->setColor(C565_WHITE);
    // }

    // TODO: GPSDO lock. Meanwhile, warmup time has passed
    uint32_t uptime = rtc_uptime();
    if (uptime > 8 * 60) {
        display->print(" G");
    }

    if (!ISTX) {
        // AUDIO

        if (main_board::getMute()) {
            display->setColor(C565_GREY_DARKER);
        } else {
            display->setColor(C565_WHITE);
        }
        display->print(" ");
        display->setFont((FontDef *)&Font_Icons9x8);
        display->writeChar(main_board::getMute() ? ICON_SOUND_OFF : ICON_SOUND_ON);
    }

    display->set_trim_enabled(true);
}

void TitleBarWidgetInner::before_paint() {

    st_topBar topBar = {main_board::getMute() ? true : false};

    if (this->dirty() || !(topBar == this->status)) {
        this->status = topBar;
        this->set_dirty();
    }
}

void TitleBarWidgetInner::on_info_changed_signal(void *) {
    ;
    this->set_dirty();
}

void TitleBarWidget::init() {

    btnDSP.action = [this](Button &, st_inputEvent) {
        main_board::toggle_dsp();
        set_dirty();
    };

    add_children({&titleBarWidgetInner, &btnDSP});
    set_name("tit_w");

    for (Widget *btn : View::children()) {
        btn->set_font((FontDef *)&Font_Fixed5x7);
        btn->set_aling(ALIGN_CENTER);
        ((Button *)btn)->set_style(ButtonStyle::BUTTON_STYLE_FLAT);
        ((Button *)btn)->set_bg(C565_VIOLET);
        ((Button *)btn)->set_fg(C565_WHITE);
    }

    // Update every rtc update event
    rtc_signal.add(this, [this](void *, void *) { set_dirty(); });
}

void TitleBarWidget::before_paint() {

    if (dirty()) {
        uint16_t color = C565_BLACK;

        color = C565_GREY_LIGHT;

        if (ISANALOG) {

            btnDSP.set_text("ANA");

        } else {

            float drop_freq = dsp_status && dsp_status->status == DSP_STATUS_RUNNING ? dsp_status->drop_rate() : 0;
            float starve_freq = dsp_status && dsp_status->status == DSP_STATUS_RUNNING ? dsp_status->starve_rate() : 0;

            bool error = true;
            if (!dsp_status || dsp_status->error != DSP_ERR_NONE || drop_freq * 100 > 2 || starve_freq * 100 > 2) {
                color = C565_RED;
            } else if (drop_freq * 100 > 1 || starve_freq * 100 > 1) {
                color = C565_YELLOW;
            } else {
                error = false;
            }
            dsp_status->reset();
            char buf[20];
            sprintf(buf, "%s%s %.1f %.1f ", "DSP", error ? "!" : "", error ? drop_freq : 0, error ? starve_freq : 0);
            trim(buf);
            btnDSP.set_text(buf);
        }

        btnDSP.set_fg(color);
    }
}
