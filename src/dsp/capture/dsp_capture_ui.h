//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_CAPTURE_UI_H
#define TRX_FRONTEND_DSP_CAPTURE_UI_H

#include "../../../lib/Menu/src/menu.h"

namespace dspCaptureUI {

    extern Menu::menu captureMenu;

    Menu::result changeDspStatus(Menu::eventMask e);
    Menu::result on_menu_event(Menu::eventMask e);
    Menu::result on_file_updated(Menu::eventMask e);
    Menu::result change_dsp_status(Menu::eventMask e);
}

#endif //TRX_FRONTEND_DSP_CAPTURE_UI_H
