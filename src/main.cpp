#include "main.h"
#include "agc.h"
#include "battery.h"
#include "dsp/fft/fft.h"
#include "dsp/fft/fft_ui.h"
#include "io/cat_protocol.h"
#include "input/input_controller.h"
#include "main_board.h"
#include "menuBase.h"
#include "os/periodic_task.h"
#include "power_amp.h"
#include "radio.h"
#include "rf_coupler.h"
#include "s_strength.h"
#include "scanner.h"
#include "setup.h"
#include "standby.h"
#include "status.h"
#include "stm32f4xx_hal.h"
#include "os/task_manager.h"
#include "stm32f4xx_hal_gpio.h"
#include "types.h"
#include "ui/menu.h"
#include "ui/view_manager.h"
#include <sys/_stdint.h>
#include <sys/unistd.h>

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
GPIOPin ledPin(LED_0_PIN, LED_0_GPIO_PORT, GPIO_MODE_INPUT);
MCP23017Pin powPin(GPIOEXP_FPANEL_STBY_LED, MCP23017_PORTB, &hmcp03, GPIO_MODE_OUTPUT_PP);

os::periodic_task blink_task(1000, []() {
    ledPin.toggle();
    powPin.toggle();
});

os::periodic_task *tasks[] = {
    &board::task,           &radio::task,     &agc::task,
#if LCD_ENABLED
#if ENABLE_FFT && DSP_ENABLED
    &fft::fft_task,
#endif
#if ENABLE_SD_CARD
    &sdcard::task,
#endif
    &view_manager::task,
#endif
    &scanner::task,         &sstrength::task, &battery::task, &power_amp::task, &dsp::task, &rf_coupler::task,
//  &touch::task) // Not required. Done by interrupts
#if USB_ENABLED
    &cat_protocol::task,
#endif
    &input_controller::task
    //  &configuration::task
};

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

/*
 * Bliks the led with a period of period_ms microseconds
 */
void blink(uint32_t period_ms) {
    blink_task.set_period(period_ms);
    blink_task.set_enabled(true);
}

void stop_blink() {
    ledPin.set(GPIO_PIN_RESET);
    powPin.set(GPIO_PIN_SET);
    blink_task.set_enabled(false);
}

void standby_signal_callback(void *, void *) {

    bool sleep = standby::power_mode == standby::POWER_MODE_SLEEP;
    bool power_save = standby::power_mode == standby::POWER_MODE_SAVE;

    if (power_save || sleep) {
        blink(2000);
    } else {
        stop_blink();
    }

    if (!power_save) {
        for (auto task : tasks) {
            task->set_enabled(!sleep);
        }
    }
}

GPIO_PinState mute_state;

void frequency_signal_callback(void *, void *args) {

    radio::st_freq_event event = *((radio::st_freq_event *)args);

    switch (event.event) {

        case radio::BEFORE_UPDATE:
            mute_state = main_board::getMute();
            // Prevent audio transients
            main_board::setMute(GPIO_PIN_SET);
            break;
        case radio::AFTER_UPDATE:
            main_board::setMute(mute_state);

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
    // nav.doNav(Menu::navCmd(Menu::enterCmd));
    // nav.doNav(Menu::navCmd(Menu::idxCmd, 1));
    // nav.doNav(Menu::navCmd(Menu::idxCmd, 3));
    //  nav.doNav(Menu::navCmd(Menu::enterCmd));
    //  nav.doNav(Menu::navCmd(Menu::idxCmd, 2));
    //  nav.doNav(Menu::navCmd(Menu::enterCmd));

    // status::handleError(status::ST_ERROR, "test error");
    //   Put focus over number editor
    //   view_manager::mainView.NumberEdit()->set_focus(true);
}

bool dsptested = false;

int main() {

    setup();

    radio::freq_signal.add(NULL, frequency_signal_callback);
    standby::signal.add(NULL, standby_signal_callback);

    view_manager::init();

    for (auto task : tasks) {
        os::task_manager.add(task);
    }

    os::task_manager.add(&blink_task);
    blink_task.set_enabled(false);

    standby::init();

    int i = 0;

    while (1) {

        os::task_manager.run();

        if (i++ % 100 == 0) {
            void *heap_end = sbrk(0);
            printf_("Heap end: %p\n", heap_end);
        }

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
int _write(int, char *ptr, int len) {

    int DataIdx;

    for (DataIdx = 0; DataIdx < len; DataIdx++) {
        ITM_SendChar(*ptr++);
    }

    return len;
}

void _putchar(char c) {
    ITM_SendChar(c);
}

#endif
