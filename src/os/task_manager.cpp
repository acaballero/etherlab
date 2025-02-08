/*
 * Filename: task_manager.cpp
 * Author: Angel Dust
 * Date: 2025-02-07
 */

#include "task_manager.h"
#include "algorithm"

namespace os {
void TaskManager::add(periodic_task *t) { tasks.push_back(t); }

void TaskManager::remove(periodic_task *t) { tasks.erase(std::remove(tasks.begin(), tasks.end(), t), tasks.end()); }

periodic_task *TaskManager::set_timeout(uint32_t delay, callback_t c) {

    // To create a timeout we set a period of the same length as the duration AND a delay, so the task will finish after its first execution
    periodic_task *task = new periodic_task(delay, c, delay, delay);
    add(task);
    return task;
}

void TaskManager::run() {

    for (auto it = tasks.begin(); it != tasks.end();) {

        periodic_task *task = *it;
        task->run();

        if (task->finished()) {
            remove(task);
        } else {
            it++;
        }
    }
}
} // namespace os
