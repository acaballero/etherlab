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
#include "handlers.h"
#include "radio.h"
#include "status.h"
#include "types.h"
#include "ui/lcd.h"
#include "hw/stm32f4xx/timers.h"
#include <cstddef>
#include <cstring>
#include <functional>

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
namespace dsp {
os::periodic_task task(50, dsp_loop);

} // namespace dsp
Task *current_task;
DspProcessor *current_processor;
buffer_t<complex_t> *current_buffer;
dsp::st_dsp_command pending_command{DSP_COMMAND_NONE};

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
        LOG("restart_callback: sending stop receive commandxs\n");
        dsp_command({(DSP_COMMAND)DSP_COMMAND_STOP, dsp::DSP_TASK_RECEIVE}, nullptr);
    } else if (config.mode == DIGITAL_RX && !current_task) {
        LOG("restart_callback: sending start receive command\n");
        dsp_command({(DSP_COMMAND)DSP_COMMAND_START, dsp::DSP_TASK_RECEIVE}, nullptr);
    } else {
        LOG("restart_callback:dsp_restart\n");
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

void dsp_stop_tasks() {
    LOG("dsp_stop_tasks");
    if (current_task) {
        LOG(": stopping current task\n");
        current_task->stop();
        //   current_task = nullptr;
        if (on_event) {
            on_event(dsp::dsp_status);
        }
    } else {
        LOG(": no current task\n");
    }
}

uint8_t dsp_command(dsp::st_dsp_command command, std::function<void(st_dsp_status *)> cb) {

    // LOG("dsp_command: cmd:%d, id:%d\n", (int)command.command, command.id);
    Task *task = command.task ? command.task : dsp::tasks[command.id];
    if (current_task == task) {
        DSP_STATUS s = current_task->status.status;
        if ((command.command == DSP_COMMAND_START && s == DSP_STATUS_RUNNING) || (command.command == DSP_COMMAND_STOP && s == DSP_STATUS_STOPPED) ||
            s == DSP_STATUS_PENDING) {
            return 1;
        }
    }

    on_event = cb;
    pending_command = command;
    current_task = task;
    current_task->status.status = DSP_STATUS_PENDING;
    current_task->status.id = pending_command.id;

    // FIXME: Ugly
    dsp::dsp_status = &current_task->status;

    return 0;
}

bool dsp_restart() {
    // LOG("dsp_restart");
    if (!ISANALOG && current_task && dsp::dsp_status && dsp::dsp_status->status == DSP_STATUS_RUNNING) {

        //   LOG(": will restart\n");
        dsp::dsp_status->status = DSP_STATUS_PENDING;
        DAC_DMA_Stop(&hdac1);
        ADC_DMA_Stop(&hadc1);
        input_stream.reset();
        output_stream.reset();
        current_task->start();
        dsp::dsp_status->reset();
        return true;
    } else if (!current_task) {
        //   LOG(": wont restart: no task\n");
    } else if (current_task && dsp::dsp_status) {
        int b = dsp::dsp_status->status == DSP_STATUS_RUNNING ? 0 : 1;
        //   LOG(": wont restart: current task is running=%d\n", b);
    }

    return false;
}

void dsp_start_task() {
    //    LOG("dsp_start_task");

    //  current_task = tasks[pending_command.id];
    //  dsp_status = &current_task->status;
    if (!dsp::dsp_status || dsp::dsp_status->status != DSP_STATUS_RUNNING) {

        //   LOG(": not running, will start\n");

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
        current_task->start();

        // TODO: Ugly!
        current_processor->status.block_size_bytes = current_task->status.block_size_bytes;
        current_processor->status.decimation_factor = current_task->status.decimation_factor;
        current_processor->status.decimated_block_size = current_task->status.decimated_block_size;
        current_processor->status.decimated_block_size_bytes = current_task->status.decimated_block_size_bytes;
        current_processor->status.n_channels = current_task->status.n_channels;

        if (current_processor->status.direction != DSP_DIRECTION_OUT) {
            current_processor->start();
        }

        dsp::dsp_status = current_task->status.direction != DSP_DIRECTION_IN ? &current_processor->status : &current_task->status;
        if (on_event) {
            on_event(dsp::dsp_status);
        }
    } else {
        //   LOG(": alerady running task\n");
    }
}

void dsp_loop() {

    dsp::st_dsp_command command = pending_command;

    if (command.command != DSP_COMMAND_NONE) {
        switch (command.command) {

            case DSP_COMMAND_START:

                dsp_start_task();
                break;

            case DSP_COMMAND_STOP:

                dsp_stop_tasks();
                break;
            default:
                assert(pending_command.command != DSP_COMMAND_NONE);
                break;
        }

        if (pending_command == command) {
            pending_command.command = DSP_COMMAND_NONE;
        }
    }

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
#if DSP_FS4_SHIFT

    if (dsp::get_freq_shift_enabled()) {

        buffer_t<adc_type> bb = {(adc_type *)current_buffer->p, DSP_BLOCK * 2};
        dc_block_i.filter(bb, 2, 0);
        dc_block_q.filter(bb, 2, 1);

        if (fft_params.decimation_factor > 1) {
        }

        dsp::rotate_fs4_q15((const q15_t *)current_buffer->p, (q15_t *)current_buffer->p, current_buffer->count);
    }

#endif

    if (!dsp::dsp_status || dsp::dsp_status->direction == DSP_DIRECTION_IN || dsp::dsp_status->direction == DSP_DIRECTION_INOUT) {

        // If the direction is input or bidirectional...

        // Fill the FFT FIFO. Here we don't care if we overrun as the FFT doesn't need to be processed in real-time
        // TODO: write to the FFT FIFO in a separate DspProcessor

        FIFO_ERROR err = fft_fifo.write_block((char *)current_buffer->p, current_buffer->size_bytes);
        UNUSED(err);
    }

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
    if (!lcd.busy) {

        if (current_task) {
#if EXECUTE_TASKS_ON_INTERRUPT
            current_task->work();
#else
            execute_task = true;
#endif
        }
    }

    HAL_TIM_IRQHandler(&TASKS_TIMER_HANDLE);
}

void dsp_test_cb(st_dsp_status *) {

    if (dsp::dsp_status->error == DSP_ERR_NONE && dsp::dsp_status->id != dsp::DSP_TASK_REPLAY) {
        dsp_command({DSP_COMMAND_START, dsp::DSP_TASK_REPLAY}, dsp_test_cb);
    }
}

#include "../ui/menu.h"

void dspStop() {

    // Clear DAC buffer.
    // TODO: If we don't clear it first thing after the process is done and before DMA interrupts cease, a repeating buffer will appear at the DAC. However,
    // there are other approaches I need to explore. E.g. flushing a "zero tail" in the output buffer so the constraints over the timing of the
    // multiple objects that are stopped is not that critical
    for (int i = 0; i < DSP_BLOCK * 2; i++) {
        dac_buff[i] = {{(adc_type)config.hw.dac_offset, (adc_type)config.hw.dac_offset}};
    }

    //  LOG("dspStop\n");
    if (current_processor) {
        current_processor->stop();
    }

    if (current_task) {
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
}

void dspSuccess() {
    // LOG("dspSuccess\n");
    dspStop();
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

    dspStop();
}
