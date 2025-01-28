#include "main.h"
#include "agc.h"
#include "battery.h"
#include "cat_protocol.h"
#include "input/input_controller.h"
#include "main_board.h"
#include "menuBase.h"
#include "periodic_task.h"
#include "power_amp.h"
#include "radio.h"
#include "rf_coupler.h"
#include "s_strength.h"
#include "scanner.h"
#include "setup.h"
#include "standby.h"
#include "types.h"
#include "ui/menu.h"
#include "ui/view_manager.h"

#if ENABLE_FFT

#include "dsp/dsp.h" // adc.h requires FFT_TYPE to be declared first

#endif

#if ENABLE_SD_CARD

#include "fatfs/fatfs.h"

#endif

#if USB_PRINT_ENABLED

#include "../lib/HAL_USB_Stack/USBPrint.h"
#include "usb/usb_device.h"

//#include "../lib/HAL_USB_Stack/usbd_cdc_if.h" // Where CDC_Transmit_FS lies
extern PCD_HandleTypeDef hpcd_USB_FS;
USBPrint usb;

#endif

#define HAL_EXTI_MODULE_ENABLED

#include "GPIOPin.h"
#include "MCP23017Pin.h"
#include "hw/stm32f4xx/adc.h"
#include "input/touch.h"

#define ENABLE_GPIO_CLOCK (RCC->AHBENR |= RCC_AHBENR_GPIOEEN | RCC_AHBENR_GPIOBEN)
#define HIGH GPIO_PIN_SET
#define LOW GPIO_PIN_RESET

unsigned long t1, t2;

void view_loop();

periodic_task view_task(250, view_loop);
bool printADC = false;

#define pinIndex(P) ((uint8_t)(P > 13 ? P - 14 : P & 7))
#define pinmask(P) ((uint8_t)(1 << pinIndex(P)))

GPIOPin ledPin(LED_0_PIN, LED_0_GPIO_PORT, GPIO_MODE_INPUT);
MCP23017Pin powPin(GPIOEXP_FPANEL_STBY_LED, MCP23017_PORTB, &hmcp03, GPIO_MODE_OUTPUT_PP);

unsigned long last_autosave_ms = 0;

/***
 *
 * Signal strength and RSSI (Receiver signal strength indicator)
 *
 * We have an audio frequency signal strength meter and (at this moment)
 * a logarithmc amplifier and limiter for the FM/AM detector, which has an RSSI
 * output. The AF signal strenght cannot be used for FM, as the detector
 * generates high noise power when the RF signal is below the detection
 * threshold. Also, in SSB mode, the log amp of the AM/FM detector board is
 * disabled. Therefore, until I develop an RSSI level measurement circuit (using
 * the IF signal) which is available for all modulations, the signal strenght
 * must be calculated using either the AF signal strenght board, for SSB, or the
 * RSSI level of the log amp, for AM/FM
 */

#define RX_TX_PIN 4

bool change_drive_strength = false;
bool change_calibration = false;

void checkAutoSaveConfig() {

    unsigned long m = 0; // HAL_GetTick();

    if ((m - last_autosave_ms) > (CONFIG_AUTOSAVE_SECS * 1000)) {
        last_autosave_ms = m;
        //  saveConfig();
    }
}

/*
 * Bliks the led with a period of period_ms microseconds
 */
void blink(uint32_t period_ms) {
    HAL_TIM_Base_Start_IT(&LED_TIMER_HANDLE);
    update_timer(LED_TIMER_TYPEDEF, period_ms, LED_TIMER_TYPEDEF_CLOCK_HZ);
}

void stop_blink() {
    ledPin.set(GPIO_PIN_RESET);
    powPin.set(GPIO_PIN_SET);
    HAL_TIM_Base_Stop_IT(&LED_TIMER_HANDLE);
}

void standby_signal_callback(void *thisptr, void *args) {
    if (standby::power_mode == standby::POWER_MODE_SLEEP) {
        blink(100000000);
    } else {
        stop_blink();
    }
}

void frequency_signal_callback(void *thisptr, void *args) {

    radio::st_freq_event event = *((radio::st_freq_event *)args);

    switch (event.event) {

        case radio::BEFORE_UPDATE:
            // Prevent audio transients
            main_board::setMute(GPIO_PIN_SET);
            break;
        case radio::AFTER_UPDATE:
            main_board::setMute(GPIO_PIN_RESET);

            if (config.filter == radio::BAND_AUTO) {
                main_board::set_filter();
                main_board::set_if_filter(config.if_filter);
            }
            view_manager::mainView.Waterfall()->centerSpectrum();
            break;
    }
}

void test() {
    // Go to a  function to avoid having to use the menu again and again
    nav.doNav(Menu::navCmd(Menu::enterCmd));
    nav.doNav(Menu::navCmd(Menu::idxCmd, 3));
    nav.doNav(Menu::navCmd(Menu::idxCmd, 6));
    //  nav.doNav(Menu::navCmd(Menu::enterCmd));
    // nav.doNav(Menu::navCmd(Menu::idxCmd, 2));
    //  nav.doNav(Menu::navCmd(Menu::enterCmd));
}

bool dsptested = false;

void view_loop() {
    // TODO: Delegate dirty state management to the widget itself based on
    // information change messages and refresh rate

    view_manager::mainView.TuneInfo()->set_dirty();
    view_manager::mainView.FFTInfo()->set_dirty();
    view_manager::mainView.Menu()->set_dirty();

    view_manager::currentView->paint();
}

//
// void test_dac() {
//    uint32_t DAC_OUT[4] = {0, 1241, 2482, 3723};
//    uint8_t i = 0;
//    HAL_StatusTypeDef ret=HAL_OK;
//    ret = HAL_DAC_Start(&hdac1, 0x00000000U); // channel1
//    ret = HAL_OPAMP_Start(&hopamp4);
//    while (ret!=HAL_ERROR)
//    {
//        //DAC1->DHR12R1 = DAC_OUT[i++];
//        ret = HAL_DAC_SetValue(&hdac1, 0x00000000U,0,DAC_OUT[i++]);
//        if(i == 4)
//        {
//            i = 0;
//        }
//        HAL_Delay(50);
//    }
//}

int main() {

    setup();
    radio::freq_signal.add(NULL, frequency_signal_callback);
    standby::signal.add(NULL, standby_signal_callback);

    view_manager::init();

    while (1) {

        if (standby::power_mode == standby::POWER_MODE_ON) {

            if (change_drive_strength) {
                lo_strength(0, config.lo_drive_strength_0);
                lo_strength(1, config.lo_drive_strength_1);
                lo_strength(2, config.lo_drive_strength_1);
                change_drive_strength = false;
            }

            if (change_calibration) {
                calibrate_freq();
                radio::update_freq();
                change_calibration = false;
            }

            checkAutoSaveConfig();
            radio::loop();
            agc::loop();

#if LCD_ENABLED

#if ENABLE_FFT && DSP_ENABLED
            fft_task.loop();
#endif

#if ENABLE_SD_CARD
            sdcard_loop();
#endif
            view_task.loop();
#endif

            scanner::loop();
            sstrength::loop();
            battery::loop();
            power_amp::loop();
            dsp_loop();
            rf_coupler::loop();
            // touch::loop(); // Not required. Dome by interrupts
        }

#if USB_ENABLED
        cat_protocol::loop(); // CAT protocol
#endif
        dispatchEvents();

        if (!dsptested) {
#if DEBUG_SD_CARD
            test_sd_card();
#endif
            test();

            dsptested = true;
        }
    }
}

void TIM3_IRQHandler(void) {
    ledPin.toggle();
    powPin.toggle();

    /* USER CODE END TIM1_UP_TIM10_IRQHandler 0 */
    HAL_TIM_IRQHandler(&LED_TIMER_HANDLE);
    /* USER CODE BEGIN TIM1_UP_TIM10_IRQHandler 1 */

    /* USER CODE END TIM1_UP_TIM10_IRQHandler 1 */
}

#if SWO_ENABLED

/*
 * Enables sending printf / puts strings through the SWO pin when a debugger is
 * attached If a debugger is present, it should enable ITM on the target,
 * otherwise ITM_Sendchar will return without doing anything
 */
int _write(int file, char *ptr, int len) {

    int DataIdx;

    for (DataIdx = 0; DataIdx < len; DataIdx++) {
        ITM_SendChar(*ptr++);
    }

    return len;
}

void _putchar(char c) { ITM_SendChar(c); }

#endif
