/* -*- C++ -*- */

#ifndef RSITE_ARDUINO_MENU_SWOOUT
#define RSITE_ARDUINO_MENU_SWOOUT
#include "../menuDefs.h"

namespace Menu {

extern panelsList default_serial_panel_list;

class SWOOut : public menuOut {
  public:
    idx_t lastLine = -1;
    inline SWOOut(idx_t *t, panelsList &p = default_serial_panel_list, menuOut::styles st = menuOut::drawNumIndex) : menuOut(t, p, st) {
    }
    size_t write(uint8_t ch) override {
        // trace(MENU_DEBUG_OUT.write('|'));
        return printf("%c", ch);
    }
    void clear() override {
        println();
        panels.reset();
    }
    void clear(idx_t panelNr) override {
        trace(MENU_DEBUG_OUT << "serialOut::clear(idx_t panelNr)" << endl;) trace(MENU_DEBUG_OUT.flush());
        println();
        panels.nodes[panelNr] = NULL;
    }
    void clearLine(idx_t ln, idx_t panelNr = 0, colorDefs color = bgColor, bool selected = false, status stat = enabledStatus, bool edit = false) override {
        lastLine = -1;
        printf("\n");
    }
    void setCursor(idx_t x, idx_t y, idx_t panelNr = 0) override {
        if (lastLine >= 0 && lastLine != y)
            println();
        lastLine = y;
    };
};

} // namespace Menu

#endif
