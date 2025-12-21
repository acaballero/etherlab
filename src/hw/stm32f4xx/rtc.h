//
// Created by Angel Dust on 08/01/2022.
//

#ifndef TRX_FRONTEND_RTC_H
#define TRX_FRONTEND_RTC_H

#include <stm32f4xx.h>

#include "Signal.h"

extern RTC_HandleTypeDef hrtc;

struct st_datetime {
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
};

#ifdef __cplusplus
extern "C" {
#endif

void MX_RTC_Init(void);
void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc);
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc);
void RTC_Alarm_IRQHandler(void);
int RTC_Set(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec, uint8_t dow);
st_datetime rtc_get_date_time();
uint32_t rtc_uptime();
bool is_rtc_ok();
#ifdef __cplusplus
}
#endif

extern Signal rtc_signal;
void rtc_to_string(st_datetime dt, bool, char *);
uint32_t rtc_to_epoch(RTC_TimeTypeDef *time, RTC_DateTypeDef *date);
void rtc_from_epoch(uint32_t epoch, RTC_TimeTypeDef *time, RTC_DateTypeDef *date);

#endif // TRX_FRONTEND_RTC_H
