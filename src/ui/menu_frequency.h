//
// Created by Angel Dust on 21/10/2022.
//

#ifndef TRX_FRONTEND_MENU_FREQUENCY_H
#define TRX_FRONTEND_MENU_FREQUENCY_H

#include <Signal.h>
#include "menu.h"
#include "stdio.h"

namespace menu_frequency {

using namespace Menu;

extern constMEM char freqEditText[] MEMMODE;

class FreqEditField : public Menu::fieldBase {
  public:
    FreqEditField(const char *label, Menu::callback cb)
        : fieldBase(*new fieldBaseShadow(label, " Hz", cb, (Menu::eventMask)(Menu::updateEvent | Menu::exitEvent), Menu::noStyle,
                                         (Menu::systemStyles)(Menu::_noStyle | Menu::_canNav | Menu::_parentDraw))) {}

    FreqEditField(Menu::callback cb) : FreqEditField(freqEditText, cb) {}

    void set_frequency(uint64_t f);

    void set_min_frequency(uint64_t f);

    void set_max_frequency(uint64_t f);

    uint64_t get_frequency() { return frequency; }

    void doNav(Menu::navNode &nav, Menu::navCmd cmd) override;

    Used printTo(navRoot &root, bool sel, menuOut &out, idx_t idx, idx_t len, idx_t panelNr) override;

  private:
    uint64_t frequency = 0;
    uint64_t min = 0;
    uint64_t max = UINT64_MAX;
    size_t step_at = 3;
    bool edited = false;
    bool step_edit = false;

    // Hide those unused methods of the base class
    virtual bool canTune() override { return false; }

    virtual void constrainField() override {}

    virtual void stepit(int increment) override {}

    virtual idx_t printReflex(menuOut &o) const override { return 0; }
};
} // namespace menu_frequency

#endif // TRX_FRONTEND_MENU_FREQUENCY_H
