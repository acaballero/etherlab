//
// Created by Angel Dust on 03/01/2026.
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
#include "titlebar_icons_widget.h"
#include "config.h"
#include "battery.h"
#include "power_amp.h"
#include "fatfs/fatfs.h"
#include "main_board.h"
#include "types.h"
#include "utils.hpp"
#include "status.h"
#include "os/task_manager.h"

TitleBarIconsWidget::TitleBarIconsWidget(const Rect &parentRect, Display *display) : Widget(parentRect, display) {
    sdcard_signal.add(this, TitleBarIconsWidget::signal_static_callback);
    battery::battery_signal.add(this, TitleBarIconsWidget::signal_static_callback);
    power_amp::temp_signal.add(this, TitleBarIconsWidget::signal_static_callback);
    power_amp::status_signal.add(this, TitleBarIconsWidget::signal_static_callback);
    rf_coupler::rf_coupler_signal.add(this, TitleBarIconsWidget::signal_static_callback);
    rtc_signal.add(this, [this](void *, const void *) {
        // Repaing every 5 seconds
        static int i;
        if (i++ % 5 == 0) {
            set_dirty();
        }
    });
    main_board::mode_signal.add(this, TitleBarIconsWidget::signal_static_callback);
}

bool TitleBarIconsWidget::paint_callback() {

    FontDef *font = (FontDef *)&Font_Tiny8x8;

    Color color = C565_GREY_LIGHT;
    char buff[20];

    display->set_trim_enabled(true);
    display->clear();
    display->gotoXY(0, MARGIN);
    display->setColor(C565_WHITE);
    display->setBgColor(C565_BLACK);
    display->setFont(font);

#if ENABLE_RTC

    color = is_rtc_ok() ? C565_WHITE : C565_GREY_DARK;

    sprintf(buff, "%02d:%02d ", datetime.time.Hours, datetime.time.Minutes);

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

    if (power_amp::status == power_amp::SHUTDOWN) {
        display->setFont((FontDef *)&Font_7x10);
        uint32_t remaining_sec = max2((int32_t)power_amp::hpa_shutdown_timeout_ms - (int32_t)(HAL_GetTick() - power_amp::last_hpa_shutdown_ms), 0) / 1000;
        display->setColor(C565_RED);
        display->print(" -");
        display->print(remaining_sec);
        display->setColor(C565_GREY_LIGHT);
        display->print("s");
        display->setColor(C565_WHITE);

    } else {
        if (power_amp::temp >= power_amp::params.MIN_TEMP) {

            display->setFont((FontDef *)&Font_Tiny8x8);
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
    }

    display->setFont((FontDef *)&Font_Tiny8x8);
    // TODO: GPSDO lock. Meanwhile, indicates warmup time has passed

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

void TitleBarIconsWidget::before_paint() {

    st_topBar topBar = {main_board::get_mute() ? true : false};

    if (dirty() || !(topBar == status)) {
        datetime = rtc_get_date_time();
        status = topBar;
        set_dirty();
    }
}

void TitleBarIconsWidget::on_info_changed_signal(const void *) {
    set_dirty();
}
