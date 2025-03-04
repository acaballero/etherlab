/*
 * Filename: task_manager.h
 * Author: Angel Dust
 * Date: 2025-02-07
 */

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <cstdint>
#include <memory>
#include <vector>
#include "os/periodic_task.h"

namespace os {
class TaskManager {

  public:
    TaskManager(){};

    void add(periodic_task *t);
    void remove(periodic_task *t);
    void run();
    periodic_task *set_timeout(uint32_t delay, callback_t c);

  private:
    std::vector<std::unique_ptr<periodic_task>> tasks{};
    uint32_t count{0};
    uint8_t index{0};
};

extern TaskManager task_manager;

} // namespace os

#endif // TASK_MANAGER_H
