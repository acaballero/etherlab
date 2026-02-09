#include "dsp_tasks.h"

namespace dsp {
const char *taskNames[] = {"Capture", "Replay", "Signal generator", "Receive", "Transmit", "APRS"};
const char *get_task_name(uint8_t id) {
    return id < DSP_TASK_N ? taskNames[id] : "-";
}
} // namespace dsp
