//
// Created by Angel Dust on 15/07/2022.
//

#ifndef TRX_FRONTEND_S_STRENGTH_H
#define TRX_FRONTEND_S_STRENGTH_H

#include "Signal.h"

namespace sstrength {

    struct st_sstrength_info {
        bool in_squelch;
        float level;
    };

    extern Signal s_strength_signal, squelch_signal;
    extern float s_level;
    extern float s_strength;
    void loop();
    float update_s_strength();
    float db_to_s_strength(float db);
    void set_squelch(float s_level);
    float get_squelch();
}

#endif //TRX_FRONTEND_S_STRENGTH_H
