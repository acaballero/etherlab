//
// Created by Angel Dust on 16/04/2021.
//

#include <io/file_factory.h>
#include "dsp_replay_ui.h"
#include "../dsp_common.h"
#include "../dsp_tasks.h"
#include "../replay/replay_task.h"
#include "../dsp.h"
#include "io/fatfs_file.h"
#include "io/file_types.h"
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

int8_t gain;
bool stopped = true;
bool loop = false;
SignalToken signal_token;
WaveInfo wi;
ReplayWidget replay_w{{DISPLAY_X_PIXELS / 2, MENU_START_Y - 35, DISPLAY_X_PIXELS / 2, INFO_HEIGHT - 6 + 35}, &lcd, "replay"};
std::unique_ptr<File> file;

void on_freq_signal(void *thisptr, const void *args) {
    radio::st_freq_event event = *((radio::st_freq_event *)args);
    if (event.event == radio::AFTER_UPDATE) {
        wi.carrier_freq = event.frequency;
        replay_w.setWaveInfo(wi);
    }
}

void on_event(st_dsp_params *status) {

    switch (status->status) {

        case DSP_STATUS_RUNNING:
        case DSP_STATUS_PENDING:
            stopped = false;
            filePicker.disable();
            break;
        case DSP_STATUS_STOPPED:
            stopped = true;
            filePicker.enable();
            break;
    }
}

Menu::result change_dsp_status(Menu::eventMask e) {

    Task *task = dsp_task.get();
    bool start = task && task->info.id == dsp::DSP_TASK_REPLAY && task->info.status != DSP_STATUS_RUNNING;

    if (e == Menu::activateEvent) {

        if (start) {
            dsp_stop();
        } else {
            dsp_start(dsp::DSP_TASK_REPLAY, on_event);
            ((ReplayTask *)task)->setFile(move(file));
        }
    }

    return Menu::proceed;
}

Menu::result on_menu_event(Menu::eventMask e) {

    auto task = (ReplayTask *)(dsp_task.get());
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
            signal_token = radio::freq_signal.add(NULL, on_freq_signal);

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
                replay_w.set_visible(true);
                replay_w.set_z_index(100);
                replay_w.setProcessorStatus(&task->get_processor()->info);
                replay_w.setTaskStatus(&task->info);
            }

            dsp_set_real_time(true);

            break;
        }

        case Menu::exitEvent:

            if (task->info.status == DSP_STATUS_STOPPED) {

                radio::freq_signal.remove(signal_token);

                dsp_set_real_time(!ISANALOG);

                filePicker.end(); // Important to call begin/end as we need to lock the SD card while exploring

                menu_size(DISPLAY_X_PIXELS, INFO_HEIGHT);
                view_manager::mainView.remove_child(&replay_w);
            } else {

                status::pop_alert(status::ERROR, "Replaying!");
                return Menu::quit; // Cancel exit
            }
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

TOGGLE(stopped, replayToggle, "Command: ", change_dsp_status, anyEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("Stop", true, change_dsp_status, anyEvent), VALUE("Start", false, change_dsp_status, anyEvent))

TOGGLE(loop, loopToggle, "Loop: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("Yes", true, doNothing, noEvent), VALUE("No", false, doNothing, noEvent))

Menu::result on_freq_updated(); // Forward declaration

menu_frequency::FreqEditField freqEdit((Menu::callback)on_freq_updated);

Menu::result on_freq_updated() {
    radio::set_frequency(freqEdit.get_frequency());
    return Menu::proceed;
}

MENU(replayMenu, "Replay", on_menu_event, (eventMask)(enterEvent | exitEvent | selBlurEvent), noStyle,

     SUBMENU(replayToggle), SUBMENU(filePicker), SUBMENU(loopToggle),
     FIELD(config.hw.dac_offset, "DAC offset:", "", 0, 2000, 1, 0, doNothing, noEvent, noStyle),
     FIELD(gain, "Gain:", " dB", DSP_MIN_TX_GAIN_DB, DSP_MAX_TX_GAIN_DB, 1, 0, change_gain, exitEvent, noStyle),
     // FIELD(config.fft.span, "Span", "Hz.", FFT_MIN_SPAN, FFT_MAX_SPAN, 10000, 0, set_sampling_params, anyEvent, noStyle),
     OBJ(freqEdit)
     // EDIT("Frequency (khz)", tempFreqBuf, digitMask, changeFreq, updateEvent, noStyle),
)

Menu::result on_filepicker(eventMask e) {

    io::path path;

    replay_w.setShowActions(e == Menu::refreshEvent || e == Menu::enterEvent);

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
            replayToggle.disable();
            freqEdit.disable();
        }
    } else if (fres == FR_OK) {

        filePicker.enable_selection();
        filePicker.enable_deletion();

        if (e == updateEvent) {

            replayToggle.enable();
            freqEdit.enable();

            if (wi.carrier_freq) {
                freqEdit.set_frequency(wi.carrier_freq);
            } else {
                freqEdit.set_frequency(radio::get_frequency());
            }

            on_freq_updated();
        }

        replay_w.setWaveInfo(wi);

    } else {
        replay_w.setWaveInfo({FSTATUS_ERROR});
        filePicker.disable_selection();
        filePicker.disable_deletion();
        replayToggle.disable();
        freqEdit.disable();
    }

    return proceed;
}
} // namespace dspReplayUI
