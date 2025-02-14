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
#include <memory>

namespace os {
void TaskManager::add(periodic_task *t) { tasks.push_back(t); }

void TaskManager::remove(periodic_task *t) {
    tasks.erase(std::remove(tasks.begin(), tasks.end(), t), tasks.end());
    delete t;
}

periodic_task *TaskManager::set_timeout(uint32_t delay, callback_t c) {

    // To create a timeout we set a period of the same length as the duration AND a delay, so the task will finish after its first execution
    periodic_task *task = new periodic_task(delay, c, delay, delay);
    add(task);
    return task;
}

void TaskManager::run() {

    for (auto it = tasks.begin(); it != tasks.end();) {

        auto task = *it;
        task->run();

        if (task->finished()) {

            // char tmp[50];
            // printf_("Finished task: %s\n", task->get_log(tmp));
            remove(task);

        } else {
            it++;
        }
    }
}
} // namespace os
