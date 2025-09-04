//
// Created by Angel Dust on 16/04/2021.
//

#include <io/file_factory.h>
#include "dsp/dsp_ui.h"
#include "dsp/fft/fft.h"
#include "dsp/replay/dsp_replay_ui.h"
#include "dsp_capture_ui.h"
#include "io/file_types.h"
#include "items.h"
#include "menuBase.h"
#include "ui/menu.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_tasks.h"
#include "dsp/dsp.h"
#include "ui/main_view.h"
#include "ui/lcd.h"
#include "ui/menu_frequency.h"
#include "capture_task.h"
#include "dsp/dsp_processors.h"
#include "dsp_capture_processor.h"
#include "capture_widget.h"
#include "fatfs/fatfs.h"
#include "status.h"
#include "ui/view_manager.h"
#include "io/file_system.h"
#include "main_board.h"
#include "os/task_manager.h"

namespace dspCaptureUI {

FileType ftype = FTYPE_WAV;
int command = DSP_COMMAND_START;
io::path fname;
char fname_buff[PATH_SIZE];
bool filename_is_edited = false;
CaptureWidget capture_w{{DISPLAY_X_PIXELS / 2, MENU_START_Y + 10, DISPLAY_X_PIXELS / 2, INFO_HEIGHT - 10}, &lcd};
MODE previous_mode;
Menu::result on_freq_updated(Menu::eventMask e); // Forward declaration
io::path get_file_name();                        // Forward declaration

menu_frequency::FreqEditField freqEdit((Menu::callback)on_freq_updated);

Menu::result on_freq_updated(Menu::eventMask e) {
    radio::set_frequency(freqEdit.get_frequency());
    if (e == Menu::exitEvent) {
        fname = get_file_name();
    }
    return Menu::proceed;
}

Menu::result on_file_updated(Menu::eventMask) {
    fname = fname_buff;
    ((CaptureTask *)dsp::tasks[dsp::DSP_TASK_CAPTURE])->setFile(FileFactory::getFile(ftype, fname));
    filename_is_edited = true;
    return Menu::proceed;
}

/**
 * Generates a file name using the timestamp if the RTC is enabled
 */
io::path get_file_name() {

    if (!filename_is_edited) {

        uint32_t f = (uint32_t)(radio::get_frequency() / 1000L);

        FSO fso;
        bool found = false;
        io::path folder = io::path{WAVEFILE_DEFAULT_FOLDER} + "/";
        fso.openFolder(folder);
        int n = fso.count();

        if (n >= 0) {
            while (!found) {
                sprintf(fname_buff, WAVEFILE_DEFAULT_FILENAME, n, fft_params.sample_freq / fft_params.decimation_factor, f, file_type_extensions[ftype]);
                int ix = fso.entryIdx((folder / fname_buff).c_str());
                found = ix == 0;
            }
        } else {
            status::handleError(status::ST_ERROR, "Error in get_file_name()");
            return "capture.wav";
        }
    }

    return io::path{fname_buff};
}

Menu::result on_menu_event(Menu::eventMask e) {

    CaptureTask *task = ((CaptureTask *)dsp::tasks[dsp::DSP_TASK_CAPTURE]);

    switch (e) {
        case Menu::enterEvent:

            capture_w.setProcessorStatus(&((DspCaptureProcessor *)processors[DSP_PROCESSOR_CAPTURE])->status);
            capture_w.setTaskStatus(&((CaptureTask *)dsp::tasks[dsp::DSP_TASK_CAPTURE])->status);

            captureMenu[captureMenu.sz() - 1].disable();
            dsp_set_real_time(true);

            // Don't try to set the file name before configuring dsp params
            fname = WAVEFILE_DEFAULT_FOLDER;
            fname += "/" + get_file_name();

            task->setFile(FileFactory::getFile(ftype, fname));

            freqEdit.set_frequency(radio::get_frequency());

            // Remove fft update priority
            fft::fft_task.set_high_priority(false);

            previous_mode = config.mode;

            break;
        case Menu::exitEvent:

            if (task->status.status == DSP_STATUS_STOPPED) {

                // Remove fft update priority
                fft::fft_task.set_high_priority(true);
                dsp_set_real_time(!ISANALOG);
                menu_size(DISPLAY_X_PIXELS, INFO_HEIGHT);
                view_manager::mainView.remove_child(&capture_w);
            } else {
                status::handleError(status::ST_ERROR, "Capturing!");
                return Menu::quit;
            }
            break;
        case Menu::noEvent:
        case Menu::activateEvent:
        case Menu::returnEvent:
        case Menu::focusEvent:
        case Menu::blurEvent:
        case Menu::selFocusEvent:
        case Menu::selBlurEvent:
        case Menu::updateEvent:
        case Menu::refreshEvent:
        case Menu::anyEvent:
            break;
    }
    return Menu::proceed;
}

void on_event(st_dsp_status *status) {

    switch (status->status) {
        case DSP_STATUS_RUNNING:
        case DSP_STATUS_PENDING:
            command = DSP_COMMAND_STOP;
            captureMenu[captureMenu.sz() - 2].disable();
            captureMenu[captureMenu.sz() - 1].disable();
            break;
        case DSP_STATUS_STOPPED:
            command = DSP_COMMAND_START;
            captureMenu[captureMenu.sz() - 2].enable();
            captureMenu[captureMenu.sz() - 1].enable();

            // Set the previous mode
            os::task_manager.set_timeout(1, []() {
                if (previous_mode == DIGITAL_RX) {
                    dsp_command({(DSP_COMMAND)DSP_COMMAND_START, dsp::DSP_TASK_RECEIVE}, nullptr);
                }
                main_board::set_mode(previous_mode);
            });

            break;
        case DSP_STATUS_STOPPING:
            break;
    }

    nav.node().target->dirty = true; // Ugly!
}

Menu::result change_dsp_status(Menu::eventMask e) {
    if (e == Menu::activateEvent) {

        DSP_COMMAND nextCommand = command == DSP_COMMAND_START ? DSP_COMMAND_STOP : DSP_COMMAND_START;

        if (nextCommand == DSP_COMMAND_START) {
            view_manager::mainView.add_child(&capture_w);
            capture_w.set_visible(true);
            menu_size(DISPLAY_X_PIXELS / 2, INFO_HEIGHT);
        }

        dsp_command({(DSP_COMMAND)nextCommand, dsp::DSP_TASK_CAPTURE}, on_event);
    }
    return Menu::proceed;
}

using namespace Menu;

result set_sampling_params(eventMask) {
    fft_config(config.fft.span);
    fname = get_file_name();
    return proceed;
}

result change_file_type(eventMask) {
    fname = get_file_name();
    ((CaptureTask *)dsp::tasks[dsp::DSP_TASK_CAPTURE])->setFile(FileFactory::getFile(ftype, fname));
    return proceed;
}

result replay(eventMask) {
    // Open replay menu

    nav.doNav(navCmd(escCmd, 1));
    nav.doNav(navCmd(idxCmd, 1));
    // nav.node().target = &dspReplayUI::replayMenu;
    //  nav.node().selected() = 0;
    return proceed;
}

prompt *fileTypeValues[] = {new Menu::menuValue<FileType>(file_type_names[FTYPE_WAV], FTYPE_WAV),
                            new Menu::menuValue<FileType>(file_type_names[FTYPE_CS16], FTYPE_CS16)};

Menu::select<FileType> &fTypeMenu =
    *new Menu::select<FileType>("File type", ftype, sizeof(fileTypeValues) / sizeof(prompt *), fileTypeValues, change_file_type, exitEvent);

#ifdef __clang__
#ifndef typeof
#define typeof __typeof__
#endif
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"

#endif

TOGGLE(command, captureToggle, "Command: ", change_dsp_status, Menu::anyEvent, Menu::noStyle,
       VALUE("Stop", DSP_COMMAND_STOP, change_dsp_status, Menu::anyEvent), VALUE("Start", DSP_COMMAND_START, change_dsp_status, Menu::anyEvent))

MENU(captureMenu, "Capture", on_menu_event, (Menu::eventMask)(Menu::enterEvent | Menu::exitEvent), Menu::noStyle, SUBMENU(captureToggle),
     EDIT("File:", fname_buff, Menu::alphaNumMask, on_file_updated, Menu::updateEvent, Menu::noStyle), OBJ(freqEdit),
     FIELD(config.fft.span, "Span", "Hz.", FFT_MIN_SPAN, FFT_MAX_SPAN, 10000, 0, set_sampling_params, anyEvent, noStyle), SUBMENU(fTypeMenu),
     OP("Replay", replay, enterEvent))
} // namespace dspCaptureUI

#ifdef __clang__
#pragma clang diagnostic pop
#endif
