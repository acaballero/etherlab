//
// Created by Angel Dust on 21/10/2022.
//

#include "menu_frequency.h"
#include "menu.h"
#include "../../lib/utils/utils.hpp"
#include "menuBase.h"

namespace menu_frequency {

using namespace Menu;

constMEM char freqEditText[] MEMMODE = "Freq";

void FreqEditField::set_frequency(uint64_t f) {
    frequency = constrain(f, min, max);
}

void FreqEditField::doNav(Menu::navNode &nav, Menu::navCmd cmd) {

    uint64_t f = frequency;

    switch (cmd.cmd) {
        case enterCmd:

            step_edit = !step_edit;
            dirty = true;
            edited = true;
            break;
        case escCmd:
            dirty = true;
            if (step_edit) {
                step_edit = false;
            } else {
                edited = false;
                nav.root->exit();
            }
            break;
        case upCmd:
            if (!step_edit) {
                f += pow(10, step_at);
            } else {
                if (step_at) {
                    step_at--;
                }
                edited = false;
            }
            dirty = true;
            break;
        case downCmd:
            if (!step_edit) {
                f -= pow(10, step_at);
            } else {
                if (step_at < (uint8_t)log10((double)frequency)) {
                    step_at++;
                }
                edited = false;
            }
            dirty = true;
            break;
        default:
            break;
    }

    if (f != frequency) {
        set_frequency(f);
        nav.event(Menu::updateEvent);
    }
}

Used FreqEditField::printTo(navRoot &root, bool sel, menuOut &out, idx_t idx, idx_t len, idx_t panelNr) {

    bool editing = this == root.navFocus;
    bool at_cursor;
    size_t c, i;
    char buf[20];

    idx_t l = navTarget::printTo(root, sel, out, idx, len, panelNr);
#ifdef MENU_FMT_WRAPS
    out.fmtStart(*this, menuOut::fmtEditCursor, root.node(), idx);
#endif
    if (l < len) {
        out.write(editing ? "> " : ": ");
        l += 2;
    }

#ifdef MENU_FMT_WRAPS
    out.fmtEnd(*this, menuOut::fmtEditCursor, root.node(), idx);
    out.fmtStart(*this, menuOut::fmtTextField, root.node(), idx);
#endif

    out.setColor(Menu::valColor, sel, enabled, editing);
    sprintf(buf, "%lu", (long)frequency);

    i = strlen(buf) - 1;
    c = 2 - ((i + 2) % 3);
    for (char *p = buf; *p != 0 && l < len; p++, l++, i--) {

        if (c == 1 && p > buf) {
            out.write(",");
            l++;
        }
        c = (c + 1) % 3;
        at_cursor = editing && step_at == i;

        if (at_cursor) {
            l += out.startCursor(root, l, idx + 1, step_edit);
        }
        out.write(*p);
        if (at_cursor) {
            l += out.endCursor(root, l, idx + 1, step_edit);
        }
    }

#ifdef MENU_FMT_WRAPS
    out.fmtEnd(*this, menuOut::fmtTextField, root.node(), idx);
#endif

    if (l < len) {
#ifdef MENU_FMT_WRAPS
        out.fmtStart(*this, menuOut::fmtUnit, root.node(), idx);
#endif

        out.setColor(Menu::unitColor, sel, enabled, editing);
        l += print_P(out, units(), len);
#ifdef MENU_FMT_WRAPS
        out.fmtEnd(*this, menuOut::fmtUnit, root.node(), idx);
#endif
    }

    return l;
}

void FreqEditField::set_max_frequency(uint64_t f) {
    max = f;
}

void FreqEditField::set_min_frequency(uint64_t f) {
    min = f;
}
} // namespace menu_frequency
