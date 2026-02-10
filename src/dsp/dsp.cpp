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
#include "dsp/capture/capture_task.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_config.h"
#include "dsp/fft/fft.h"
#include "dsp/fft/fft_params.h"
#include "dsp/fft/fft_types.h"
#include "dsp/receive/receive_task.h"
#include "dsp/replay/replay_task.h"
#include "dsp/signal_generator/signal_generator_task.h"
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
#include <sys/_stdint.h>

#include "../ui/menu.h"

#if ENABLE_SD_CARD

#include "../fatfs/fatfs.h"
#include "../../lib/FatFs/ff.h"

#endif

#include "hw/stm32f4xx/adc.h"
#include "dsp_tasks.h"
#include "dsp_tasks.h"
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
buffer_t<adc_type> *current_buffer;
DSP_STATUS dspstatus;
uint64_t overload_history{0};
bool check_overload_pending{false};
#if !EXECUTE_TASKS_ON_INTERRUPT
volatile bool execute_task = false;
#endif

DCBlock dc_block_i{0.987};
DCBlock dc_block_q{0.987};

std::function<void(st_dsp_params *)> on_event;

/*
 * Task factory
 */
static std::unique_ptr<Task> create_task(dsp::DSP_TASK_ID id) {

    std::unique_ptr<Task> task;

    switch (id) {
        case dsp::DSP_TASK_CAPTURE:
            task = std::make_unique<CaptureTask>(dsp_success, dsp_error);
            break;
        case dsp::DSP_TASK_REPLAY:
            task = std::make_unique<ReplayTask>(dsp_success, dsp_error);
            break;
        case dsp::DSP_TASK_SIGNAL_GENERATOR:
            task = std::make_unique<SignalGeneratorTask>(dsp_success, dsp_error);
            break;
        case dsp::DSP_TASK_RECEIVE:
            task = std::make_unique<ReceiveTask>(dsp_success, dsp_error);
            break;
        case dsp::DSP_TASK_TRANSMIT:
            task = std::make_unique<TransmitTask>(dsp_success, dsp_error);
            break;
        default:
            status::pop_alert(status::ERROR, "Unknown DSP task ID");
            return nullptr;
    }

    task->info.id = id;
    return task;
}

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

    if (ISANALOG && dsp_task && dsp::dsp_params && dsp::dsp_params->status == DSP_STATUS_RUNNING) {

        LOG("restart_callback: ANALOG mode ON: issuing stop command\n");
        dsp_stop();

    } else if (config.mode == DIGITAL_RX && (!dsp_task || (dsp_task->info.id == dsp::DSP_TASK_TRANSMIT))) {

        LOG("restart_callback: Start DSP task RX\n");
        dsp_start(dsp::DSP_TASK_RECEIVE, nullptr);

    } else if (config.mode == DIGITAL_TX && (!dsp_task || (dsp_task->info.id == dsp::DSP_TASK_RECEIVE))) {

        LOG("restart_callback: Start DSP task TX\n");
        dsp_start(dsp::DSP_TASK_TRANSMIT, nullptr);

    } else {

        dsp_restart();
    }

    if (dsp::get_agc_enabled()) {
        LOG("restart_callback: Resetting AGC\n");
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

Task *dsp_start(std::unique_ptr<Task> task, std::function<void(st_dsp_params *)> cb) {

    uint8_t id = task->info.id;

    LOG_IND(2, "dsp_start: Enqueuing next task start : %s\n", dsp::get_task_name(id));

    if (dsp_task && dsp_task->info.id == id) {
        DSP_STATUS s = dsp_task->info.status;
        if (s == DSP_STATUS_RUNNING || s == DSP_STATUS_PENDING) {
            LOG("WARN: Skipping start: Already RUNNING or enqueued\n");
            LOG_IND_RAW(-2, "");
            return dsp_task.get();
        }
    }

    dsp_stop();

    dsp_task.reset();
    dsp_task = move(task);

    on_event = cb;

    dsp_task->info.status = DSP_STATUS_PENDING;
    dsp_task->info.id = id;

    dsp::dsp_params = dsp_task->get_info();

    LOG_IND_RAW(-2, "");

    return dsp_task.get();
}

Task *dsp_start(dsp::DSP_TASK_ID id, std::function<void(st_dsp_params *)> cb) {
    return dsp_start(create_task(id), cb);
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

    LOG_IND(2, "dsp_start_task: ");
    if (!dsp::dsp_params || dsp::dsp_params->status != DSP_STATUS_RUNNING) {

        LOG_RAW("starting new task %s\n", dsp::get_task_name(dsp_task->info.id));

        input_stream.reset();
        output_stream.reset();

        // The first time the DAC DMA is started, it has to be reseted to the quadrature modulator common mode to prevent carrier transients
        // TODO: Not sure if its better to do decouple the DACs from the modulator and set the common mode in hardware. The drawback would be
        // not being able to fine-tune the offsets in software
        reset_dac_buffer(config.hw.dac_offset);

        //  LOG("dsp_start_task: starting task\n");
        dsp_task->info.reset();

        if (dsp_task->start()) {

            current_buffer->sample_rate = dsp_task->info.sample_rate;

            if (dsp_task->info.id == dsp::DSP_TASK_TRANSMIT) {
                dsp_start_usb_bridge();
            }

            // Assign this again so the returned info depends on the already initialized task (FIXME)
            dsp::dsp_params = dsp_task->get_info();

            // TODO: Do this elsewhere. Also, read the analog volume pot or use a rotary encoder to set the gain
            if (dsp_task->info.id == dsp::DSP_TASK_RECEIVE) {
                dsp::set_gain_db(0);
            } else {
                dsp::set_gain_db(dsp::dsp_config.gain);
            }

            if (on_event) {
                on_event(&dsp_task->info);
            }

            HAL_TIM_Base_Start_IT(&TASKS_TIMER_HANDLE);

        } else {
            status::pop_alert(status::ERROR, "Error starting DSP task");
        }

    } else {
        LOG_RAW("Already running task, skipping\n");
    }

    LOG_IND_RAW(-2, "");
}

bool dsp_restart() {
    // LOG("dsp_restart");
    if (!ISANALOG && dsp_task && dsp::dsp_params && dsp::dsp_params->status == DSP_STATUS_RUNNING) {

        LOG("dsp_restart: Restarting running task\n");

        dsp_task->info.status = DSP_STATUS_PENDING;
        auto proc = dsp_task->get_processor();
        if (proc) {
            proc->info.status = DSP_STATUS_PENDING;
        }

        DAC_DMA_Stop(&hdac1);
        ADC_DMA_Stop(&hadc1);

        dsp_start_task();
        return true;
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

    // Process pending start command (queued by dsp_start)
    if (dsp_task && dsp_task->info.status == DSP_STATUS_PENDING) {
        LOG("dsp_loop: processing pending start\n");

        dsp_start_task();
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

    auto proc = dsp_task ? dsp_task->get_processor() : nullptr;

    if (proc && (proc->info.direction == DSP_DIRECTION_OUT || proc->info.direction == DSP_DIRECTION_INOUT)) {
        proc->work(current_buffer);
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
        (proc && proc->info.direction == DSP_DIRECTION_INOUT && dsp_task && dsp_task->info.direction == DSP_DIRECTION_OUT)) {

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

    auto proc = dsp_task ? dsp_task->get_processor() : nullptr;
    if (proc && (proc->info.direction == DSP_DIRECTION_IN || proc->info.direction == DSP_DIRECTION_INOUT)) {
        proc->work(current_buffer);
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
        if (dsp_task) {
#if EXECUTE_TASKS_ON_INTERRUPT
            dsp_task->work();
#else
            execute_task = true;
#endif
        }
    } else {
        // LOG("Task interrupt skipped\n");
    }

    HAL_TIM_IRQHandler(&TASKS_TIMER_HANDLE);
}

void dsp_stop() {

    auto current_task_id = dsp_task ? dsp_task->info.id : -1;
    if (dspstatus != DSP_STATUS_STOPPING && current_task_id >= 0) {

        dspstatus = DSP_STATUS_STOPPING;

        LOG_IND(2, "dsp_stop: Stopping task %s\n", dsp::get_task_name(current_task_id));

        if (dsp_task->info.status != DSP_STATUS_STOPPED) {
            dsp_task->stop();
        }
        st_dsp_params params = *dsp::dsp_params; // Copy final task status
        dsp_task.reset();                        // Forces deallocation before new construct reclaim memory
        dsp::dsp_params = nullptr;
        LOG_IND_RAW(-2, "dsp_stop: Task stopped and deleted\n");

#if !EXECUTE_TASKS_ON_INTERRUPT
        execute_task = false;
#endif

        if (on_event) {
            LOG("dsp_stop: on_event\n");
            on_event(&params);
        }

        // Some tasks, when stopped, do not cause the start of the previous running task.
        // In some cases, the on_event callback (set by whoever issued the start command) takes care of that.
        // If this is the case dsp_task may have be assigned within the on_event callback.
        // In DSP mode, there should always one running task, so a restart is made if no task is enqueued at this point

        restart_callback(nullptr, nullptr);

        dspstatus = DSP_STATUS_STOPPED;

        dsp_stop_usb_bridge();
    }
}

void dsp_success() {
    // LOG("dsp_success\n");
    dsp_stop();
}

void dsp_error(DSP_ERROR err) {

    // LOG("dsp_error\n");
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
