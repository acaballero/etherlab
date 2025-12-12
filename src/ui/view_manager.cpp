//
// Created by Angel Dust on 12/07/2024.
//
#include "view_manager.h"
#include "Display_afb.h"
#include "input/inputEvent.h"
#include "os/periodic_task.h"
#include "status.h"
#include "stm32f4xx_hal.h"
#include "types.h"
#include "ui/keyboard_view.h"
#include "ui/keypad_view.h"
#include "ui/number_edit_view.h"
#include "ui/option_buttons_view.h"
#include "ui/splash_view.h"
#include "os/task_manager.h"
#include "dsp/aprs/aprs_ui.h"
#include "ui/status_widget.h"
#include "ui/ui_types.h"
#include "ui/view.h"
#include <cstddef>
#include <sys/_stdint.h>

namespace view_manager {

static const uint8_t MAX_VIEWS = 6;

MainView mainView;
SplashView splashView;
KeypadView keypadView{{0, HEADER_HEIGHT, DISPLAY_X_PIXELS, KeypadView::HEIGHT}};
KeyboardView keyboardView{{0, HEADER_HEIGHT, KeyboardView::WIDTH, KeyboardView::HEIGHT}};
MessageView msg_w{
    {6, DISPLAY_Y_PIXELS * 2 / 3, DISPLAY_X_PIXELS - 12, INFO_HEIGHT - 6}, (FontDef *)&Font_11x18, (FontDef *)&Font_7x10, C565_GREY_DARK, C565_RED, C565_WHITE};
// NumberEditView numberEditView{{0, DISPLAY_Y_PIXELS - NumberEditView::HEIGHT, DISPLAY_X_PIXELS, NumberEditView::HEIGHT}};
// OptionButtonsView optionButtonsView{{0, HEADER_HEIGHT, DISPLAY_X_PIXELS, OptionButtonsView::HEIGHT}};
View *breadcrumb[MAX_VIEWS];
View *currentView;
int view_index = -1;

std::unique_ptr<View> app_view_p;
std::unique_ptr<View> view_p;

void view_loop();
os::periodic_task task(250, view_loop);

void push(View *view) {
    if (view_index < MAX_VIEWS) {
        if (currentView) {
            currentView->set_visible(false);
        }
        currentView = view;
        breadcrumb[++view_index] = view;
        currentView->set_visible(true);
        currentView->set_dirty();
        currentView->paint();
    }
}

void pop() {
    if (view_index > 0) {

        currentView->set_visible(false);
        currentView = breadcrumb[--view_index];

        currentView->set_visible(true);
        currentView->set_dirty();
        currentView->set_focus(true);
        currentView->paint();
    }
}

void main_view_warning_callback(void *, const void *args) {

    status::Status *st = (status::Status *)args;

    static int task_id; // Ugly. Functions should be stateless, but 'static' exist so what shoud I say... ;)

    View *current = breadcrumb[view_index];

    if (task_id != -1) {
        os::task_manager.remove(task_id);
    }

    task_id = os::task_manager.set_timeout(
        4000,
        [current]() {
            current->remove_child(&msg_w);
            // msg_w.set_visible(false);
        },
        "msgv");

    bool was_visible = msg_w.parent() && msg_w.visible();
    current->add_child(&msg_w); // does nothing if the child already has a parent
    current->to_top(&msg_w);

    if (was_visible) {
        msg_w.add_log(st->code == status::ERROR ? "ERROR" : "INFO", st->msg);
    } else {
        msg_w.show_msg(st->code == status::ERROR ? "ERROR" : "INFO", st->msg);
    }
}

void init() {
    status::status_signal.add(NULL, main_view_warning_callback);

    keypadView.on_hide_fn = pop;
    keyboardView.on_hide_fn = pop;
    msg_w.set_name("msg");
    mainView.set_visible(false);

    LOG("Initializing view manager\n");
    push(&splashView);

    lcd.backlight(true);

    os::task_manager.set_timeout(1500, []() {
        pop();
        push(&mainView);
    });

    // This signal receives the currently focused widget
    // TODO: Move to this namespace
    Menu::navigation_signal.add(nullptr, [](void *, const void *params) {
        Widget *w = (Widget *)params;
        if (w && (w->focused_widget() || w->is_focused())) {
            ((StatusWidget *)mainView.Status())->set_actions(w->get_quick_actions());
        } else {
            ((StatusWidget *)mainView.Status())->set_actions(nullptr);
        }
    });
}

void view_loop() {
    // TODO: Delegate dirty state manaegnment to the widget itself based on
    // information change messages and refresh rate

    mainView.TuneInfo()->set_dirty();
    mainView.FFTInfo()->set_dirty();

    currentView->paint();
}

using test_result_t = std::pair<Widget *const, const uint32_t>;

/* Walk all visible widgets in hierarchy, collecting those that pass test */
template <typename TestFn> static void collect_widgets(Widget *const w, TestFn test, std::vector<test_result_t> &collection) {
    for (auto child : w->children()) {
        if (child->can_be_seen()) {
            const auto result = test(child);
            if (result.first) {
                // auto w2 = ((Widget *)result.first);
                //      LOG("candidate to focus: %s | %d x %d | distance: %d\n", w2->get_name(), w2->parent_rect().width(), w2->parent_rect().height(),
                //      result.second);
                collection.push_back(result);
            }
            collect_widgets(child, test, collection);
        }
    }
}

void open_app(std::unique_ptr<View> view) {
    app_view_p = std::move(view);

    auto *view_ptr = app_view_p.get();
    app_view_p->on_hide_fn = [view_ptr]() {
        view_manager::mainView.remove_child(view_ptr);
        app_view_p.reset();
        Menu::close();
    };

    app_view_p->set_visible(true);

    app_view_p->set_focus(true);

    view_manager::mainView.add_child(app_view_p.get());
    view_manager::mainView.to_top(app_view_p.get());
}

void open(std::unique_ptr<View> v) {
    view_p = move(v);

    auto *view_ptr = view_p.get();
    view_p->on_hide_fn = [view_ptr]() {
        view_manager::mainView.remove_child(view_ptr);
        view_p.reset();
    };

    view_manager::mainView.add_child(view_ptr);
    view_manager::mainView.to_top(view_ptr);
    view_ptr->set_focus(true);
}

int32_t rect_distances(const ui::DIRECTION direction, const Rect &rect_from, const Rect &rect_to) {
    Coord direction_axis_end, direction_axis_start;
    bool aligned;

    switch (direction) {
        case ui::RIGHT:
            direction_axis_end = rect_to.left();
            direction_axis_start = rect_from.right();
            aligned = rect_from.top() < rect_to.bottom() && rect_from.bottom() > rect_to.top();
            break;

        case ui::LEFT:
            direction_axis_end = rect_from.left();
            direction_axis_start = rect_to.right();
            aligned = rect_from.top() < rect_to.bottom() && rect_from.bottom() > rect_to.top();
            break;

        case ui::UP:
            direction_axis_end = rect_from.top();
            direction_axis_start = rect_to.bottom();
            aligned = rect_from.right() > rect_to.left() && rect_from.left() < rect_to.right();
            break;

        case ui::DOWN:
            direction_axis_end = rect_to.top();
            direction_axis_start = rect_from.bottom();
            aligned = rect_from.right() > rect_to.left() && rect_from.left() < rect_to.right();
            break;

        default:
            return -1;
    }

    Coord on_axis_distance = direction_axis_end - direction_axis_start;
    if (!aligned || on_axis_distance < 0) {
        return -1;
    }

    Coord perpendicular_axis_start, perpendicular_axis_end;

    switch (direction) {
        case ui::RIGHT:
        case ui::LEFT:
            perpendicular_axis_start = rect_from.center().y();
            perpendicular_axis_end = rect_to.center().y();

            break;

        case ui::UP:
        case ui::DOWN:
            perpendicular_axis_start = rect_from.center().x();
            perpendicular_axis_end = rect_to.center().x();

            break;

        default:
            return -1;
    }

    auto abs_perp_dist = std::abs(perpendicular_axis_end - perpendicular_axis_start) + 1;
    auto axis_dist = on_axis_distance + 1;

    switch (direction) {
        case ui::RIGHT:
        case ui::LEFT:
            return abs_perp_dist * abs_perp_dist * axis_dist;
            break;

        case ui::UP:
        case ui::DOWN:
            return abs_perp_dist * axis_dist * axis_dist;
            break;

        default:
            return 0;
    }
}

// int32_t rect_center_distances(const ui::DIRECTION direction, const Rect &rect_from, const Rect &rect_to) {
//     Coord dx, dy;

//     Point p1 = rect_from.center();
//     Point p2 = rect_to.center();

//     switch (direction) {
//         case ui::RIGHT:
//             dx = p2.x() - p1.x();
//             dy = p2.y() - p1.y();
//             break;
//         case ui::DOWN:
//             dx = p2.x() - p1.x();
//             dy = p2.y() - p1.y();
//             break;
//         case ui::LEFT:
//             dx = p1.x() - p2.x();
//             dy = p1.y() - p2.y();
//             break;
//         case ui::UP:
//             dx = p1.x() - p2.x();
//             dy = p1.y() - p2.y();
//             break;

//         default:
//             return -1;
//     }

//     if (dx < 0 || dy < 0) {
//         return -1;
//     }

//     switch (direction) {
//         case ui::RIGHT:
//         case ui::LEFT:
//             return dx + dy * dy * dy;
//             break;

//         case ui::UP:
//         case ui::DOWN:
//             return dx * dx * dx + dy;
//             break;

//         default:
//             return 0;
//     }
// }

bool on_input(st_inputEvent &e) {
    bool consumed = currentView->on_input(e);
    if (!consumed && e.type == INPUT_EVENT_TYPE_ENCODER) {
        consumed = change_focus(currentView, e.value > 0 ? ui::RIGHT : ui::LEFT);
    }

    return consumed;
}

/*
 * Distance of a widget to a predecessor
 * Returns -1 if w is not under root
 */
int depth(Widget *w, Widget *root = nullptr) {
    if (w == root) {
        return 0;
    }
    int d = 1;
    while (w->parent() && w->parent() != root) {
        w = w->parent();
        ++d;
    }
    return !w && root ? -1 : d;
}

Widget *common_ancestor(Widget *a, Widget *b, Widget *root = nullptr) {
    if (!a || !b) {
        return nullptr;
    }

    int da = depth(a, root);
    int db = depth(b, root);

    if (da < 0 || db < 0) {
        return nullptr;
    }

    // Start from the deepest
    while (da > db) {
        a = a->parent();
        --da;
    }

    while (db > da) {
        b = b->parent();
        --db;
    }

    // If still not at the same ancestor
    while (a != b) {
        a = a->parent();
        b = b->parent();
    }

    return a; // First common ancestor
}

int distance_between(Widget *a, Widget *b) {
    Widget *p = common_ancestor(a, b);
    if (p) {
        return depth(a, p);
    }

    return -1;
}

bool change_focus(Widget *const top_widget, ui::DIRECTION direction, uint32_t max_levels) {

    bool changed = false;
    Widget *focused_widget = top_widget->focused_widget();

    //  LOG_IND(2, "change_focus: from widget %s | direction %d\n", focused_widget->get_name(), direction);

    if (focused_widget) {
        const auto focus_screen_rect = focused_widget->screen_rect();

        const auto test_fn = [&focus_screen_rect, direction, max_levels, focused_widget](Widget *const w) -> test_result_t {
            if (w->can_be_seen() && w->focusable() && w->enabled() && w != focused_widget) {

                int distance_to_ancestor = distance_between(focused_widget, w);

                if (max_levels > 0) {
                    if (distance_to_ancestor > max_levels) {
                        return {nullptr, 0};
                    }
                }

                auto distance = rect_distances(direction, focus_screen_rect, w->screen_rect());

                // distance = distance * distance_to_ancestor;

                //    LOG("Distance to %s: %d\n", w->get_name(), distance);
                if (distance >= 0) {
                    return {w, distance};
                }
            } else {
                //     LOG("%s not visible (%d) or not focusable (%d)\n", w->get_name(), w->focusable(), w->visible());
            }

            return {nullptr, 0};
        };

        std::vector<test_result_t> collection;
        collect_widgets(top_widget, test_fn, collection);

        //     LOG("Found %d focusable widgets\n", collection.size());

        const auto nearest = std::min_element(collection.cbegin(), collection.cend(), [](const test_result_t &a, const test_result_t &b) {
            return a.second < b.second;
        });

        // Up and left to indicate back

        if (nearest != collection.cend()) {
            //   LOG("Nearest widget is %s\n", nearest->first->get_name());
            changed = (*nearest).first->set_focus(true);
        } else {
            //    LOG("No nearest widget found\n");

            // Not using UP,DOWN keys, so when if no widget is focusable to the left or right, try going up or down
            if (direction == ui::LEFT) {
                changed = change_focus(top_widget, ui::UP);
            } else if (direction == ui::RIGHT) {
                changed = change_focus(top_widget, ui::DOWN);
            }
        }
    } else {
        //   LOG("No focused widget found\n");
    }

    //  LOG_IND_RAW(-2, "");
    return changed;
}

} // namespace view_manager
