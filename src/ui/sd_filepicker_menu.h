#ifndef TRX_FRONTEND_SD_FILEPICKER_MENU_H
#define TRX_FRONTEND_SD_FILEPICKER_MENU_H

#include <cstring>
#include <memory>
#include <status.h>
#include <string>
#include "Display_afb.h"
#include "dsp/replay/dsp_replay_ui.h"
#include "hw/stm32.h"
#include "menuBase.h"
#include "ui/menu.h"
#include "../../lib/Menu/src/menu.h"
#include "../../lib/FatFs/ff.h"
#include "../fatfs/fatfs.h"
#include "../../lib/utils/utils.hpp"
#include "io/file_types.h"
#include "menu_actions.h"
#include "ui/menuILI9431Out.h"
#include "ui/ui_types.h"
#include "ui/view_manager.h"
#include "ui/modal_view.h"
#include "io/file_system.h"

// TODO: Error handling

class SDMenuT : public Menu::menuNode {
  public:
    io::path selected_path = "/";
    io::path focused_path = "/";
    int8_t focused_file_ix = -1;
    bool can_select = true;
    bool can_delete = true;
    std::vector<int> sel_items;
    std::unique_ptr<FSO> fso;

    Menu::menu_action_st menu_actions_arr[6];
    Menu::menu_actions_st menu_actions = {menu_actions_arr, 6};

    enum menu_actions_type { UP, DOWN, ENTER, BACK, OPEN, DELETE };
    void enable_selection() {
        this->can_select = true;
    }

    void disable_selection() {
        this->can_select = false;
    }

    void enable_deletion() {
        this->can_delete = true;
    }

    void disable_deletion() {
        this->can_delete = false;
    }

    bool is_dir(BYTE attr) {
        return attr & AM_DIR;
    }

    // Using menuNode::menuNode
    // do not use default constructors as we wont allocate for data
    SDMenuT(constText *title, const char *, Menu::action act = Menu::doNothing, Menu::eventMask mask = Menu::noEvent)
        : menuNode(title, 0, NULL, act, mask, Menu::noStyle, (Menu::systemStyles)(Menu::_menuData | Menu::_canNav)) {
    }

    FRESULT begin() {
        return this->begin(selected_path);
    }

    FRESULT begin(const io::path &path) {

        if (!fso) {
            fso = std::make_unique<FSO>(); // Don't try to construct the FSO in the bss (sdcard device won't be initialized)
        }

        fso->close();
        FRESULT fres = fso->openFolder(path);

        if (fres == FR_OK) {
            focused_path = path.parent_path() + "/";
            // Select the file
            // LOG("begin: before entryIdx\n");
            int ix = fso->entryIdx(path.filename().c_str());

            if (ix >= 0) {
                focus(ix);
            }

            selected_path = focused_path;
            fso->page_top_ix = 0;
        }

        uint16_t i = 0;
        for (i = 0; i < Menu::navigation_actions.size; i++) {
            menu_actions_arr[i] = Menu::navigation_actions_arr[i];
        }

        menu_actions_arr[OPEN] = {"Open", []() {
                                      nav.doNav(Menu::selCmd);
                                  }};
        menu_actions_arr[DELETE] = {"Delete", [this]() {
                                        delete_files();
                                    }};

        refresh();

        return fres;
    }

    void refresh() {
        fso->curr_folder_count = -1;
        fso->count();

        if (nav.navFocus == this && focused_file_ix >= fso->curr_folder_count) {
            nav.node().sel = fso->curr_folder_count;
            focus(fso->curr_folder_count - 1);
        }
        fso->read_page(&fso->dir, 0, fso->file_page, FILES_PER_PAGE);
        clear_selection();
        update_actions();
    }

    void delete_files() {

        std::string message_str;

        if (sel_items.empty() && nav.node().sel >= 1 && this->can_delete) {
            char fn[FN_SIZE];
            fso->entry(nav.node().sel - 1, fn, sizeof(fn));
            if (!(is_dir(fso->fileinfo.fattrib))) {
                toggle_select(nav.node().sel - 1);
            }
        }

        if (!sel_items.empty()) {

            if (sel_items.size() == 1) {
                char fn[FN_SIZE];
                fso->entry(sel_items[0], fn, sizeof(fn));
                io::path file_path = focused_path.parent_path() / fn;
                message_str = {"Delete " + file_path.native() + " file/s?"};
            } else {
                message_str = {"Delete " + std::to_string(sel_items.size()) + " files?"};
            }

            view_manager::open(std::make_unique<ModalView>("Confirmation", message_str, modal_t::YESNO, [this](bool ok) {
                if (ok) {

                    for (const auto ix : sel_items) {
                        char fn[FN_SIZE];
                        fso->entry(ix, fn, sizeof(fn));
                        io::path file_path = focused_path.parent_path() / fn;

                        // LOG("Deleting file '%s'\n", file_path.c_str());
                        FRESULT res = f_unlink(file_path.c_str());

                        if (res != FR_OK) {
                            status::pop_alert(status::ERROR, "Error deleting file");
                        }
                    }

                    refresh();
                } else {
                    if (sel_items.size() == 1) {

                        refresh();
                    }
                }
            }));
        }
    }

    void clear_selection() {
        sel_items.clear();
    }

    void end() {
        fso->close();
    }

    // this requires latest menu version to virtualize data tables
    Menu::prompt &operator[](Menu::idx_t) const override {
        return *(Menu::prompt *)this;
    } // this will serve both as menu and as its own prompt

    Menu::result sysHandler(SYS_FUNC_PARAMS) override {
        switch (event) {
            case Menu::enterEvent:
                if (nav.root->navFocus != nav.target) { // On sd card entry
                    // restore context
                    if (!selected_path.filename().empty()) {
                        FRESULT res = begin();
                        if (res == FR_OK) {
                            nav.sel = fso->entryIdx(selected_path.filename().c_str()) + 1;
                        }
                    }
                    if (&item == this) {
                        actions_signal.emit(&menu_actions);
                    }
                }

            default:
                break;
        }
        return Menu::proceed;
    }

    void focus(int i) {

        if (i < 0) {
            focused_path = focused_path.parent_path() + "/";
        } else {
            char fn[FN_SIZE];
            fso->entry(i, fn, sizeof(fn));
            focused_path = focused_path.parent_path() / fn;
            // LOG("focus:%s,%d\n", fn, i);
        }

        focused_file_ix = i;
        nav.node().event(Menu::refreshEvent);
    }

    void toggle_select(int item) {

        auto it = std::find(sel_items.begin(), sel_items.end(), item);
        if (it != sel_items.end()) {
            // Item exists, remove it
            sel_items.erase(it);
        } else {
            // Item doesn't exist, add it
            sel_items.push_back(item);
        }
    }

    bool is_selected(int item) {
        return std::find(sel_items.begin(), sel_items.end(), item) != sel_items.end();
    }

    void update_actions() {

        auto sel = nav.node().sel;

        if (!sel) {
            if (!focused_path.filename().empty()) {
                //   LOG("update_actions: before entryIdx\n");
                sel = fso->entryIdx(focused_path.filename().c_str()); // Updates fso::fileinfo
            }
        }

        fso->entry(sel - 1); // loads fso->fileinfo

        menu_actions_arr[OPEN].enabled = sel && sel_items.empty() && can_select;
        menu_actions_arr[DELETE].enabled = (!sel_items.empty() || can_delete) && !is_dir(fso->fileinfo.fattrib) && sel;

        if (nav.node().target == this) {
            actions_signal.emit(&menu_actions);
        }
    }

    void doNav(Menu::navNode &nav, Menu::navCmd cmd) override {

        io::path folder = focused_path.parent_path();
        char fn[FN_SIZE];
        Menu::navCmd bubble_cmd = cmd;
        bool navigate = false;

        if (cmd.cmd == Menu::enterCmd && nav.sel == 0) { // [..] has been clicked -> Previous folder
            cmd.cmd = Menu::escCmd;
        }

        switch (cmd.cmd) {
            case Menu::idxCmd: // Options
                delete_files();
                break;
            case Menu::enterCmd:
                if (nav.sel >= 1) {

                    fso->entry(nav.sel - 1, fn, sizeof(fn));
                    focused_path = folder / fn;

                    if (is_dir(fso->fileinfo.fattrib)) {
                        // Open folder (reusing the menu)
                        focused_path /= "";
                        fso->openFolder(focused_path);
                        dirty = true; // Redraw menu
                        nav.sel = 0;
                        refresh();
                    } else {
                        if (this->can_select) {
                            toggle_select(nav.sel - 1);
                        }
                    }
                } else {
                    // [..]
                }
                break;
            case Menu::escCmd:

                if (folder.empty() || folder.filename().empty()) {
                    // Root
                    nav.root->node().event(Menu::enterEvent);
                    bubble_cmd = Menu::escCmd;
                    navigate = true;
                } else {
                    // Previous folder

                    focused_path = focused_path.parent_path().parent_path(); // 1st up: the folder + one up, the parent folder
                    fso->openFolder(focused_path);
                    //  LOG("Menu::escCmd: before entryIdx\n");
                    nav.sel = fso->entryIdx(folder.filename().c_str()) + 1;
                    focus(nav.sel - 1);
                    dirty = true; // redraw menu
                    refresh();
                }

                break;
            case Menu::downCmd:
            case Menu::upCmd:
                navigate = true;
                break;
            case Menu::selCmd:
                if (nav.sel >= 1 && this->can_select) {
                    // Select current file and exit
                    char fn[FN_SIZE];
                    fso->entry(nav.sel - 1, fn, sizeof(fn));
                    selected_path = selected_path.parent_path() / fn;
                    nav.event(Menu::updateEvent);
                    bubble_cmd = Menu::escCmd;
                    navigate = true;
                    dirty = true;
                }
                break;
            case Menu::noCmd:
            case Menu::leftCmd:
            case Menu::rightCmd:
            case Menu::scrlUpCmd:
            case Menu::scrlDownCmd:
                break;
        }

        if (navigate) {

            menuNode::doNav(nav, bubble_cmd);

            if (nav.sel - 1 != focused_file_ix) {
                focus(nav.sel - 1);
            }

            if (bubble_cmd == Menu::escCmd) {
                actions_signal.emit(nullptr); // Will exit: unstack quick actions
            } else {
                update_actions();
            }
        }
    }

    // Print menu and items as this is a virtual data menu
    Menu::Used printTo(Menu::navRoot &root, bool sel, Menu::menuOut &out, Menu::idx_t idx, Menu::idx_t len, Menu::idx_t) override {

        bool show_parent = out.tops[root.level] == 0;

        if (root.navFocus != this) {

            // Show given title or filename if selected
            if (selected_path.filename().empty() || !can_select) {
                /*menuNode::printTo(root,sel,out,idx,len,pn)*/
                return out.printRaw(shadow->text, len);
            } else { // Only shows the file if the selection is enabled
                len -= out.printRaw(shadow->text, len);
                len -= out.printRaw(": ", len);
                out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
                return out.printRaw(selected_path.filename().c_str(), len);
            }

        } else if (idx == -1) {
            // When menu open (show folder name)

            ((Menu::menuNodeShadow *)shadow)->sz = fso->count() + 1;

            auto str = focused_path.parent_path().c_str();
            return out.printRaw(str[0] ? str : "/", len);
        } else {
            int top = out.tops[root.level];
            top = top < 2 ? 0 : top - 1;

            Menu::idx_t i = top + idx;
            st_file_page_entry *fentry;
            char buff[FN_SIZE + 4];

            if (top != fso->page_top_ix) { // update current page
                fso->read_page(&fso->dir, top, fso->file_page, FILES_PER_PAGE);
                fso->page_top_ix = top;
            }

            int pg_index = i - fso->page_top_ix - (show_parent ? 1 : 0);

            if (show_parent && i == 0) {
                out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
                return out.printRaw("[..]", len);
            } else {
                fentry = &fso->file_page[pg_index];
            }

            i = i - (show_parent ? 1 : 0);

            bool is_dir = fentry->attr & AM_DIR;
            auto color = is_dir ? Menu::valColor : Menu::fgColor;

            if (is_selected(i)) {
                out.setColor(Menu::titleColor, sel);
            } else {

                out.setColor(color, sel);
            }

            snprintf(buff, sizeof(buff), "[%2d] ", i + 1);
            len -= out.printRaw(buff, len);

            out.setColor(color, sel);
            snprintf(buff, sizeof(buff), "%s", fentry->name);
            len -= out.printRaw(buff, len);

            if (is_dir) {
                len -= out.printRaw("/", len);
            }
        }

        return len;
    }
};

extern SDMenuT filePicker;

#endif // TRX_FRONTEND_SD_FILEPICKER_MENU_H
