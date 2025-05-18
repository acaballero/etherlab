//
// Created by Angel Dust on 16/04/2021.
//
#include "config.h"

Config config;

namespace configuration {

void saveConfig() {
}

// TODO: Periodic save only on SDcard. But mind this will halt everything unless done with DMA and interrupts.
os::periodic_task task(CONFIG_AUTOSAVE_SECS * 1000, saveConfig);

} // namespace configuration
