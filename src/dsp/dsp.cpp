//
// Created by Angel Dust on 04/04/2021.
//
#include "dsp.h"
#include "agc.h"
#include "arm_math.h"
#include "blocks/dc_block.h"
#include "common/tusb_verify.h"
#include "config.h"
#include "diskio.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_config.h"
#include "dsp/fft/fft.h"
#include "dsp/fft/fft_params.h"
#include "dsp/fft/fft_types.h"
#include "dsp/receive/receive_task.h"
#include "dsp/transmit/transmit_task.h"
#include "handlers.h"
#include "hw/board/board_v2.h"
#include "radio.h"
#include "status.h"
#include "stm32f4xx_hal.h"
#include "tinyusb/usb_audio_dsp_bridge.h"
#include "tinyusb/usb_composite_device.h"
#include "types.h"
#include "ui/lcd.h"
#include "hw/stm32f4xx/timers.h"
#include "utils.hpp"
#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>

#include "../ui/menu.h"

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
os::periodic_task task(50, dsp_loop, 0, 0);
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

std::unique_ptr<Task> dsp_task;
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

DCBlock dc_block_i{0.987};
DCBlock dc_block_q{0.987};

std::function<void(st_dsp_params *)> on_event;

/** Sets or unsets the real-time DSP mode, for which only one slice of FFT can be used **/
void dsp_set_real_time(bool real_time) {

    // LOG("Setting real time: %d\n", b);

    // When doing real-time DSP, we can only process one slice (no frequency hops allowed)
    fft::set_max_slices(real_time ? 1 : config.fft.max_slices);

    // In DIGITAL_TX mode we don't want the FFT to decimate. The stream will be sent to the FFT FIFO with the final sample rate

    fft::set_max_decimation(config.mode == DIGITAL_TX ? 1 : config.fft.max_decimation_factor);

    //  Update FFT and sample rate parameters
    dsp::set_sample_freq_limits(real_time);
}

void restart_callback(void *, const void *) {

    if (ISANALOG && current_task && dsp::dsp_params && dsp::dsp_params->status == DSP_STATUS_RUNNING) {
        LOG("restart_callback: ANALOG mode ON: issuing stop command\n");
        dsp_command({(DSP_COMMAND)DSP_COMMAND_STOP, current_task->status.id, current_task}, nullptr);
    } else if (config.mode == DIGITAL_RX && (!current_task || (current_task->status.id == dsp::DSP_PROCESSOR_TRANSMIT))) {
        LOG("restart_callback: Toggle digital TX->RX\n");
        dsp_stop();
        dsp_command({(DSP_COMMAND)DSP_COMMAND_START, dsp::DSP_PROCESSOR_RECEIVE}, nullptr);
    } else if (config.mode == DIGITAL_TX && (!current_task || (current_task->status.id == dsp::DSP_PROCESSOR_RECEIVE))) {
        LOG("restart_callback: Toggle digital RX->TX\n");
        dsp_stop();
        dsp_task = std::make_unique<TransmitTask>(dspSuccess, dspError);
        dsp_command({(DSP_COMMAND)DSP_COMMAND_START, dsp::DSP_PROCESSOR_TRANSMIT}, nullptr);
    } else {
        LOG("restart_callback:dsp_restart\n");
        dsp_restart();
    }

    if (dsp::get_agc_enabled()) {
        agc::reset();
    }
}

void dsp_init(dsp::st_dsp_config &config) {

    dsp::set_config(config);
    ADC_DMA_Start(&hadc1);
    dsp::set_sample_freq_limits(false);
    main_board::mode_signal.add(nullptr, restart_callback); // modulation or mode changed
    fft::signal.add(nullptr, restart_callback);             // fft params changed
    restart_callback(nullptr, nullptr);                     // First time, in case we start in DSP mode and miss initial signals
}

void dsp_stop_task() {

    LOG_IND(2, "dsp_stop_task\n");
    if (current_processor) {
        LOG("stopping processor\n");
        current_processor->stop();
    }

    if (current_task) {
        LOG("stopping task\n");
        current_task->stop();
        dsp_task.reset(); // Forces deallocation before new construct reclaim memory
    }

    LOG_IND_RAW(-2, "");
    current_task = NULL;
    current_processor = NULL;
}

Task *get_command_task(dsp::st_dsp_command &command) {

    Task *task = nullptr;
    if (command.task) {
        task = command.task;
    } else {
        if (command.id < dsp::DSP_TASKS_N) {
            // TODO: Elimitate pre-created tasks
            task = dsp::tasks[command.id];
        } else {

            switch (command.id) {
                case dsp::DSP_PROCESSOR_RECEIVE:
                    dsp_task = std::make_unique<ReceiveTask>(dspSuccess, dspError);
                    task = dsp_task.get();
                    break;
                case dsp::DSP_PROCESSOR_TRANSMIT:
                    dsp_task = std::make_unique<TransmitTask>(dspSuccess, dspError);
                    task = dsp_task.get();
                    break;

                default:
                    status::pop_alert(status::ERROR, "Error getting task from command ID");

                    do {
                    } while (0); // Breakpoint
            }
        }
    }

    return task;
}
uint8_t dsp_command(dsp::st_dsp_command command, std::function<void(st_dsp_params *)> cb) {

    LOG("dsp_command: Scheduling cmd: %s, processor: %s\n", dsp::commandNames[command.command], dsp::processorNames[command.id]);

    Task *task = get_command_task(command);
    if (current_task == task) {
        DSP_STATUS s = current_task->status.status;
        if ((command.command == DSP_COMMAND_START && s == DSP_STATUS_RUNNING) || (command.command == DSP_COMMAND_STOP && s == DSP_STATUS_STOPPED) ||
            s == DSP_STATUS_PENDING) {
            LOG("WARN: Skipping command %s: current task status: %d\n", dsp::commandNames[command.command], s);
            return 1;
        }
    }

    if (command.command != DSP_COMMAND_STOP) {
        dsp_stop();
    }

    on_event = cb;
    pending_command = command;
    current_task = task;
    current_task->status.status = DSP_STATUS_PENDING;
    current_task->status.id = pending_command.id;

    // FIXME: Ugly
    dsp::dsp_params = &current_task->status;

    return 0;
}

/*
 * When this callback is passed to the usb bridge, it is called from the USB task timer ISR and fills the
 * Note this writes the input_stream so it can't be running simultaneously with another task trying to write to it
 */
void usb_audio_in_callback() {

    int16_t *p;

    int32_t free = input_stream.free((char **)&p);
    int32_t bytes_to_read = DSP_BLOCK * sizeof(int16_t);

    if (free >= bytes_to_read) {
        usb_audio_receive(p, DSP_BLOCK);
        input_stream.feed(bytes_to_read);
    }
}

void dsp_start_usb_bridge() {
    LOG("dsp_start_usb_bridge: Setting USB bridge callback\n");
    set_audio_in_callback(usb_audio_in_callback);
}

void dsp_stop_usb_bridge() {
    LOG("dsp_stop_usb_bridge: Clearing USB bridge callback\n");
    set_audio_in_callback(nullptr);
}

void dsp_start_task() {
    LOG_IND(2, "dsp_start_task:");

    if (!dsp::dsp_params || dsp::dsp_params->status != DSP_STATUS_RUNNING) {

        LOG_RAW(" Starting new task. Processor %s\n", dsp::processorNames[pending_command.id]);

        input_stream.reset();
        output_stream.reset();

        // The first time the DAC DMA is started, it has to be reseted to the quadrature modulator common mode to prevent carrier transients
        // TODO: Not sure if its better to do decouple the DACs from the modulator and set the common mode in hardware. The drawback would be
        // not being able to fine-tune the offsets in software
        reset_dac_buffer(config.hw.dac_offset);

        current_buffer->sample_rate = current_task->status.sample_rate;

        current_processor = dsp::processors[pending_command.id];
        current_processor->reset(); // In case wasn't properly stopped from an earlier run (which should be avoided, by the way)

        if (current_processor->status.direction == DSP_DIRECTION_OUT) {
            // Link the start of the processor with the 1st block processed event of the task to prevent false underruns
            current_task->on_first_block = []() {
                LOG("First block ready: starting processor\n");

                current_processor->start();
            };
        }

        //  LOG("dsp_start_task: starting task\n");
        current_task->status.reset();

        // TODO: Do this elsewhere. Also, read the analog volume pot or use a rotary encoder to set the gain
        if (current_task->status.id == dsp::DSP_PROCESSOR_RECEIVE) {
            dsp::set_gain_db(0);
        } else {
            dsp::set_gain_db(dsp::dsp_config.gain);
        }

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
                LOG("Starting processor\n");
                bool ok = current_processor->start();
                if (!ok) {
                    status::pop_alert(status::ERROR, "Error starting DSP processor");
                    return;
                }
            } else {
                // current_processor->status.status = DSP_STATUS_PENDING;
            }

            // FIXME: Why this logic? Weak and smelling.
            dsp::dsp_params = current_task->status.direction != DSP_DIRECTION_IN ? &current_processor->status : &current_task->status;

            if (current_task->status.id == dsp::DSP_PROCESSOR_TRANSMIT) {
                dsp_start_usb_bridge();
            }

            if (on_event) {
                on_event(dsp::dsp_params);
            }
        } else {
            status::pop_alert(status::ERROR, "Error starting DSP task");
        }
    } else {
        LOG(" Already running task, skipping\n");
    }

    LOG_IND_RAW(-2, "");
}

bool dsp_restart() {
    // LOG("dsp_restart");
    if (!ISANALOG && current_task && dsp::dsp_params && dsp::dsp_params->status == DSP_STATUS_RUNNING) {

        //   LOG(": will restart\n");
        current_task->status.status = DSP_STATUS_PENDING;
        current_processor->status.status = DSP_STATUS_PENDING;
        DAC_DMA_Stop(&hdac1);
        ADC_DMA_Stop(&hadc1);
        dsp_start_task();
        return true;
    } else if (!current_task) {
        //   LOG(": wont restart: no task\n");
    } else if (current_task && dsp::dsp_params) {
        int b = dsp::dsp_params->status == DSP_STATUS_RUNNING ? 0 : 1;
        (void)b;
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
        LOG("Processing pending DSP command %s\n", dsp::commandNames[command.command]);
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

        if (pending_command == command) { // Another command may have been queued so only if it's the same
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
    //    GPIOD->BSRR |= GPIO_PIN_9;

    if (current_processor && (current_processor->status.direction == DSP_DIRECTION_OUT || current_processor->status.direction == DSP_DIRECTION_INOUT)) {
        current_processor->work(current_buffer);
    } else {
        memset((char *)current_buffer->p, 0, current_buffer->count << 1); // Empty output
    }

    // Apply gain
    adc_type *p = (adc_type *)current_buffer->p;
    for (size_t i = 0; i < current_buffer->count; i += 2) {
        p[i] = (adc_type)(p[i] * dsp::dsp_params->gain_factor);
        p[i + 1] = (adc_type)(p[i + 1] * dsp::dsp_params->gain_factor);
    }

    if ((dsp::dsp_params && dsp::dsp_params->direction == DSP_DIRECTION_OUT) ||
        (current_processor && current_processor->status.direction == DSP_DIRECTION_INOUT && current_task &&
         current_task->status.direction == DSP_DIRECTION_OUT)) {

        FIFO_ERROR err = fft_fifo.write_block((char *)current_buffer->p, current_buffer->size_bytes);
        UNUSED(err);
    }

    // Apply DAC pre-distortion
    auto offset_balance = config.hw.dac_offset + config.hw.dac_off_balance;

    for (size_t i = 0; i < current_buffer->count; i += 2) {
        p[i] = p[i] + config.hw.dac_offset;
        p[i + 1] = (adc_type)(p[i + 1] * config.hw.dac_amp_balance) + offset_balance;
    }

    //  GPIOD->BSRR |= GPIO_PIN_9 << 16;
}

inline void adc_work() {
    // GPIOD->BSRR |= GPIO_PIN_9;

    if (check_overload_pending) {
        check_overload(); // This has to be done here since this buffer can be quickly dc-blocked in-place and the overload reference range is only positive
        check_overload_pending = false;
    }

#if DSP_FS4_SHIFT

    if (dsp::get_freq_shift_allowed()) {

        // Removing DC here and thus not having to do it in subsequent stages (FFT, DSP processing) is not as efficient as it seems at first glance since:
        // - FFT processing is not done in real-time so no need to do the work for it
        // - Receivers, for example, could remove DC just before demodulation, at a much lower sample rate but, in fact, after shifting, the DC spike is
        //   removed by the low pass filters
        // The drawback is we have to rotate it there (fft module)too after DC removal

        if (config.fft.removeDC) {
            buffer_t<adc_type> bb = {(adc_type *)current_buffer->p, DSP_BLOCK * 2};
            dc_block_i.filter(bb, 2, 0);
            dc_block_q.filter(bb, 2, 1);
        }

        if (fft::fft_params.decimation_factor > 1) {
            // TODO: Decimate here vs in both FFT and current DSP task?
        }

        dsp::rotate_fs4_q15((const q15_t *)current_buffer->p, (q15_t *)current_buffer->p, DSP_BLOCK);
    }

    if (!dsp::dsp_params || dsp::dsp_params->direction == DSP_DIRECTION_IN || dsp::dsp_params->direction == DSP_DIRECTION_INOUT) {

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

void dsp_test_cb(st_dsp_params *) {

    if (dsp::dsp_params->error == DSP_ERR_NONE && dsp::dsp_params->id != dsp::DSP_TASK_REPLAY) {
        dsp_command({DSP_COMMAND_START, dsp::DSP_TASK_REPLAY}, dsp_test_cb);
    }
}

void dsp_stop() {

    if (dspstatus != DSP_STATUS_STOPPING) {

        dspstatus = DSP_STATUS_STOPPING;

        auto current_task_id = current_task ? current_task->status.id : -1;

        dsp_stop_task();

#if !EXECUTE_TASKS_ON_INTERRUPT
        execute_task = false;
#endif

        if (on_event) {
            //  LOG("dspStop: onEvent\n");
            on_event(dsp::dsp_params);
        }

        dsp::dsp_params = NULL;

        dspstatus = DSP_STATUS_STOPPED;

        if (!ISANALOG && current_task_id >= 0 && current_task_id != dsp::DSP_PROCESSOR_RECEIVE) {
            // TODO: This prevents stopping all tasks in digital mode by  causing the receive task to be restarted. But its ugly
            dsp_command({DSP_COMMAND_START, dsp::DSP_PROCESSOR_RECEIVE}, nullptr);
        }

        dsp_stop_usb_bridge();
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
            pop_alert(status::ERROR, "Wave file open error");
            break;
        case DSP_ERR_FILECLOSE:
            pop_alert(status::ERROR, "Wave file close error");
            break;
        case DSP_ERR_FILEWRITE:
            pop_alert(status::ERROR, "Wave write error");
            break;
        case DSP_ERR_FILEREAD:
            pop_alert(status::ERROR, "Wave read error");
            break;
        case DSP_ERR:
            pop_alert(status::ERROR, "DSP error");
            break;
        case DSP_ERR_DMAOVERRUN:
            pop_alert(status::ERROR, "DMA overrun");
            break;
        case DSP_ERR_FIFO_OVERRUN:
            pop_alert(status::ERROR, "FIFO overrun");
            break;
        case DSP_ERR_FIFO_UNDERRUN:
            pop_alert(status::ERROR, "FIFO underrun");
            break;
        default:
            break;
    }

    dsp_stop();
}
