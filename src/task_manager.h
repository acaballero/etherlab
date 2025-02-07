/*
 * Filename: task_manager.h
 * Author: Angel Dust
 * Date: 2025-02-07
 */

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <cstdint>
#include <vector>
#include "periodic_task.h"

class TaskManager {

  public:
    TaskManager(){};

    void add(periodic_task *t);
    void remove(periodic_task *t);
    void run();

  private:
    std::vector<periodic_task *> tasks{};
    uint32_t count{0};
    uint8_t index{0};
};

#endif // TASK_MANAGER_H
