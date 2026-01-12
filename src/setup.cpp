//
// Created by Angel Dust on 06/03/2021.
//

#include "setup.h"
#include "hw/stm32.h"
#include "hw/hw_config.h"
#include "hw/stm32f4xx/eeprom.h"
#include "memory_allocator.h"
#include "stm32f4xx_hal.h"
#include "ui/menu.h"
#include "settings.h"
#include "status.h"
#include "ui/lcd.h"
#include "dsp/dsp.h"
#include "input/input_controller.h"
#include "main_board.h"
#include "usb/tinyusb/usb_composite_device.h"

// - FATFS -
#if ENABLE_SD_CARD

#include "fatfs/fatfs.h"
#include "radio.h"

#endif
// - END FATFS -

void initPowerControl() {

    LOG("Setting up mother board power rails\n");

    // Make sure all power lines start off
    main_board::PowControlShiftReg.write(0);

    // Initialises the MCP23017 GPIO expansion port
    BitBangI2C_setup();

    // Enables the 74HC595 shift register output (driving its OE pin to LOW).
    main_board::setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_ENABLE_POW_CTRL_SHIFT_REG, false);

    // Note that, at this point, the MCP32017 ports are just been set to outputs (BitBangI2C_setup),
    // changing their state from high impedance (they are inputs at startup) to a driven LOW level.
    // This makes the next statement unnecessary. But anyway, I leave it for clarity
}

// extern void initialise_monitor_handles(void);
void setup() {

    bool b;
    // No need to init the SWO here. OpenOCD / ST-Link will initialise it. Otherwise, as ITM_SendChar is blocking, if we initialise
    // it when there's no ST-Link connected, it will freeze the MCU.
    //  SWO_Init(0x1, CPU_CORE_FREQUENCY_HZ);

    HAL_Init();

    CCMMemoryAllocator::init();

    SystemClock_Config();

    HAL_Delay(100);

    MX_GPIO_Init();

    //  DWT_Init(); // Enable hardware profiling

#if USB_ENABLED
    usb_composite_init();
#endif

    // Notice: Initialize any MCP23017 feature after BitbangI2C (initPowerControl)
    initPowerControl();

    MX_DMA_Init();

    setup_adcs();

    setup_timers();

    // Notice: Input controller depends on ADC, so make sure it is initialized after the ADC
    inputControllerInit();

    b = setup_connectivity();

#if ENABLE_SD_CARD
    if (!b) {
        LOG("SDIO initialization error. Card not present or failed.\n");
    } else {
        sdcard_init();
    }

#endif

#if ENABLE_RTC
    MX_RTC_Init();
#endif

    // These lines are commented because we are enabling and disabling the DAC whenever is needed
    // DAC output#1
    // MX_DAC_Init();
    // DAC routed to OPAMP4 in follower mode
    // MX_OPAMP4_Init();

    /* EEPROM Init */
    if (EE_Init() != EE_OK) {
        Error_Handler();
    }

    LOG("Reading settigns\n");

    if (settings_read(&config) != EE_OK) {

        // Wrong config version, write the new one
        settings_write(&config);
        status::pop_alert(status::ERROR, "Settings read error");
    }

    main_board::init();

    lcd_init();

    menu_setup();

    fft_init();

#if DSP_ENABLED
    dsp_init(config.dsp);

#endif

    radio::set_band();

#if USE_FRAmE_BUFFER && LCD_ENABLED
    lcd.renderAll();
#endif
}
