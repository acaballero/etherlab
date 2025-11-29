//
// Created by Angel Dust on 17/04/2021.
//

#include <scanner.h>
#include <rf_coupler.h>
#include <stdint.h>
#include "Display_afb.h"
#include "dsp/dsp.h"
#include "dsp/dsp_common.h"
#include "hw/stm32f4xx/rtc.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "stm32f4xx_hal.h"
#include "titlebar_widget.h"
#include "config.h"
#include "battery.h"
#include "power_amp.h"
#include "fatfs/fatfs.h"
#include "main_board.h"
#include "types.h"
#include "utils.hpp"
#include "status.h"

TitleBarWidgetInner::TitleBarWidgetInner(const Rect &parentRect, Display *display) : Widget(parentRect, display) {
    sdcard_signal.add(this, TitleBarWidgetInner::signal_static_callback);
    battery::battery_signal.add(this, TitleBarWidgetInner::signal_static_callback);
    power_amp::temp_signal.add(this, [this](void *, const void *) {
        set_dirty();
    });
    rf_coupler::rf_coupler_signal.add(this, TitleBarWidgetInner::signal_static_callback);
    rtc_signal.add(this, [this](void *, const void *) {
        // Repaing every 5 seconds
        static int i;
        if (i++ % 5 == 0) {
            set_dirty();
        }
    });
    main_board::mode_signal.add(this, TitleBarWidgetInner::signal_static_callback);
}

bool TitleBarWidgetInner::paint_callback() {

    FontDef *font = (FontDef *)&Font_Tiny8x8;

    Color color = C565_GREY_LIGHT;
    char buff[20];

    display->clear();
    display->gotoXY(0, MARGIN);
    display->setColor(C565_WHITE);
    display->setBgColor(C565_BLACK);
    display->setFont(font);

#if ENABLE_RTC
    st_datetime datetime = rtc_get_date_time();
    uint32_t current_epoch = rtc_to_epoch(&datetime.time, &datetime.date);
    if (current_epoch != last_epoch) {
        last_epoch = current_epoch;
    } else {
        // The clock is not ticking
        color = C565_GREY_DARK;
        datetime = {};
    }

    sprintf(buff, "%02d:%02d ", datetime.time.Hours, datetime.time.Minutes);
    // sprintf(buff, "%02d/%02d %02d:%02d  ",date.Month,date.Date,time.Hours,time.Minutes);

    display->setColor(color);
    display->print(buff);
#else
    display->print("TRX_100");
#endif

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
            color = C565_GREY_DARK;
            c = ICON_BATT_LOW;
            break;
    }

    display->setFont((FontDef *)&Font_Icons9x8);
    display->setColor(color);
    display->writeChar(c);
    display->setFont((FontDef *)&Font_Tiny8x8);
    display->setColor(C565_WHITE);
    // display->print(battery_info.voltage, 2);
    display->print(" ");

    display->setFont((FontDef *)&Font_Icons9x8);

#if ENABLE_SD_CARD

    bool check_space = false;

    switch (sdcard_info.status) {
        case sdcard_STATUS::MountError:
            color = C565_GREY_DARKER;
            break;
        case sdcard_STATUS::IOError:
        case sdcard_STATUS::ConnectError:
            color = C565_RED;
            break;
        case sdcard_STATUS::Present:
            color = C565_WHITE;
            break;
        case sdcard_STATUS::NotPresent:
            color = C565_GREY_DARK;
            break;
        case sdcard_STATUS::Mounted:
            if (usb_msc_active) {
                // Mounted but MSC is active and waiting for the USB host
                color = C565_PURPLE;
            } else {
                color = C565_GREEN;
            }
            break;
        case sdcard_STATUS::MassStorageDeviceActive:
            color = C565_PURPLE;
            break;
    }

    display->setColor(color);
    display->writeChar(ICON_SD_CARD);

    if (check_space) {
        sdcard_st_info sd_info = sdcard::get_info();

        if ((float)sd_info.free_bytes / sd_info.total_bytes < 0.1 || sd_info.free_bytes < 100000) {
            display->setFont((FontDef *)&Font_Tiny8x8);
            display->writeChar('!');
        }
    }

#else
    display->setColor(C565_GREY_DARKER);
    display->writeChar(ICON_SD_CARD);
#endif

    display->setFont((FontDef *)&Font_Tiny8x8);
    display->print(" ");
    display->setFont((FontDef *)&Font_Icons9x8);

#if USB_ENABLED
    switch (getUSBConnectionStatus()) {
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

    // TODO: GPSDO lock. Meanwhile, warmup time has passed

    uint32_t uptime = rtc_uptime();

    display->setColor(uptime > 8 * 60 ? C565_GREEN : C565_GREY_DARK);
    display->print(" G");

    if (!ISTX) {
        // AUDIO

        if (main_board::get_mute()) {
            display->setColor(C565_GREY_DARKER);
        } else {
            display->setColor(C565_WHITE);
        }
        display->print(" ");
        display->setFont((FontDef *)&Font_Icons9x8);
        display->writeChar(main_board::get_mute() ? ICON_SOUND_OFF : ICON_SOUND_ON);
    }

    return true;
}

void TitleBarWidgetInner::before_paint() {

    st_topBar topBar = {main_board::get_mute() ? true : false};

    if (this->dirty() || !(topBar == this->status)) {
        this->status = topBar;
        this->set_dirty();
    }
}

void TitleBarWidgetInner::on_info_changed_signal(const void *) {
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
        btn->set_font((FontDef *)&Font_7x10);
        btn->set_aling(ALIGN_CENTER);
        ((Button *)btn)->set_style(ButtonStyle::BUTTON_STYLE_FLAT);
        ((Button *)btn)->set_bg(C565_VIOLET);
        ((Button *)btn)->set_fg(C565_WHITE);
    }

    // Update every mode update event
    main_board::mode_signal.add(this, [this](void *, const void *) {
        set_dirty();
    });

    // And every clock tick, so the DSP status label is regularly updated
    rtc_signal.add(this, [this](void *, const void *) {
        set_dirty();
    });
}

void TitleBarWidget::before_paint() {

    if (dirty()) {

        uint16_t color = C565_BLACK;

        color = C565_GREY_LIGHT;

        display->set_padding(8, 8);
        btnDSP.set_bg(ISTX ? C565_RED : C565_VIOLET);

        if (ISANALOG) {
            btnDSP.set_text("ANA");
        } else {
            float drop_freq = dsp::dsp_params && dsp::dsp_params->status == DSP_STATUS_RUNNING ? dsp::dsp_params->drop_rate() : 0;
            float starve_freq = dsp::dsp_params && dsp::dsp_params->status == DSP_STATUS_RUNNING ? dsp::dsp_params->starve_rate() : 0;
            bool error = true;

            if (!dsp::dsp_params) {
                color = C565_GREY_DARKER;
                error = false;
            } else if (dsp::dsp_params->error != DSP_ERR_NONE || drop_freq * 100 > 1 || starve_freq * 100 > 1) {
                color = C565_ORANGE;
            } else if (drop_freq * 100 > 0.1 || starve_freq * 100 > 0.1) {
                color = C565_YELLOW;
            } else {
                error = false;
            }

            dsp::dsp_params->reset();
            char buf[20];
            MODULATION_MODE mod = main_board::get_modulation_mode();
            bool space = dsp::apply_compression(mod) || dsp::apply_deemph(mod) || dsp::apply_audio_bpf() || dsp::dsp_config.baseband_echo;
            sprintf(buf, "%s%s%s%s%s%s%s", "DSP", space ? " " : "", dsp::apply_compression(mod) ? "C" : "", dsp::apply_deemph(mod) ? "D" : "",
                    dsp::apply_audio_bpf() ? "F" : "", dsp::dsp_config.baseband_echo ? "E" : "", error ? " !" : "");
            trim(buf);
            btnDSP.set_text(buf);
        }

        btnDSP.set_fg(color);
    }
}
