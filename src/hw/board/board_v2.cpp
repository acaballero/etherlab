//
// Created by Angel Dust on 29/05/2021.
//

#include "dsp/dsp_common.h"
#include "dsp/fft/fft.h"
#include "hw/stm32f4xx/timers.h"
#include "stdio.h"
#include "board_v2.h"
#include "../stm32.h"
#include "status.h"
#include "radio.h"
#include "main_board.h"
#include "../../../lib/CMX973/cmx973.h"
#include "../../../lib/ADF4351/adf4351.h"
#include "../../../lib/Si5351/si5351_I2C.h"
#include "stm32f4xx_hal_dac.h"
#include "stm32f4xx_hal_def.h"
#include "types.h"

namespace board {

bool change_drive_strength = false;
bool change_calibration = false;

void loop() {
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
}
os::periodic_task task(100, loop);

int16_t if_gain_to_db(IF_GAIN if_gain) {
    switch (if_gain) {
        case IF_GAIN_0:
            return 0;
        case IF_GAIN_MINUS6:
            return -6;
        case IF_GAIN_MINUS12:
            return -12;
        case IF_GAIN_MINUS18:
            return -18;
        case IF_GAIN_MINUS24:
            return -24;
        case IF_GAIN_MINUS30:
            return -30;
        default:
            status::pop_alert(status::ERROR, "Undefined IF_GAIN value");
            return 0;
    }
}

} // namespace board

Si5351 si5351;

adf4350_init_param adf4350Params = {

    .clkin = ADF4351_XTAL_FREQ,
    .channel_spacing = 500,
    .power_up_frequency = 40000000,
    .reference_div_factor = 0,
    .reference_doubler_enable = 0,
    .reference_div2_enable = 1,

    // r2_user_settings
    .phase_detector_polarity_positive_enable = 1,
    .lock_detect_precision_6ns_enable = 0,
    .lock_detect_function_integer_n_enable = 0,
    .charge_pump_current = 7, // Must match loop filter desiign
    .muxout_select = 6,       // 0: three-state, 1: VDD, 2: GND, 3: R counter, 4: N divider, 5: analog_lock, 6: digital lock
    .low_spur_mode_enable = 1,

    // r3_user_settings
    .cycle_slip_reduction_enable = 0, // Caution with this. Enabling it caused huge spurs at 2.5 KHz offset (with 500 Hz channel spacing. Offset increases with
                                      // spacing). Enabling it also requires 50% duty cycle reference so maybe div2 ref is also required
    .charge_cancellation_enable = 0,
    .anti_backlash_3ns_enable = 0,
    .band_select_clock_mode_high_enable = 1,
    .clk_divider_12bit = 0,
    .clk_divider_mode = 0,

    // r4_user_settings
    .aux_output_enable = 0,
    .aux_output_fundamental_enable = 0,
    .mute_till_lock_enable = 1,
    .output_power = 3, // 0:-4dbm,1:-1,2:+2,3:+5
    .aux_output_power = 0};

IF_GAIN vga_gain = config.hw.cmx973_vga;
IF_GAIN vgb_gain = config.hw.cmx973_vga;
// The IIP3 of the CMX973 depends on VGA/VGB settings
int cmx973_input_ip3;

si5351_drive lo_power_to_si5351_drive_strength(LO_POWER lo_power) {
    switch (lo_power) {
        case LO_POWER_LOW:
            return SI5351_DRIVE_2MA;
        case LO_POWER_MEDIUM:
            return SI5351_DRIVE_4MA;
        case LO_POWER_HIGH:
        default:
            return SI5351_DRIVE_6MA;
    }
}

uint8_t lo_power_to_adf4350_drive_strength(LO_POWER lo_power) {
    switch (lo_power) {
        case LO_POWER_LOW:
            return 1;
        case LO_POWER_MEDIUM:
            return 2;
        case LO_POWER_HIGH:
        default:
            return 3;
    }
}

void lo_strength(uint8_t stage, LO_POWER power) {

    switch (stage) {
        case 0:
            adf4350Params.output_power = lo_power_to_adf4350_drive_strength(power);
            adf4350_setup(adf4350Params);
            break;
        case 1:
            si5351.drive_strength(SI5351_2LO_CLK, lo_power_to_si5351_drive_strength(power));
            break;
        case 2:
            si5351.drive_strength(SI5351_IF_CLK, lo_power_to_si5351_drive_strength(power));
            break;
    }

    radio::update_freq();
}

bool if_freq(RF_DIRECTION direction, uint64_t freq) {

    si5351_clock clk = (direction == RF_DIRECTION_TX) ? SI5351_TX_CLK : SI5351_RX_CLK;
    uint8_t div = (direction == RF_DIRECTION_TX) ? cmx973State.lo_tx_div : cmx973State.lo_rx_div;
    HAL_StatusTypeDef ret = HAL_OK;

    if (freq == 0) {
        ret = si5351.output_enable(clk, false);
    } else {
        si5351.output_enable(clk, true);

        // A shift is applied so the frequency of interest does not lie around DC to avoid DC leakage and flickr noise

        freq += radio::get_dsp_frequency_shift();
        uint64_t f = freq * SI5351_FREQ_MULT * (div ? 2 : 4);

#if DEBUG_MSGS
        if (si5351.get_freq(clk) != f) {
            LOG("Setting DSP IF %s frequency: %llu (%d shift)\n", clk == SI5351_TX_CLK ? "TX" : "RX", freq, radio::get_dsp_frequency_shift());
        }
#endif

        ret = si5351.set_freq(f, clk);
    }

    return ret == HAL_OK;
}

int calc_max_input_dbm() {
    // Based on estimations from the datasheet of the CMX973
    if (vga_gain == IF_GAIN_0 && vgb_gain == IF_GAIN_0) {
        return -56;
    } else if (vga_gain <= IF_GAIN_MINUS18 && vgb_gain == IF_GAIN_0) {
        return -56 - board::if_gain_to_db(vga_gain) / 2;
    } else {
        return -56 - (board::if_gain_to_db(vga_gain) + board::if_gain_to_db(vgb_gain)) / 2;
    }
}

void if_gain(RF_DIRECTION direction, IF_GAIN vga, IF_GAIN vgb) {

    if (vga > IF_GAIN_MINUS18) {
        vga = IF_GAIN_MINUS18;
    }

    vga_gain = vga;
    vgb_gain = vgb;

    if (direction == RF_DIRECTION_RX) {
        cmx973State.rxc = (cmx973State.rxc & ~CMX973_RXC_VGAMSK) | (vga << 0); // VGB gain
        cmx973State.rxc = (cmx973State.rxc & ~CMX973_RXC_VGBMSK) | (vgb << 2); // VGB gain
        uint8_t ret = cmx973_update();
        if (ret) {
            // LOG("Error updating CMX973: %d", ret)
        }

        cmx973_input_ip3 = calc_max_input_dbm();

    } else {
        status::pop_alert(status::ERROR, "The IF gain can't be changed in TX direction");
    }
}

int get_max_input_dbm() {
    return cmx973_input_ip3;
}

/**
 * Returns the overall gain
 * @return
 */
int board_gain() {
    return board::if_gain_to_db(vga_gain) + board::if_gain_to_db(vgb_gain) +
           59; // 60 is the total approximate gain of the CMX937 given current settings, minus 1 to account for the filter loss
}

void lo_enable(uint8_t stage, bool enabled) {

    LOG("LO for mixer #%d %s\n", stage, enabled ? "enabled" : "disabled");
    switch (stage) {
        case 0:
            status::pop_alert(status::ERROR, "The 1st LO can't be disabled");
            break;
        case 1:
            si5351.output_enable(SI5351_2LO_CLK, enabled);
            break;
        case 2:
            si5351.output_enable(SI5351_IF_CLK, enabled);
            break;
    }
}

bool lo_freq(uint8_t stage, uint64_t freq) {

    bool ok = false;

    LOG("Setting mixer #%d LO frequency : %llu\n", stage, freq);
    switch (stage) {
        case 0:

            ok = adf4350_out_frequency(freq) > 0;
            break;
        case 1:
            ok = si5351.set_freq(freq * SI5351_FREQ_MULT, SI5351_2LO_CLK) == HAL_OK;
            break;
        case 2:
            ok = si5351.set_freq(freq * SI5351_FREQ_MULT, SI5351_IF_CLK) == HAL_OK;
            ;
            break;
    }

    if (!ok) {
        status::pop_alert(status::ERROR, "Error setting frequency");
    }
    return ok;
}

void lo_setup() {
    // The adf4350 doesn't have a frequency offset setup (like the si5351 has) so we're correcting the frequency
    // each time we change it using the value in config.f_correction
    adf4350Params.clkin = ADF4351_XTAL_FREQ + config.f_correction;
    adf4350Params.output_power = lo_power_to_adf4350_drive_strength(config.lo_drive_strength_0);
    auto ret = adf4350_setup(adf4350Params);

    LOG("ADF4351 setup: %s\n", ret == 0 ? "OK" : "ERR");
}

void calibrate_freq() {
    si5351.set_correction(config.if_correction * SI5351_FREQ_MULT, SI5351_PLL_INPUT_XO);

    // PLLB frequency is fixed (so we can use it as VCXO and pull its frequency)
    // si5351.set_freq_manual(config.f_2nd_lo*SI5351_FREQ_MULT,SI5351_PLLB_FREQ,SI5351_2LO_CLK,0);
    si5351.set_freq(radio::mixers[1].getLo() * SI5351_FREQ_MULT, SI5351_2LO_CLK);

    lo_setup();
}

/*
 * Sets the direction of the quadrature mod/demod
 */
void if_direction(RF_DIRECTION direction) {

    LOG("Setting DSP IF path: %s\n", radio::rf_path_names[direction]);

    switch (direction) {

        case RF_DIRECTION_TX:

            cmx973State.gcr = CMX973_GRR_ENBIAS | CMX973_GRR_TXEN | (cmx973State.lo_rx_div ? CMX973_GRR_TXDIV : 0);

            // RF Switch configuration
            // Keep V1 HIGH, V2 LOW for full duplex configuration or separated RX/TX paths
            /*
            HAL_GPIO_WritePin(RX_SW_V1_GPIO_PORT, RX_SW_V1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(RX_SW_V2_GPIO_PORT, RX_SW_V2_PIN, GPIO_PIN_SET);
             */
            break;
        case RF_DIRECTION_RX:

            cmx973State.gcr = CMX973_GRR_ENBIAS | CMX973_GRR_RXEN | (cmx973State.lo_rx_div ? CMX973_GRR_RXDIV : 0);

            // RF Switch
            HAL_GPIO_WritePin(RX_SW_V1_GPIO_PORT, RX_SW_V1_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(RX_SW_V2_GPIO_PORT, RX_SW_V2_PIN, GPIO_PIN_RESET);
            break;
        case RF_DIRECTION_OFF:
            cmx973State.gcr = CMX973_GRR_ENBIAS | (cmx973State.lo_rx_div ? CMX973_GRR_RXDIV : 0);
            break;
    }

    uint8_t ret = cmx973_update();
    if (ret) {
        // LOG("Error updating CMX973: %d", ret)
    }
}

void if_setup() {

    cmx973_write_cmd(CMX973_GRR); // Reset

    cmx973State.rxc = CMX973_RXC_COR | CMX973_RXC_OUTDRV; // I/Q balance correction and increased output drive enabled

    uint8_t ret = cmx973_update();

    if (ret) {
        status::pop_alert(status::ERROR, "Error updating CMX973");
    }

    // if_gain(RF_DIRECTION_RX, config.hw.cmx973_vga, config.hw.cmx973_vgb);

    if_direction(RF_DIRECTION_RX); // Receive

    // The variable gain before and after the mixer are left to their
    // default value, which is 0 (max gain)

    bool b = si5351.init(Si5351_I2C_HANDLE, SI5351_CRYSTAL_LOAD_10PF, SI5351_XTAL_FREQ, config.f_correction);

    if (!b) {
        // LOG("Error initalizing Si5351\n", 0);
    }

    /*** TEST ***/
    // si5351.set_ref_freq(25000000, SI5351_PLL_INPUT_CLKIN);
    // si5351.pll_reset(SI5351_PLLB);

    // si5351.set_pll_input(SI5351_PLLB, SI5351_PLL_INPUT_CLKIN);
    // si5351.set_clock_source(SI5351_CLK1, SI5351_CLK_SRC_MS);
    // si5351.update_status();

    //   si5351.set_correction(config.if_correction*SI5351_FREQ_MULT, SI5351_PLL_INPUT_XO);
    //  si5351.set_vcxo(SI5351_PLLB_FREQ,100);

    // si5351.set_ms_source(SI5351_CLK1, SI5351_PLLB);
    // si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_2MA);

    // si5351.set_freq(50000000 * SI5351_FREQ_MULT, SI5351_CLK1);

    //
    //   PLLB frequency is fixed (so we can use it as VCXO and pull its frequency)
    //

    // si5351.set_freq_manual(80000000*SI5351_FREQ_MULT,SI5351_PLLB_FREQ,SI5351_CLK1,0);

    // si5351.set_clock_source(SI5351_CLK1, SI5351_CLK_SRC_XTAL);

    // si5351.update_status();

    // si5351.output_enable(SI5351_CLK1, 0);

    // si5351.write_regs(const_cast<si5351b_revb_register_t *>(si5351b_revb_registers), sizeof si5351b_revb_registers / sizeof si5351b_revb_registers[0]);

    // Set the PLLs reference from crystal
    si5351.set_pll_input(SI5351_PLLA, SI5351_PLL_INPUT_XO);
    si5351.set_pll_input(SI5351_PLLB, SI5351_PLL_INPUT_XO);

    // Quadrature mod/demod RX/TX clocks
    si5351.set_ms_source(SI5351_RX_CLK, SI5351_PLLB);
    si5351.drive_strength(SI5351_RX_CLK, SI5351_DRIVE_2MA);
    si5351.set_ms_source(SI5351_TX_CLK, SI5351_PLLB);
    si5351.drive_strength(SI5351_TX_CLK, SI5351_DRIVE_2MA);

    // 2nd LO
    si5351.set_ms_source(SI5351_2LO_CLK, SI5351_PLLA);
    si5351.drive_strength(SI5351_2LO_CLK, lo_power_to_si5351_drive_strength(config.lo_drive_strength_1));

    // 3rd LO (IF LO)
    si5351.set_ms_source(SI5351_IF_CLK, SI5351_PLLA);
    si5351.drive_strength(SI5351_IF_CLK, lo_power_to_si5351_drive_strength(config.lo_drive_strength_1));
    si5351.output_enable(SI5351_IF_CLK, 0);

    // si5351.set_vcxo(SI5351_PLLB_FREQ,100);

    calibrate_freq();
}

bool radio_config(st_radio_config radioConfig) {

    LOG_IND(2, "radio_config | direction: %s | mode: %s | sample rate: %d\n", radio::rf_path_names[radioConfig.direction],
            radioConfig.mode == ANALOG ? "Analog" : "DSP", radioConfig.sample_freq);

    // Disable DAC audio output
    if_freq(RF_DIRECTION_TX, 0);
    if_freq(RF_DIRECTION_RX, 0);
    if_direction(RF_DIRECTION_OFF);
    DAC_DMA_Stop(&hdac1);
    ADC_DMA_Stop(&hadc1);

    if (radioConfig.direction == RF_DIRECTION_TX) {

        bool ret = main_board::set_mode(DIGITAL_TX);

        if (!ret) {
            return false;
        }

        // Stop DMA, set quadrature IF
        ADC_DMA_Stop(&hadc1);

        // We need to set the frequency for the IF value to be calculated
        radio::update_freq();

        // One clock must be stopped before setting the other. If they are on CLK6 & CLK7, they can't be simultaneously
        // set to a fractional divider
        if_freq(RF_DIRECTION_RX, 0);
        if_freq(RF_DIRECTION_TX, radio::f_dsp_if);

        // Enable DAC for IF modulation
        MX_DAC_Init();

        set_timer_sample_rate(DAC_TIMER, DAC_TIMER_CLOCK_HZ, radioConfig.sample_freq, MAX_DSP_DECIMATION_FACTOR);
        DAC_DMA_Start(&hdac1);

        // Starting the DAC causes a DC transient. Wait for it to stop

        if_direction(RF_DIRECTION_TX);

    } else if (radioConfig.direction == RF_DIRECTION_RX) {

        // Disable DAC audio output
        if_freq(RF_DIRECTION_TX, 0);
        DAC_DMA_Stop(&hdac1);

        if (radioConfig.mode == ANALOG) {

            main_board::set_mode(ISTX ? ANALOG_TX : ANALOG_RX);

            // Stop TX quadrature clocks
            if_freq(RF_DIRECTION_TX, 0);
            if_freq(RF_DIRECTION_RX, radio::f_dsp_if);

            if_direction(RF_DIRECTION_RX);

        } else { // DSP

            main_board::set_mode(DIGITAL_RX);

            if_freq(RF_DIRECTION_TX, 0);
            if_freq(RF_DIRECTION_RX, radio::f_dsp_if);

            if_direction(RF_DIRECTION_RX);

            // Enable DAC for audio output
            MX_DAC_Init();
            //  HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
            //  HAL_DAC_Start(&hdac1, DAC_CHANNEL_2);

            // Instead of calculating the DAC timer period from the audio output sample rate, which would generate
            // a phase mismatch between them when not integer prescaler and period can be found for the target frequencies,
            // the sample rate (period and prescaler) are derived from those of the ADC timer.

            // NOTE: Have in mind that, when dual interleaved DAC is used, the nyquist frequency is HALF the sample frequency that we set here

            if (fft::fft_params.sample_freq % radioConfig.sample_freq != 0) {
                LOG("Error setting DAC_TIMER for DIGITAL_RX: The ADC/DAC sample rates (%d/%d) is not integer. Their phases will slide!\n",
                    fft::fft_params.sample_freq, radioConfig.sample_freq);
                return false;
            } else {
                int ratio = ((float32_t)fft::fft_params.sample_freq / radioConfig.sample_freq) * ((float)ADC_DMA_TIMER_CLOCK_HZ / (float)DAC_TIMER_CLOCK_HZ);
                uint32_t adc_timer_real_freq = get_adc_timer_frequency();

                LOG("Setting DAC_TIMER for DIGITAL_RX: DAC target: %llu | FFT target sample freq: %d | ", radioConfig.sample_freq, fft::fft_params.sample_freq);
                LOG_RAW("ADC real freq: %d | ratio: %d | result: %d\n", adc_timer_real_freq, ratio, adc_timer_real_freq / ratio);
                set_timer_sample_rate(DAC_TIMER, DAC_TIMER_CLOCK_HZ, adc_timer_real_freq / ratio);

                // TODO: Use only one DAC instead of two in quadrature
                DAC_DMA_Start(&hdac1);
            }
        }
        ADC_DMA_Start(&hadc1);
    }

    LOG_IND(-2, "radio_config: Finished\n");
    return true;
}

void setup_board_peripherals() {

    lo_setup();
    if_setup(); // IF mod/demod setup
}

int power_down_lo_clocks() {
    uint8_t ret = 0;
    ret = si5351.sleep();
    int32_t status = adf4350_out_powerdown(true);
    cmx973_sleep();
    return status >= 0 && ret == 0 ? 0 : -1;
}

int power_up_lo_clocks() {
    cmx973_wakeup();
    uint8_t ret = 0;
    ret = si5351.wakeup();
    int32_t status = adf4350_out_powerdown(false);
    return status >= 0 && ret == 0 ? 0 : -1;
}
