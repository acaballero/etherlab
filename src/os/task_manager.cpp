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
void TaskManager::add(periodic_task *t) { tasks.push_back(std::unique_ptr<periodic_task>(t)); }

void TaskManager::remove(periodic_task *t) {

    auto it = std::remove_if(tasks.begin(), tasks.end(), [t](const std::unique_ptr<periodic_task> &item) {
        return item.get() == t; // Compare raw pointers
    });

    if (it != tasks.end()) {
        tasks.erase(it, tasks.end()); // Erase the matching unique_ptr
    }
}

periodic_task *TaskManager::set_timeout(uint32_t delay, callback_t c) {

    // To create a timeout we set a period of the same length as the duration AND a delay, so the task will finish after its first execution
    periodic_task *task = new periodic_task(delay, c, delay, delay);
    add(task);
    return task;
}

void TaskManager::run() {

    size_t i = 0;
    while (i < tasks.size()) {

        tasks[i]->run();

        if (tasks[i]->finished()) {
            tasks.erase(tasks.begin() + i);
        } else {
            i++;
        }
    }
}

os::TaskManager task_manager;
} // namespace os
