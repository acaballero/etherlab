//
// Created by Angel Dust on 04/04/2021.
//
#include "dsp.h"
#include "diskio.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_config.h"
#include "status.h"
#include "types.h"
#include "ui/lcd.h"
#include "hw/stm32f4xx/timers.h"

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

void dsp_loop();
namespace dsp {
os::periodic_task task(50, dsp_loop);

} // namespace dsp
Task *current_task;
DspProcessor *current_processor;
buffer_t<complex_t> *current_buffer;
st_dspCommand pending_command{DSP_COMMAND_NONE};

#if !EXECUTE_TASKS_ON_INTERRUPT
volatile bool execute_task = false;
#endif

void (*on_event)(st_dspStatus *);

uint8_t current_max_slices = config.fft.max_slices;

void dsp_set_real_time(bool b) {

    // Update FFT and sample rate parameters
    set_max_sample_freq(b);

    if (b) {
        // When doing real-time DSP, we can only process one slice (no frequency hops allowed)
        current_max_slices = config.fft.max_slices;
        config.fft.max_slices = 1;
    } else {
        config.fft.max_slices = current_max_slices;
    }

    fft_config(config.fft.span);
}

void dsp_init(dsp::st_dsp_config &config) {
    dsp::set_config(config);
    ADC_DMA_Start(&hadc1);
    set_max_sample_freq(false);
}

void dsp_stop_tasks() {
    if (current_task) {
        current_task->stop();
    }
}

uint8_t dsp_command(st_dspCommand command, void (*cb)(st_dspStatus *)) {

    Task *task = dsp::tasks[command.id];
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
    dsp_status = &current_task->status;

    return 0;
}

bool dsp_restart() {
    if (current_task && dsp_status && dsp_status->status == DSP_STATUS_RUNNING) {
        current_task->start();
        return true;
    }

    return false;
}

void dsp_start_task() {

    //  current_task = tasks[pending_command.id];
    //  dsp_status = &current_task->status;
    if (dsp_status->status != DSP_STATUS_RUNNING) {
        current_task->start();
        current_processor = processors[pending_command.id];
        current_processor->status.block_size_bytes = current_task->status.block_size_bytes;
        current_processor->status.decimation_factor = current_task->status.decimation_factor;
        current_processor->status.decimated_block_size = current_task->status.decimated_block_size;
        current_processor->status.decimated_block_size_bytes = current_task->status.decimated_block_size_bytes;
        current_processor->status.n_channels = current_task->status.n_channels;
        current_processor->start();
        current_buffer->sample_rate = current_task->status.sample_rate;

        dsp_status = current_task->status.id == dsp::DSP_TASK_RECEIVE ? &current_processor->status : &current_task->status;

        if (on_event) {
            on_event(dsp_status);
        }
    }
}

void dsp_loop() {

    st_dspCommand command = pending_command;

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

    if (dsp_status && dsp_status->direction == DSP_DIRECTION_OUT) {
        FIFO_ERROR err = fft_fifo.writeBlock((char *)current_buffer->p, current_buffer->size_bytes);
        UNUSED(err);
    }

    // GPIOD->BSRR |= GPIO_PIN_5 << 16;
}

inline void adc_work() {
    // GPIOD->BSRR |= GPIO_PIN_5;

    if (!dsp_status || dsp_status->direction == DSP_DIRECTION_IN || dsp_status->direction == DSP_DIRECTION_INOUT) {
        // If the IF chain direction is input, the

        // Fill the FFT FIFO. Here we don't care if we overrun as the FFT doesn't need to be processed in real-time
        // TODO: write to the FFT FIFO in a separate DspProcessor
        FIFO_ERROR err = fft_fifo.writeBlock((char *)current_buffer->p, current_buffer->size_bytes);
        UNUSED(err);
    }

    if (current_processor && (current_processor->status.direction == DSP_DIRECTION_IN || current_processor->status.direction == DSP_DIRECTION_INOUT)) {
        current_processor->work(current_buffer);
    }

    // if (dsp_status && dsp_status->direction == DSP_DIRECTION_OUT) {
    //     FIFO_ERROR err = fft_fifo.writeBlock((char *)current_buffer->p, current_buffer->size_bytes);
    //     UNUSED(err);
    // }

    // GPIOD->BSRR |= GPIO_PIN_5 << 16;
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
    /* This is called after half the conversion is completed */

    // TODO: Check ADC buffer overruns
    current_buffer = &adc_buffer_2;
    adc_work(); // Process the 2nd half of the buffer
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *) {

    current_buffer = &adc_buffer_1;
    adc_work(); // Process the 1st half of the buffer

    // If DMAContinuousConversion is disabled, the adc buffer will be written only once, and HAL_ADC_Start_DMA
    // should be called in order to start again after this callback
    // If DMAContinuousConversion is enabled, there's no need to restart the ADC and DMA by calling HAL_ADC_Start_DMA,
    // the ADC will remain active and triggered by the timer (or by software, whichever method is used) after this callback.
    // HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_buffer, 8);
}

void TIM8_TRG_COM_TIM14_IRQHandler(void) {
    // void TIM1_BRK_TIM15_IRQHandler(void) {

    // Prevent interrupting the LCD drawing phase or the DMA will overrun the buffer
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

    /* USER CODE END TIM1_BRK_TIM15_IRQn 0 */
    HAL_TIM_IRQHandler(&TASKS_TIMER_HANDLE);
    /* USER CODE BEGIN TIM1_BRK_TIM15_IRQn 1 */

    /* USER CODE END TIM1_BRK_TIM15_IRQn 1 */
}

void dsp_test_cb(st_dspStatus *) {

    if (dsp_status->error == DSP_ERR_NONE && dsp_status->id != dsp::DSP_TASK_REPLAY) {
        dsp_command({DSP_COMMAND_START, dsp::DSP_TASK_REPLAY}, dsp_test_cb);
    }
}

#include "../ui/menu.h"

void dspStop() {

    if (current_processor) {
        current_processor->stop();
    }

    if (current_task) {
        current_task->stop();
    }

    if (current_processor && current_task) {
        //        if (current_processor->status.direction == DSP_DIRECTION_IN) {
        //            current_task->status.fifo_overruns = current_processor->status.fifo_overruns;
        //        } else {
        //            current_task->status.fifo_underruns = current_processor->status.fifo_underruns;
        //        }
    }

    current_task = NULL;
    current_processor = NULL;

#if !EXECUTE_TASKS_ON_INTERRUPT
    execute_task = false;
#endif

    if (on_event) {
        on_event(dsp_status);
    }

    dsp_status = NULL;
}

void dspSuccess() { dspStop(); }

void dspError(DSP_ERROR err) {

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
