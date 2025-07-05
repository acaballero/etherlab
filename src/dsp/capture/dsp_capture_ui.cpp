//
// Created by Angel Dust on 16/04/2021.
//

#include <io/file_factory.h>
#include "dsp_capture_ui.h"
#include "io/file_types.h"
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

namespace dspCaptureUI {

FileType ftype = FTYPE_WAV;
int command = DSP_COMMAND_START;
io::path fname;
char fname_buff[PATH_SIZE];
bool filename_is_edited = false;
CaptureWidget capture_w{{DISPLAY_X_PIXELS / 2, MENU_START_Y + 10, DISPLAY_X_PIXELS / 2, INFO_HEIGHT - 10}, &lcd};

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
    fname = {fname_buff};
    ((CaptureTask *)dsp::tasks[dsp::DSP_TASK_CAPTURE])->setFile(FileFactory::getFile(ftype, fname));
    filename_is_edited = true;
    return Menu::proceed;
}

/**
 * Generates a file name using the timestamp if the RTC is enabled
 */
io::path get_file_name() {

    if (!filename_is_edited) {

#if ENABLE_RTC
        RTC_TimeTypeDef time;
        RTC_DateTypeDef date;
        HAL_RTC_GetTime(&hrtc, &time, FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &date, FORMAT_BIN);
        uint32_t f = (uint32_t)(radio::get_frequency() / 1000L);

        sprintf(fname_buff, WAVEFILE_DEFAULT_FILENAME, fft_params.sample_freq / fft_params.decimation_factor, f, date.Year, date.Month, date.Date, time.Hours,
                time.Minutes, time.Seconds, file_type_extensions[ftype]);

#else
        sprintf(buff, "%s", (char *)DSP_CAPTURE_DEFAULT_FILENAME);
#endif
    }

    return io::path{fname_buff};
}

Menu::result on_menu_event(Menu::eventMask e) {

    CaptureTask *task = ((CaptureTask *)dsp::tasks[dsp::DSP_TASK_CAPTURE]);

    switch (e) {
        case Menu::enterEvent:

            capture_w.setProcessorStatus(&((DspCaptureProcessor *)processors[DSP_PROCESSOR_CAPTURE])->status);
            capture_w.setTaskStatus(&((CaptureTask *)dsp::tasks[dsp::DSP_TASK_CAPTURE])->status);

            dsp_set_real_time(true);
            task->configureDsp();

            // Don't try to set the file name before configuring dsp params
            fname = WAVEFILE_DEFAULT_FOLDER;
            fname += "/" + get_file_name();

            task->setFile(FileFactory::getFile(ftype, fname));

            freqEdit.set_frequency(radio::get_frequency());

            break;
        case Menu::exitEvent:

            if (task->status.status == DSP_STATUS_STOPPED) {

                dsp_set_real_time(!ISANALOG);

                menu_size(DISPLAY_X_PIXELS, INFO_HEIGHT);
                view_manager::mainView.remove_child(&capture_w);
            } else {
                status::handleError(status::ST_ERROR, "Capturing!");
                return Menu::quit;
            }
            break;
    }
    return Menu::proceed;
}

void on_event(st_dsp_status *status) {

    switch (status->status) {
        case DSP_STATUS_RUNNING:
        case DSP_STATUS_PENDING:
            command = DSP_COMMAND_STOP;
            captureMenu[captureMenu.sz() - 1].disable();
            break;
        case DSP_STATUS_STOPPED:
            command = DSP_COMMAND_START;
            captureMenu[captureMenu.sz() - 1].enable();
            break;
    }
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

prompt *fileTypeValues[] = {new Menu::menuValue<FileType>(file_type_names[FTYPE_WAV], FTYPE_WAV),
                            new Menu::menuValue<FileType>(file_type_names[FTYPE_CS16], FTYPE_CS16)};

Menu::select<FileType> &fTypeMenu =
    *new Menu::select<FileType>("File type", ftype, sizeof(fileTypeValues) / sizeof(prompt *), fileTypeValues, change_file_type, exitEvent);

TOGGLE(command, captureToggle, "Command: ", change_dsp_status, Menu::anyEvent, Menu::noStyle,
       VALUE("Stop", DSP_COMMAND_STOP, change_dsp_status, Menu::anyEvent), VALUE("Start", DSP_COMMAND_START, change_dsp_status, Menu::anyEvent))

MENU(captureMenu, "Capture", on_menu_event, (Menu::eventMask)(Menu::enterEvent | Menu::exitEvent), Menu::noStyle, SUBMENU(captureToggle),
     EDIT("File:", fname_buff, Menu::alphaNumMask, on_file_updated, Menu::updateEvent, Menu::noStyle), OBJ(freqEdit),
     FIELD(config.fft.span, "Span", "Hz.", FFT_MIN_SPAN, FFT_MAX_SPAN, 10000, 0, set_sampling_params, anyEvent, noStyle), SUBMENU(fTypeMenu))
} // namespace dspCaptureUI
