//
// Created by Angel Dust on 02/07/2024.
//

#ifndef TRX_FRONTEND_DSP_SIGNAL_GENERATOR_UI_H
#define TRX_FRONTEND_DSP_SIGNAL_GENERATOR_UI_H

#include "../../../lib/Menu/src/menu.h"
#include "../dsp_common.h"

namespace dspSignalGeneratorUI {
    extern Menu::menu signalGeneratorMenu;

    void on_event(st_dspStatus *status);
    Menu::result change_dsp_status(Menu::eventMask e);
    Menu::result on_menu_event(Menu::eventMask e);

}

#endif //TRX_FRONTEND_DSP_SIGNAL_GENERATOR_UI_H
