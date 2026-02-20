//
// Created by Angel Dust on 16/04/2021.
//

#include <io/file_factory.h>
#include "Display_afb.h"
#include "dsp_replay_ui.h"
#include "../dsp_common.h"
#include "../dsp_tasks.h"
#include "../replay/replay_task.h"
#include "../dsp.h"
#include "io/fatfs_file.h"
#include "io/file_types.h"
#include "ui/gain_info.h"
#include "ui/main_view.h"
#include "ui/sd_filepicker_menu.h"
#include "replay_widget.h"
#include "ui/menu.h"
#include "ui/menu_frequency.h"
#include "radio.h"
#include "status.h"
#include "dsp/replay/dsp_replay_processor.h"
#include "dsp/dsp_tasks.h"
#include "dsp/capture/capture_task.h"
#include "ui/view_manager.h"

namespace dspReplayUI {

int8_t gain = 0;
int8_t prev_gain_db = 0;
bool stopped = true;
bool loop = false;

WaveInfo wi;
ReplayWidget replay_w{{DISPLAY_X_PIXELS / 2, MENU_START_Y - 35, DISPLAY_X_PIXELS / 2, INFO_HEIGHT - 6 + 35}, &lcd, "replay"};
// GainInfoWidget gain_w{{DISPLAY_X_PIXELS / 2, MENU_START_Y - 35, DISPLAY_X_PIXELS / 2, 20}, &lcd};
std::unique_ptr<File> file;

char tempFreqBuf[] = "00 000 000 000";

void on_freq_updated(uint64_t v) {
    radio::set_frequency(v);
    char buf[16];
    format_long(v, buf);
    sprintf(tempFreqBuf, "%s", buf);
}

void on_freq_signal(void *, const void *args) {
    radio::st_freq_event event = *((radio::st_freq_event *)args);
    if (event.event == radio::AFTER_UPDATE) {
        wi.carrier_freq = event.frequency;
        replay_w.setWaveInfo(wi);
    }
}

void on_event(st_dsp_params *status, st_dsp_params *task_info) {

    switch (task_info->status) {

        case DSP_STATUS_RUNNING:
        case DSP_STATUS_PENDING:
            stopped = false;
            filePicker.disable();
            dsp::set_gain_db(gain);
            break;
        case DSP_STATUS_STOPPED:
        case DSP_STATUS_STOPPING:
            stopped = true;
            filePicker.enable();
            dsp::set_gain_db(prev_gain_db);

            break;
    }

    replay_w.setProcessorStatus(*status);
    replay_w.setTaskStatus(*task_info);
}

Menu::result change_dsp_status(Menu::eventMask e) {

    if (e == Menu::enterEvent) {

        if (!stopped) {
            dsp_stop();
        } else {
            auto task = dsp_start(dsp::DSP_TASK_REPLAY, on_event);
            ((ReplayTask *)task)->setFile(file.get());
            ((ReplayTask *)task)->setLoop(loop);
            replay_w.setProcessorStatus(task->get_processor()->info);
            replay_w.setTaskStatus(task->info);
        }
    }

    return Menu::proceed;
}

Menu::result on_menu_event(Menu::eventMask e) {

    auto task = dsp_task.get();
    io::path start_path;
    FRESULT fres;

    switch (e) {

        case Menu::selBlurEvent:
            // If the task is finished, reset it
            if (task->info.stop_ms) {
                task->info.stop_ms = 0;
            }
            break;

        case Menu::enterEvent: {
            on_freq_updated(radio::get_frequency());
            // Select the last saved file as default
            io::path base_path = io::path{WAVEFILE_DEFAULT_FOLDER} + "/";
            FSO fso{base_path};

            start_path = fso.get_last_updated_file();

            fso.close();

            if (start_path.empty()) {
                start_path = io::path{"/"} + base_path;
                io::check_and_create_folder(start_path.parent_path().c_str());
            } else {
                start_path = base_path + start_path;
            }

            // TODO: Seriously, this menu system is one of the shittiest piece of code I've come across. Got to get rid of it any time soon
            filePicker.shadow->hFn = (Menu::callback)on_filepicker;

            // Start filepicker with selected file
            fres = filePicker.begin(start_path);

            // Update (select) it
            on_filepicker(Menu::updateEvent);

            if (fres == FR_OK) {

                menu_size(DISPLAY_X_PIXELS / 2, INFO_HEIGHT + 35);
                view_manager::mainView.add_child(&replay_w);
                // if (config.debug) {
                //     view_manager::mainView.add_child(&gain_w);
                //     gain_w.set_z_index(200);
                // }

                replay_w.set_visible(true);
                replay_w.set_z_index(100);
            }

            prev_gain_db = dsp::get_gain_db();

            break;
        }

        case Menu::exitEvent:

            //    if (stopped) {

            if (!stopped) {
                dsp_stop();
            }

            filePicker.end(); // Important to call begin/end as we need to lock the SD card while exploring
            menu_size(DISPLAY_X_PIXELS, INFO_HEIGHT);
            view_manager::mainView.remove_child(&replay_w);
            //   } else {

            //       status::pop_alert(status::ERROR, "Replaying!");
            //       return Menu::quit; // Cancel exit
            //   }
            break;
    }

    return Menu::proceed;
}

using namespace Menu;

result set_sampling_params(eventMask) {
    fft_config(config.fft.span);
    return proceed;
}

result change_gain(eventMask) {
    dsp::set_gain_db(gain);
    return proceed;
}

TOGGLE(loop, loopToggle, "Loop: ", doNothing, noEvent, noStyle, VALUE("Yes", true, doNothing, noEvent), VALUE("No", false, doNothing, noEvent))

result edit_freq(eventMask, navNode &) {
    if (stopped) {
        radio::set_band(radio::BAND_AUTO); // In case there's a band selected we need to be able to select any frequency
        Menu::open_keypad<uint64_t>(
            radio::get_frequency(), "Hz", "Frequency", 0, false,
            [](uint64_t v) {
                on_freq_updated(v);
            },
            radio::get_min_frequency(), radio::get_max_frequency());

        return proceed;
    }
    return quit;
}

labelPrompt freqEditMenu((const char *)"Frequency", tempFreqBuf, edit_freq, enterEvent, noStyle);
MENU(replayMenu, "Replay", on_menu_event, (eventMask)(enterEvent | exitEvent | selBlurEvent), noStyle,

     OP("Start / Stop", change_dsp_status, enterEvent), SUBMENU(filePicker), SUBMENU(loopToggle),
     FIELD(config.hw.dac_offset, "DAC offset:", "", 0, 3000, 1, 0, doNothing, noEvent, noStyle),
     FIELD(gain, "Gain:", " dB", DSP_MIN_TX_GAIN_DB, DSP_MAX_TX_GAIN_DB, 1, 0, change_gain, exitEvent, noStyle),

     OBJ(freqEditMenu)

)

Menu::result on_filepicker(eventMask e) {

    io::path path;

    if (e == Menu::refreshEvent) {
        path = filePicker.focused_path;
        // LOG("onfile: refresh: %s\n", path.c_str());
    } else {
        path = filePicker.selected_path;
        // LOG("onfile: select: %s\n", path.c_str());
    }

    // Check file format. Files are not deeply analyzed to determine their type. It is inferred from the extension

    file = FileFactory::getFile(path);

    FRESULT fres = FR_INVALID_NAME;
    if (file.get()) {
        fres = file->open(wi);
    }

    filePicker.disable_selection();
    filePicker.disable_deletion();

    if (fres == FR_INVALID_NAME) {
        replay_w.setWaveInfo({FSTATUS_NONE});
        if (e != updateEvent) {
            filePicker.enable_deletion();
        } else {
            replayMenu[0].disable();
            freqEditMenu.disable();
        }
    } else if (fres == FR_OK) {

        filePicker.enable_selection();
        filePicker.enable_deletion();

        if (e == updateEvent) {

            replayMenu[0].enable();
            freqEditMenu.enable();

            if (wi.carrier_freq) {
                on_freq_updated(wi.carrier_freq);
            } else {
                on_freq_updated(radio::get_frequency());
            }
        }

        replay_w.setWaveInfo(wi);

    } else {
        replay_w.setWaveInfo({FSTATUS_ERROR});
        filePicker.disable_selection();
        filePicker.disable_deletion();
        replayMenu[0].disable();
        freqEditMenu.disable();
    }

    return proceed;
}
} // namespace dspReplayUI
