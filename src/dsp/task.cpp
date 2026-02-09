//
// Created by Angel Dust on 16/04/2021.
//

#include "hw/stm32.h"
#include "task.h"
#include "dsp_buffers.h"

Task::Task(void (*onSucess)(), void (*onError)(DSP_ERROR)) {
    on_error = onError;
    on_success = onSucess;
}

void Task::reset() {
    info.reset();
}

void Task::halt(DSP_ERROR e) {
    info.error = e;
    stop();
}

// Template method that orchestrates startup sequence
bool Task::start() {
    // Task-specific initialization
    if (!start_impl()) {
        return false;
    }

    if (!start_processor()) {
        return false;
    }

    return true;
}

bool Task::start_impl() {
    info.status = DSP_STATUS_RUNNING;
    reset();
    info.start_ms = HAL_GetTick();
    return true;
}

// Common processor startup logic
bool Task::start_processor() {

    if (!processor) {
        processor = create_processor();
    }

    processor.get()->reset();

    // Sync params from task to processor
    processor->info.block_size_bytes = info.block_size_bytes;
    processor->info.bandwidth = info.bandwidth;
    processor->info.sample_rate = info.sample_rate;
    processor->info.decimation_factor = info.decimation_factor;
    processor->info.decimated_block_size = info.decimated_block_size;
    processor->info.decimated_block_size_bytes = info.decimated_block_size_bytes;
    processor->info.n_channels = info.n_channels;

    // Setup deferred processor start for output paths to prevent underruns
    if (processor->wait_first_block()) {
        on_first_block = [this]() {
            LOG("First block ready: starting processor\n");
            processor->start();
        };
    } else {
        // Start processor immediately unless waiting for first block

        LOG("Starting processor\n");
        if (!processor->start()) {
            status::pop_alert(status::ERROR, "Error starting DSP processor");
            return false;
        }
    }

    return true;
}

void Task::stop() {

    info.status = DSP_STATUS_STOPPED;
    info.stop_ms = HAL_GetTick();

    if (processor) {
        processor->stop();
    }

    if (info.error) {
        if (on_error) {
            on_error(info.error);
        }
    } else {
        if (on_success) {
            on_success();
        }
    }
}
