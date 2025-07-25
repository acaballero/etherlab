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

namespace os {
int TaskManager::add(periodic_task *t) {
    t->set_id(++last_id);
    tasks.push_back(std::unique_ptr<periodic_task>(t));

    return last_id;
}

bool TaskManager::remove(int task_id) {
    auto it = std::remove_if(tasks.begin(), tasks.end(), [task_id](const std::unique_ptr<periodic_task> &item) {
        return item->get_id() == task_id;
    });

    return remove(it->get());
}

bool TaskManager::remove(periodic_task *t) {

    auto it = std::remove_if(tasks.begin(), tasks.end(), [t](const std::unique_ptr<periodic_task> &item) {
        return item.get() == t; // Compare raw pointers
    });

    if (it != tasks.end()) {
        tasks.erase(it, tasks.end()); // Erase the matching unique_ptr

        return true;
    }

    return false;
}

periodic_task *TaskManager::set_timeout(uint32_t delay, callback_t c) {

    // To create a timeout we set a period of same length and duration AND a delay, so the task will finish after its first execution
    periodic_task *task = new periodic_task(delay, c, delay, delay);
    add(task);
    return task;
}

// void TaskManager::run() {

//     size_t i = 0;
//     while (i < tasks.size()) {

//         tasks[i]->run();

//         if (tasks[i]->finished()) {
//             remove(tasks[i].get());
//         } else {
//             i++;
//         }
//     }
// }

void TaskManager::run() {

    uint64_t current_time = HAL_GetTick();
    static size_t round_robin_index = 0;
    static uint32_t last_normal_execution = current_time;
    const uint32_t MAX_STARVATION_MS = 100; // Guarantee service of low priority tasks every 100ms to prevent starvation

    bool force_normal_task = (current_time - last_normal_execution) >= MAX_STARVATION_MS;

    if (!force_normal_task) {
        // Check high-priority tasks first (only if we're not forcing normal tasks)
        for (const auto &task : tasks) {
            if (task->is_high_priority() && task->ready_to_run(current_time)) {
                task->run();
                if (task->finished()) {
                    remove(task.get());
                }
                return;
            }
        }
    }

    // Execute one normal-priority task (round-robin)
    size_t attempts = 0;
    while (attempts < tasks.size()) {
        if (round_robin_index >= tasks.size()) {
            round_robin_index = 0;
        }

        const auto &task = tasks[round_robin_index];
        if (!task->is_high_priority() && task->ready_to_run(current_time)) {
            task->run();
            last_normal_execution = current_time; // Reset starvation timer
            if (task->finished()) {
                remove(task.get());
            } else {
                round_robin_index++;
            }
            return;
        }

        round_robin_index++;
        attempts++;
    }
}

os::TaskManager task_manager;
} // namespace os
