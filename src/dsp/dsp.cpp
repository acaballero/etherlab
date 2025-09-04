//
// Created by Angel Dust on 04/04/2021.
//
#include "dsp.h"
#include "arm_math.h"
#include "blocks/dc_block.h"
#include "config.h"
#include "diskio.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_config.h"
#include "dsp/fft/fft.h"
#include "dsp/fft/fft_types.h"
#include "handlers.h"
#include "radio.h"
#include "status.h"
#include "stm32f4xx_hal.h"
#include "types.h"
#include "ui/lcd.h"
#include "hw/stm32f4xx/timers.h"
#include "utils.hpp"
#include <cstddef>
#include <cstring>
#include <functional>
#include <sys/_stdint.h>

#if ENABLE_SD_CARD

#include "../fatfs/fatfs.h"
#include "../../lib/FatFs/ff.h"

#endif

#include "hw/stm32f4xx/adc.h"
#include "dsp_tasks.h"
#include "dsp_processors.h"
#include "dsp_config.h"
#include "buffer.hpp"
#include "dsp_buffers.h"
#include "main_board.h"
#include "printf.h"

void dsp_loop();
void dsp_stop();

namespace dsp {
os::periodic_task task(50, dsp_loop, 0, 0, "loop");
bool adc_overload{false};

bool apply_audio_bpf() {
    return dsp::dsp_config.audio_bpf_enabled;
}
bool apply_deemph(MODULATION_MODE mod) {
    return dsp::dsp_config.deemphasis_enabled && (mod == FM || mod == WFM);
}
bool apply_compression(MODULATION_MODE mod) {
    return dsp::dsp_config.audio_compressor_enabled && (mod == AM || mod == SSB_USB || mod == SSB_LSB);
}
} // namespace dsp

Task *current_task;
DspProcessor *current_processor;
buffer_t<adc_type> *current_buffer;
dsp::st_dsp_command pending_command{DSP_COMMAND_NONE};
DSP_STATUS dspstatus;
uint64_t overload_history{0};
bool check_overload_pending{false};
#if !EXECUTE_TASKS_ON_INTERRUPT
volatile bool execute_task = false;
#endif

DCBlock dc_block_i{0.999};
DCBlock dc_block_q{0.999};

std::function<void(st_dsp_status *)> on_event;

/** Sets or unsets the real-time DSP mode, for which only one slice of FFT can be used **/
void dsp_set_real_time(bool b) {

    // LOG("Setting real time: %d\n", b);
    //  Update FFT and sample rate parameters
    dsp::set_max_sample_freq(b);

    if (b) {
        // When doing real-time DSP, we can only process one slice (no frequency hops allowed)
        fft::current_max_slices = 1;
    } else {
        fft::current_max_slices = config.fft.max_slices;
    }

    fft_config(config.fft.span);
}

void restart_callback(void *, void *) {

    //  TODO: This assumes the current task is 'receive'
    if (config.mode != DIGITAL_RX && current_task && dsp::dsp_status && dsp::dsp_status->status == DSP_STATUS_RUNNING) {
        //   LOG("restart_callback: sending stop receive commandxs\n");
        dsp_command({(DSP_COMMAND)DSP_COMMAND_STOP, dsp::DSP_TASK_RECEIVE}, nullptr);
    } else if (config.mode == DIGITAL_RX && !current_task) {
        //   LOG("restart_callback: sending start receive command\n");
        dsp_command({(DSP_COMMAND)DSP_COMMAND_START, dsp::DSP_TASK_RECEIVE}, nullptr);
    } else {
        //   LOG("restart_callback:dsp_restart\n");
        dsp_restart();
    }
}

void dsp_init(dsp::st_dsp_config &config) {

    dsp::set_config(config);
    ADC_DMA_Start(&hadc1);
    dsp::set_max_sample_freq(false);
    main_board::mode_signal.add(nullptr, restart_callback); // modulation or mode changed
    fft::signal.add(nullptr, restart_callback);             // fft params changed
    restart_callback(nullptr, nullptr);                     // First time, in case we start in DSP mode and miss initial signals
}

// void dsp_stop_tasks() {
//     LOG("dsp_stop_tasks");
//     if (current_task) {
//         LOG(": stopping current task\n");
//         current_task->stop();
//         //   current_task = nullptr;
//         if (on_event) {
//             on_event(dsp::dsp_status);
//         }
//     } else {
//         LOG(": no current task\n");
//     }
// }

uint8_t dsp_command(dsp::st_dsp_command command, std::function<void(st_dsp_status *)> cb) {

    LOG("dsp_command: cmd:%d, id:%d\n", (int)command.command, command.id);
    Task *task = command.task ? command.task : dsp::tasks[command.id];
    if (current_task == task) {
        DSP_STATUS s = current_task->status.status;
        if ((command.command == DSP_COMMAND_START && s == DSP_STATUS_RUNNING) || (command.command == DSP_COMMAND_STOP && s == DSP_STATUS_STOPPED) ||
            s == DSP_STATUS_PENDING) {
            LOG("WARN: Skipping command %d: current task status: %d\n", command.command, s);
            return 1;
        }
    }

    on_event = cb;
    pending_command = command;
    current_task = task;
    current_task->status.status = DSP_STATUS_PENDING;
    current_processor->status.status = DSP_STATUS_PENDING;
    current_task->status.id = pending_command.id;

    // FIXME: Ugly
    dsp::dsp_status = &current_task->status;

    return 0;
}

void dsp_start_task() {
    LOG("dsp_start_task");

    //  current_task = tasks[pending_command.id];
    //  dsp_status = &current_task->status;
    if (!dsp::dsp_status || dsp::dsp_status->status != DSP_STATUS_RUNNING) {

        LOG_RAW(": not running, will start\n");

        input_stream.reset();
        output_stream.reset();

        current_buffer->sample_rate = current_task->status.sample_rate;

        current_processor = processors[pending_command.id];
        current_processor->reset(); // In case wasn't property stopped from an earlier run (which should be avoided, by the way)

        if (current_processor->status.direction == DSP_DIRECTION_OUT) {
            // Link the start of the processor with the 1st block processed event of the task to prevent false underruns
            current_task->on_first_block = []() {
                //     LOG("On first block\n");

                current_processor->start();
            };
        }

        //  LOG("dsp_start_task: starting task\n");
        current_task->status.reset();
        if (current_task->start()) {

            // TODO: Ugly!
            current_processor->status.block_size_bytes = current_task->status.block_size_bytes;
            current_processor->status.bandwidth = current_task->status.bandwidth;
            current_processor->status.sample_rate = current_task->status.sample_rate;
            current_processor->status.decimation_factor = current_task->status.decimation_factor;
            current_processor->status.decimated_block_size = current_task->status.decimated_block_size;
            current_processor->status.decimated_block_size_bytes = current_task->status.decimated_block_size_bytes;
            current_processor->status.n_channels = current_task->status.n_channels;

            if (current_processor->status.direction != DSP_DIRECTION_OUT) {
                bool ok = current_processor->start();
                if (!ok) {
                    status::handleError(status::ST_ERROR, "Error starting DSP processor");
                    return;
                }
            } else {
                current_processor->status.status = DSP_STATUS_PENDING;
            }

            dsp::dsp_status = current_task->status.direction != DSP_DIRECTION_IN ? &current_processor->status : &current_task->status;
            if (on_event) {
                on_event(dsp::dsp_status);
            }
        } else {
            status::handleError(status::ST_ERROR, "Error starting DSP task");
        }
    } else {
        //   LOG(": alerady running task\n");
    }
}

bool dsp_restart() {
    // LOG("dsp_restart");
    if (!ISANALOG && current_task && dsp::dsp_status && dsp::dsp_status->status == DSP_STATUS_RUNNING) {

        //   LOG(": will restart\n");
        current_task->status.status = DSP_STATUS_PENDING;
        current_processor->status.status = DSP_STATUS_PENDING;
        DAC_DMA_Stop(&hdac1);
        ADC_DMA_Stop(&hadc1);
        dsp_start_task();
        return true;
    } else if (!current_task) {
        //   LOG(": wont restart: no task\n");
    } else if (current_task && dsp::dsp_status) {
        int b = dsp::dsp_status->status == DSP_STATUS_RUNNING ? 0 : 1;
        //   LOG(": wont restart: current task is running=%d\n", b);
    }

    return false;
}

inline void check_overload() {
    adc_type max;
    uint32_t max_ix;
    // Get the max for overload detection
    arm_max_q15((q15_t *)current_buffer->p, current_buffer->count, &max, &max_ix);

    overload_history = (overload_history << 1) | (max > fft::adc_max_ampl ? 1 : 0);
    // char buff[65];
    // int_to_binary(overload_history, buff, 64);
    // LOG("ADC overload history:%s\n", buff);
    dsp::adc_overload = overload_history > 0;
}

void dsp_loop() {

    dsp::st_dsp_command command = pending_command;

    if (command.command != DSP_COMMAND_NONE) {
        switch (command.command) {

            case DSP_COMMAND_START:

                dsp_start_task();
                break;

            case DSP_COMMAND_STOP:

                dsp_stop();
                break;
            default:
                assert(pending_command.command != DSP_COMMAND_NONE);
                break;
        }

        if (pending_command == command) { // Another command may have been queued
            pending_command.command = DSP_COMMAND_NONE;
        }
    }

    check_overload_pending = true; // Check ADC overload in next adquisition

#if !EXECUTE_TASKS_ON_INTERRUPT
    if (execute_task) {
        current_task->work();
        execute_task = false;
    }
#endif
}

//__attribute__((section(".ccmram")))
inline void dac_work() {
    // GPIOD->BSRR |= GPIO_PIN_5;

    if (current_processor && (current_processor->status.direction == DSP_DIRECTION_OUT || current_processor->status.direction == DSP_DIRECTION_INOUT)) {
        current_processor->work(current_buffer);
    }

    if (dsp::dsp_status && dsp::dsp_status->direction == DSP_DIRECTION_OUT) {
        // DSP TX direction: The FFT is fed from the produced data stream
        FIFO_ERROR err = fft_fifo.write_block((char *)current_buffer->p, current_buffer->size_bytes);
        UNUSED(err);
    }

    // GPIOD->BSRR |= GPIO_PIN_5 << 16;
}

inline void adc_work() {
    // GPIOD->BSRR |= GPIO_PIN_9;

    if (check_overload_pending) {
        check_overload(); // This has to be done here since this buffer can be quickly dc-blocked in-place and the overload reference range is only positive
        check_overload_pending = false;
    }

#if DSP_FS4_SHIFT

    if (dsp::get_freq_shift_enabled()) {

        // Removing DC here and thus not having to do it in subsequent stages (FFT, DSP processing) is not as efficient as it seems at first glance since:
        // - FFT processing is not done in real-time so no need to do the work for it
        // - Receivers, for example, could remove DC just before demodulation, at a much lower sample rate but, in fact, after shifting, the DC spike is
        //   removed by the low pass filters
        // The drawback is we have to rotate it there (fft module)too after DC removal

        buffer_t<adc_type> bb = {(adc_type *)current_buffer->p, DSP_BLOCK * 2};
        dc_block_i.filter(bb, 2, 0);
        dc_block_q.filter(bb, 2, 1);

        if (fft_params.decimation_factor > 1) {
            // TODO: Decimate here vs in both FFT and current DSP task?
        }

        dsp::rotate_fs4_q15((const q15_t *)current_buffer->p, (q15_t *)current_buffer->p, DSP_BLOCK);
    }

    if (!dsp::dsp_status || dsp::dsp_status->direction == DSP_DIRECTION_IN || dsp::dsp_status->direction == DSP_DIRECTION_INOUT) {

        // If the direction is input or bidirectional...

        // Fill the FFT FIFO. Here we don't care if we overrun (returns error) as the FFT doesn't need to be processed in real-time
        // TODO: write to the FFT FIFO in a separate DspProcessor

        fft_fifo.write_block((char *)current_buffer->p, current_buffer->size_bytes);

        // char *d;
        // static uint32_t last_t;
        // uint32_t t = HAL_GetTick();

        // if (fft_fifo.available(&d) >= 64 && t - last_t > 1000) {

        //     dsp::log_buff((adc_type *)d, 64, "", true);
        //     last_t = t;
        // }
    }

#endif

    if (current_processor && (current_processor->status.direction == DSP_DIRECTION_IN || current_processor->status.direction == DSP_DIRECTION_INOUT)) {
        current_processor->work(current_buffer);
    }

    // GPIOD->BSRR |= GPIO_PIN_9 << 16;
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *) {
    current_buffer = &dac_buffer_2;
    dac_work(); // Process the 2nd half of the buffer
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *) {
    current_buffer = &dac_buffer_1;
    dac_work(); // Process the 1st half of the buffer
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *) {
    current_buffer = &adc_buffer_2;
    adc_work(); // Process the 2nd half of the buffer
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *) {

    current_buffer = &adc_buffer_1;
    adc_work(); // Process the 1st half of the buffer
}

void TIM8_TRG_COM_TIM14_IRQHandler(void) {
    // void TIM1_BRK_TIM15_IRQHandler(void) {

    // Prevent interrupting the LCD drawing phase or the display DMA will overrun
    // TODO: Consider a different approach, as (also) lowering the refresh ratio while doing any critical DSP task, or
    // disabling EXECUTE_TASKS_ON_INTERRUPT
    if (lcd.can_interrupt()) {
        if (current_task) {
#if EXECUTE_TASKS_ON_INTERRUPT
            current_task->work();
#else
            execute_task = true;
#endif
        }
    } else {
        // LOG("Task interrupt skipped\n");
    }

    HAL_TIM_IRQHandler(&TASKS_TIMER_HANDLE);
}

void dsp_test_cb(st_dsp_status *) {

    if (dsp::dsp_status->error == DSP_ERR_NONE && dsp::dsp_status->id != dsp::DSP_TASK_REPLAY) {
        dsp_command({DSP_COMMAND_START, dsp::DSP_TASK_REPLAY}, dsp_test_cb);
    }
}

#include "../ui/menu.h"

void dsp_stop() {

    if (dspstatus != DSP_STATUS_STOPPING) {

        dspstatus = DSP_STATUS_STOPPING;
        // Clear DAC buffer.
        // TODO: If we don't clear it first thing after the process is done and before DMA interrupts cease, a repeating buffer will appear at the DAC. However,
        // there are other approaches I need to explore. E.g. flushing a "zero tail" in the output buffer so the constraints over the timing of the
        // multiple objects that are stopped is not that critical

        for (int i = 0; i < DSP_BLOCK * 2; i++) {
            dac_buff[i] = (adc_type)config.hw.dac_offset;
        }

        LOG("dspStop\n");
        if (current_processor) {
            LOG("dspStop:processor stop\n");
            current_processor->stop();
        }

        if (current_task) {
            LOG("dspStop:task stop\n");
            current_task->stop();
        }

        current_task = NULL;
        current_processor = NULL;

#if !EXECUTE_TASKS_ON_INTERRUPT
        execute_task = false;
#endif

        if (on_event) {
            //  LOG("dspStop: onEvent\n");
            on_event(dsp::dsp_status);
        }

        dsp::dsp_status = NULL;

        dspstatus = DSP_STATUS_STOPPED;

        if (!ISANALOG) {
            // TODO: This forces the receive task to start again. But its ugly
            fft_config(fft_params.span);
        }
    }
}

void dspSuccess() {
    // LOG("dspSuccess\n");
    dsp_stop();
}

void dspError(DSP_ERROR err) {

    // LOG("dspError\n");
    switch (err) {

        case DSP_ERR_FILEOPEN:
            handleError(status::ST_ERROR, "Wave file open error");
            break;
        case DSP_ERR_FILECLOSE:
            handleError(status::ST_ERROR, "Wave file close error");
            break;
        case DSP_ERR_FILEWRITE:
            handleError(status::ST_ERROR, "Wave write error");
            break;
        case DSP_ERR_FILEREAD:
            handleError(status::ST_ERROR, "Wave read error");
            break;
        case DSP_ERR:
            handleError(status::ST_ERROR, "DSP error");
            break;
        case DSP_ERR_DMAOVERRUN:
            handleError(status::ST_ERROR, "DMA overrun");
            break;
        case DSP_ERR_FIFO_OVERRUN:
            handleError(status::ST_ERROR, "FIFO overrun");
            break;
        case DSP_ERR_FIFO_UNDERRUN:
            handleError(status::ST_ERROR, "FIFO underrun");
            break;
        default:
            break;
    }

    dsp_stop();
}
