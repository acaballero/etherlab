//
// Created by Angel Dust on 16/04/2021.
//
#include "config.h"

Config config;

namespace configuration {

void saveConfig() {}

periodic_task task(CONFIG_AUTOSAVE_SECS * 1000, saveConfig);

} // namespace configuration
