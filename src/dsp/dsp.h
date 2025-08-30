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
#include "types.h"

namespace dsp {

struct st_dsp_command {
    DSP_COMMAND command;
    uint8_t id = 0;
    Task *task = nullptr;
    bool operator==(const st_dsp_command &st) const {
        return command == st.command && id == st.id;
    }
};

extern os::periodic_task task;
extern bool adc_overload;
bool apply_audio_bpf();
bool apply_deemph(MODULATION_MODE mod);
bool apply_compression(MODULATION_MODE mod);
} // namespace dsp

#ifdef __cplusplus
extern "C" {
#endif

void dsp_init(dsp::st_dsp_config &);
void dsp_set_real_time(bool);
uint8_t dsp_command(dsp::st_dsp_command command, std::function<void(st_dsp_status *)> cb);
bool dsp_restart();
inline void dsp_work();

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc);

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac);
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac);

// SD CARD FIFO processing handler
// void TIM1_BRK_TIM15_IRQHandler(void);
void TIM8_TRG_COM_TIM14_IRQHandler(void);

void dspSuccess();
void dspError(DSP_ERROR);

#ifdef __cplusplus
}
#endif

#endif // TRX_FRONTEND_DSP_H
