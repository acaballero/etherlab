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

os::TaskManager task_manager;
} // namespace os
