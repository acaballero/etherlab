//
// Created by Angel Dust on 04/04/2021.
//

#ifndef TRX_FRONTEND_DSP_H
#define TRX_FRONTEND_DSP_H

#include "dsp/dsp_config.h"
#include "hw/stm32.h"
#include "hw/hw_config.h"
#include "dsp_common.h"
#include "os/periodic_task.h"
#include "task.h"
#include "dsp_tasks.h"
#include "types.h"
#include <memory>

namespace dsp {

extern os::periodic_task task;
extern bool adc_overload;
bool apply_audio_bpf();
bool apply_deemph(MODULATION_MODE mod);
bool apply_compression(MODULATION_MODE mod);

} // namespace dsp

extern std::unique_ptr<Task> dsp_task;

Task *dsp_start(std::function<std::unique_ptr<Task>()> factory, std::function<void(st_dsp_params *, st_dsp_params *)> cb);
Task *dsp_start(dsp::DSP_TASK_ID id, std::function<void(st_dsp_params *, st_dsp_params *)> cb);

#ifdef __cplusplus
extern "C" {
#endif

void dsp_init(dsp::st_dsp_config &);
void dsp_set_real_time(bool);

void dsp_stop();
bool dsp_restart();
inline void dsp_work();

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc);

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac);
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac);

// Tals processing handler

void TIM8_TRG_COM_TIM14_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif // TRX_FRONTEND_DSP_H
