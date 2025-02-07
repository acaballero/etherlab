/*
 * Filename: task_manager.cpp
 * Author: Angel Dust
 * Date: 2025-02-07
 */

#include "task_manager.h"
#include "algorithm"

void TaskManager::add(periodic_task *t) { tasks.push_back(t); }

void TaskManager::remove(periodic_task *t) { tasks.erase(std::remove(tasks.begin(), tasks.end(), t), tasks.end()); }

void TaskManager::run() {

    for (auto task : tasks) {
        task->loop();
    }
}
