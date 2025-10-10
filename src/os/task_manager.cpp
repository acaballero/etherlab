/*
 * Filename: task_manager.cpp
 * Author: Angel Dust
 * Date: 2025-02-07
 */

#include "task_manager.h"
#include "algorithm"
#include "os/periodic_task.h"
#include "utils.hpp"
#include "printf.h"
#include <cstddef>
#include <memory>
#include "status.h"

#define WITH_PRIORITIES 1

namespace os {
int TaskManager::add(periodic_task *t) {

#if DEBUG_MSGS
    if (t->get_name()) {
        LOG("Adding task '%s'\n", t->get_name());
    }
#endif

    t->set_id(++last_id);
    tasks.push_back(std::unique_ptr<periodic_task>(t));

    return last_id;
}

bool TaskManager::remove(int task_id) {
    auto it = std::remove_if(tasks.begin(), tasks.end(), [task_id](const std::unique_ptr<periodic_task> &item) {
        return item->get_id() == task_id;
    });

    if (it != tasks.end()) {
        tasks.erase(it, tasks.end());
        return true;
    }
    return false;
}

bool TaskManager::remove(periodic_task *t) {

#if DEBUG_MSGS
    if (t->get_name()) {
        LOG("Removing task '%s'\n", t->get_name());
    }
#endif

    auto it = std::remove_if(tasks.begin(), tasks.end(), [t](const std::unique_ptr<periodic_task> &item) {
        return item.get() == t; // Compare raw pointers
    });

    if (it != tasks.end()) {
        tasks.erase(it, tasks.end()); // Erase the matching unique_ptr

        return true;
    }

    return false;
}

int TaskManager::set_timeout(uint32_t delay, callback_t c, char *name) {

    // To create a timeout we set a period of same length and duration AND a delay, so the task will finish after its first execution
    periodic_task *task = new periodic_task(delay, c, delay, delay, name);
    task->set_high_priority(true); // Timeouts must be respected
    return add(task);
}

#if !WITH_PRIORITIES
void TaskManager::run() {

    size_t i = 0;
    while (i < tasks.size()) {

        tasks[i]->run();

        if (tasks[i]->finished()) {
            remove(tasks[i].get());
        } else {
            i++;
        }
    }
}
#else
void TaskManager::run() {
    uint64_t current_time = HAL_GetTick();
    static size_t round_robin_index = 0;
    const uint32_t MAX_STARVATION_MS = 200;

    // Helper to check if a task is starving
    auto is_starving = [current_time](const auto &task) {
        return !task->is_high_priority() && task->ready_to_run(current_time) && (current_time - task->get_last_time()) >= MAX_STARVATION_MS;
    };

    // Generic task execution loop with predicate
    auto execute_tasks = [&](auto predicate, const char *log_prefix = "") {
        size_t i = 0;
        while (i < tasks.size()) {
            auto task = tasks[i].get();
            if (predicate(task, i)) {
#if DEBUG_MSGS
                if (task->get_name()) {
                    int elapsed = current_time - task->get_last_time();
                    LOG("%llu: Executing %s task '%s'", current_time, log_prefix, task->get_name());
                    LOG_RAW(": %llu ms, e: %d ms\n", task->get_period(), elapsed);
                }
#endif
                task->run();
                if (task->finished()) {
                    remove(task);
                    // Don't increment i since task was removed
                } else {
                    i++;
                }
            } else {
                i++;
            }
        }
    };

    // Check if any normal tasks are starving
    bool has_starving_tasks = std::any_of(tasks.begin(), tasks.end(), is_starving);

    if (has_starving_tasks) {
        // Execute all starving tasks
        execute_tasks(
            [&](const auto &task, size_t) {
                return is_starving(task);
            },
            "STARVING");
        return;
    }

    // Execute high-priority tasks
    execute_tasks(
        [&](const auto &task, size_t) {
            return task->is_high_priority() && task->ready_to_run(current_time);
        },
        "PRIORITY");

    for (size_t i = 0; i < tasks.size() && !tasks.empty(); i++) {
        round_robin_index %= tasks.size();
        if (!tasks[round_robin_index]->is_high_priority() && tasks[round_robin_index]->ready_to_run(current_time)) {

            // Use the same execution pattern
            size_t target_index = round_robin_index;
            execute_tasks([target_index](const auto &, size_t i) {
                return i == target_index;
            });

            // Handle round-robin advancement
            if (target_index < tasks.size()) {
                round_robin_index = (target_index + 1) % tasks.size();
            }
            return;
        }
        round_robin_index = (round_robin_index + 1) % tasks.size();
    }
}
#endif
os::TaskManager task_manager;
} // namespace os
