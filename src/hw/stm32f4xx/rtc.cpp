//
// Created by Angel Dust on 08/01/2022.
//

#include "rtc.h"
#include "string.h"
#include "Signal.h"
#include <sys/_stdint.h>

#define JULIAN_DATE_BASE 2440588 // Unix epoch time in Julian calendar (UnixTime = 00:00:00 01.01.1970 => JDN = 2440588)

void rtc_update();

RTC_HandleTypeDef hrtc;
st_datetime date_time;
st_datetime last_boot;
Signal rtc_signal;
uint32_t last_epoch;
bool rtc_ok = false;

/**
 * @brief RTC MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hrtc: RTC handle pointer
 * @retval None
 */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc) {
    if (hrtc->Instance == RTC) {
        /* USER CODE BEGIN RTC_MspInit 0 */

        /* USER CODE END RTC_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_RTC_ENABLE();
        /* USER CODE BEGIN RTC_MspInit 1 */
        /* RTC interrupt Init */
        HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
        /* USER CODE END RTC_MspInit 1 */
    }
}

/**
 * @brief RTC MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hrtc: RTC handle pointer
 * @retval None
 */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc) {
    if (hrtc->Instance == RTC) {
        /* USER CODE BEGIN RTC_MspDeInit 0 */

        /* USER CODE END RTC_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_RTC_DISABLE();
        /* USER CODE BEGIN RTC_MspDeInit 1 */
        /* RTC interrupt DeInit */
        HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
        /* USER CODE END RTC_MspDeInit 1 */
    }
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void RTC_Error_Handler(void) {
    /* USER CODE BEGIN ADC_Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */

    /* USER CODE END ADC_Error_Handler_Debug */
}

/**
 * @brief RTC Initialization Function
 * @param None
 * @retval None
 */
void MX_RTC_Init(void) {
    /* USER CODE BEGIN RTC_Init 0 */

    /* USER CODE END RTC_Init 0 */

    /* USER CODE BEGIN RTC_Init 1 */

    /* USER CODE END RTC_Init 1 */
    /** Initialize RTC Only
     */
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 116;
    hrtc.Init.SynchPrediv = 341; // Division ratio = (AsyncPred+1)*(SynchPred+1)
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        RTC_Error_Handler();
    }
    /* USER CODE BEGIN RTC_Init 2 */

    /* USER CODE END RTC_Init 2 */

    /* SET alarm every 60 seconds */

    /**Enable the Alarm A    */
    RTC_AlarmTypeDef sAlarm;
    sAlarm.AlarmTime.Hours = 0;
    sAlarm.AlarmTime.Minutes = 0;
    sAlarm.AlarmTime.Seconds = 0;
    sAlarm.AlarmTime.TimeFormat = RTC_HOURFORMAT_24;
    sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_ADD1H;
    sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
    sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY | RTC_ALARMMASK_HOURS | RTC_ALARMMASK_MINUTES;
    sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    sAlarm.AlarmDateWeekDay = 1;
    sAlarm.Alarm = RTC_ALARM_A;
    if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK) {
        RTC_Error_Handler();
    }

    rtc_update();
    last_boot = date_time;
}

int RTC_Set(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec, uint8_t dow) {

    HAL_StatusTypeDef res;
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    memset(&time, 0, sizeof(time));
    memset(&date, 0, sizeof(date));

    date.WeekDay = dow;
    date.Year = year;
    date.Month = month;
    date.Date = day;

    res = HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
    if (res != HAL_OK) {
        return -1;
    }

    time.Hours = hour;
    time.Minutes = min;
    time.Seconds = sec;

    res = HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
    if (res != HAL_OK) {
        return -2;
    }

    rtc_signal.emit((void *)&date_time);

    return 0;
}

/**
 * @brief This function handles RTC alarms A and B interrupt through EXTI line 17.
 */
void RTC_Alarm_IRQHandler(void) {
    /* USER CODE BEGIN RTC_Alarm_IRQn 0 */
    rtc_signal.emit((void *)&date_time);

    /* USER CODE END RTC_Alarm_IRQn 0 */
    HAL_RTC_AlarmIRQHandler(&hrtc);
    /* USER CODE BEGIN RTC_Alarm_IRQn 1 */

    /* USER CODE END RTC_Alarm_IRQn 1 */
}

bool is_rtc_ok() {
    return rtc_ok;
}

// Convert Date/Time structures to epoch time
uint32_t rtc_to_epoch(RTC_TimeTypeDef *time, RTC_DateTypeDef *date) {
    uint8_t a;
    uint16_t y;
    uint8_t m;
    uint32_t JDN;

    // These hardcore maths are taken from http://en.wikipedia.org/wiki/Julian_day

    // Calculate some coefficients
    a = (14 - date->Date) / 12;
    y = (date->Year + 2000) + 4800 - a; // years since 1 March 4801 BC
    m = date->Month + (12 * a) - 3;     // since 1 March 4801 BC

    // Gregorian calendar date compute
    JDN = date->Date;
    JDN += (153 * m + 2) / 5;
    JDN += 365 * y;
    JDN += y / 4;
    JDN += -y / 100;
    JDN += y / 400;
    JDN = JDN - 32045;
    JDN = JDN - JULIAN_DATE_BASE; // Calculate from base date
    JDN *= 86400;                 // Days to seconds
    JDN += time->Hours * 3600;    // ... and today seconds
    JDN += time->Minutes * 60;
    JDN += time->Seconds;

    return JDN;
}

// Convert epoch time to Date/Time structures
void rtc_from_epoch(uint32_t epoch, RTC_TimeTypeDef *time, RTC_DateTypeDef *date) {
    uint32_t tm;
    uint32_t t1;
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t m;
    int16_t year = 0;
    int16_t month = 0;
    int16_t dow = 0;
    int16_t mday = 0;
    int16_t hour = 0;
    int16_t min = 0;
    int16_t sec = 0;
    uint64_t JD = 0;
    uint64_t JDN = 0;

    // These hardcore math's are taken from http://en.wikipedia.org/wiki/Julian_day

    JD = ((epoch + 43200) / (86400 >> 1)) + (2440587 << 1) + 1;
    JDN = JD >> 1;

    tm = epoch;
    t1 = tm / 60;
    sec = tm - (t1 * 60);
    tm = t1;
    t1 = tm / 60;
    min = tm - (t1 * 60);
    tm = t1;
    t1 = tm / 24;
    hour = tm - (t1 * 24);

    dow = JDN % 7;
    a = JDN + 32044;
    b = ((4 * a) + 3) / 146097;
    c = a - ((146097 * b) / 4);
    d = ((4 * c) + 3) / 1461;
    e = c - ((1461 * d) / 4);
    m = ((5 * e) + 2) / 153;
    mday = e - (((153 * m) + 2) / 5) + 1;
    month = m + 3 - (12 * (m / 10));
    year = (100 * b) + d - 4800 + (m / 10);

    date->Year = year - 2000;
    date->Month = month;
    date->Date = mday;
    date->WeekDay = dow;
    time->Hours = hour;
    time->Minutes = min;
    time->Seconds = sec;
}

void rtc_update() {
    // The order in which date and time registers are read must be time, then date.
    HAL_RTC_GetTime(&hrtc, &date_time.time, FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date_time.date, FORMAT_BIN);
}

st_datetime rtc_get_date_time() {
    rtc_update();

    uint32_t current_epoch = rtc_to_epoch(&date_time.time, &date_time.date);

    if (current_epoch != last_epoch) {
        last_epoch = current_epoch;
        rtc_ok = true;
    } else {
        // The clock is not ticking. Return elapsed time from boot
        rtc_ok = false;
        rtc_from_epoch(HAL_GetTick() / 1000, &date_time.time, &date_time.date);
    }

    return date_time;
}

void rtc_to_string(st_datetime dt, bool only_date, char *buffer) {

    if (only_date) {
        snprintf(buffer, 9, "%02d:%02d:%02d", dt.time.Hours, dt.time.Minutes, dt.time.Seconds);
    } else {
        snprintf(buffer, 17, "%02d-%02d-%02d %02d:%02d:%02d", dt.date.Year, dt.date.Month, dt.date.Date, dt.time.Hours, dt.time.Minutes, dt.time.Seconds);
    }
}

uint32_t rtc_uptime() {
    auto dt = rtc_get_date_time();
    uint32_t epoch_now = rtc_to_epoch(&dt.time, &dt.date);
    uint32_t epoch_boot = rtc_to_epoch(&dt.time, &dt.date);
    uint32_t elapsed = epoch_now - epoch_boot;
    return elapsed;
}
