//
// Created by Angel Dust on 17/04/2021.
//

#include <scanner.h>
#include <rf_coupler.h>
#include <stdint.h>
#include "Display_afb.h"
#include "ips_font.h"
#include "titlebar_widget.h"
#include "config.h"
#include "battery.h"
#include "power_amp.h"
#include "fatfs/fatfs.h"
#include "main_board.h"

TitleBarWidget::TitleBarWidget(const Rect &parentRect, Display *display) : Widget(parentRect, display) {
    sdcard_signal.add(this, TitleBarWidget::signal_static_callback);
    battery::battery_signal.add(this, TitleBarWidget::signal_static_callback);
    power_amp::temp_signal.add(this, TitleBarWidget::signal_static_callback);
    rf_coupler::rf_coupler_signal.add(this, TitleBarWidget::signal_static_callback);
    rtc_signal.add(this, TitleBarWidget::signal_static_callback);
}

void TitleBarWidget::paint_callback() {

    FontDef *font = (FontDef *)&Font_Tiny8x8;
    uint8_t margin = (area.box.height - font->height) / 2;
    uint16_t color = C565_BLACK;
    char buff[20];

    display->clear();

    display->gotoXY(0, margin);

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

    display->set_trim_enabled(false);
    if (battery::battery_info.status != battery::BATTERY_STATUS_UNDEFINED) {

        char c;

        switch (battery::battery_info.status) {
            case battery::BATTERY_STATUS_LOW:
                color = C565_RED;
                c = ICON_BATT_LOW;
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
            color = C565_GREY_DARK;
            break;
    }

    display->setColor(color);
    display->writeChar(ICON_USB);
#else
    display->setColor(C565_GREY_DARKER);
    display->writeChar(ICON_USB);
#endif

    display->setColor(C565_GREY_LIGHT);
    display->setFont((FontDef *)&Font_Tiny8x8);
    display->print(" ");
    display->setFont((FontDef *)&Font_Icons9x8);

    color = C565_GREY_LIGHT;
    if (ISANALOG) {
        display->setColor(color);
        display->writeChar(ICON_ANALOG);
    } else {

        if (dsp_status->error != DSP_ERR_NONE) {
            color = C565_RED;
        } else if (dsp_status->status == DSP_STATUS_RUNNING) {
            color = C565_GREEN;
        }

        display->setColor(color);
        display->writeChar(ICON_DIGITAL);
    }

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
            display->setColor(C565_GREY_DARK);
        }
        display->print(" ");
        display->setFont((FontDef *)&Font_Icons9x8);
        display->writeChar(main_board::getMute() ? ICON_SOUND_OFF : ICON_SOUND_ON);
    }

    display->set_trim_enabled(true);
}

void TitleBarWidget::before_paint() {

    st_topBar topBar = {dsp_status, main_board::getMute() ? true : false};

    if (this->dirty() || !(topBar == this->status)) {
        this->status = topBar;
        this->set_dirty();
    }
}

void TitleBarWidget::on_info_changed_signal(void *) {
    ;
    this->set_dirty();
}
