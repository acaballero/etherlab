#pragma once

#include <stdint.h>
#include <stm32f4xx.h>

/* CMX973 - HAL GPIO linkage */
#define CMX973_CS_PIN GPIO_PIN_14
#define CMX973_CS_GPIO_PORT GPIOE
#define CMX973_SPI_HANDLE hspi4 // Defined in the parent project

/* Writable registers */
/*
 *      Divider registers
 *      -----------------

        These registers set the M divider value for the PLL (Feedback divider). The PLL dividers are only
        updated when $2C has been written, so this register should be written to last. Bits 7 and 5 also
        control the PLL and charge-pump blocks and these control bits are active as soon as $2C is written.
        (Note: To enable the PLL, b2 of the General Control Register ($1B) also needs to be set).

M17:M0
        Phase Locked Loop M divider value.

CP
        $2C, b5 = ’1’ enables the charge pump, $2C b5 = ’0’ puts the charge pump into high-impedance
        mode.

LD_Synth
        Only write ‘0’ to b6 of $2C (when read, this shows the PLL lock status, see section 6.7.2).

E
        $2C, b7 = ’1’ enables the PLL; b7 = ’0’ disables the PLL – in this mode an external local oscillator
        may be supplied to the CMX973, see also section 5.4.3 and Table 15. (Note: To enable the PLL
        b2 of the General Control Register ($1B) also needs to be set).

$2C b4-b2

        Reserved, set to ‘0’.

        */

#define CMX973_GRR	0x1A // General reset
#define CMX973_GCR	0x1B // General control
#define CMX973_RXC	0x1C // RX Control
#define CMX973_RXM	0x1D // RX Mode
#define CMX973_TXC	0x1E // TX Control
#define CMX973_RXO	0x1F // RX Offset
#define CMX973_PLLM	0x2A // PLL M Divider (0x2A-0x2C)
#define CMX973_PLLR	0x2D // PLL R Divider (0x2D-0x2E)
#define CMX973_VCOR	0x2F // VCO Control

/* Readable registers */
#define CMX973_GCR_R	0xEB // General control
#define CMX973_RXC_R	0xEC // RX Control
#define CMX973_RXM_R	0xED // RX Mode
#define CMX973_TXC_R	0xEE // TX Control
#define CMX973_RXO_R	0xEF // RX Offset
#define CMX973_PLLM_R	0xDA // PLL M Divider (0xDA-0xDC)
#define CMX973_PLLR_R	0xDD // PLL R Divider (0xDD-0xDE)
#define CMX973_VCOR_R	0xDF // VCO Control

/* General control register bit definitions */
#define CMX973_GRR_RXDIV					(1 << 7)
#define CMX973_GRR_TXDIV					(1 << 6)
#define CMX973_GRR_DIFAMP					(1 << 5)
#define CMX973_GRR_ENBIAS					(1 << 4)
#define CMX973_GRR_VCOEN					(1 << 3)
#define CMX973_GRR_PLLEN					(1 << 2)
#define CMX973_GRR_RXEN	    				(1 << 1)
#define CMX973_GRR_TXEN 					(1 << 0)

/* RX control register bit definitions */
#define CMX973_RXC_OUTDRV					(1 << 7)
#define CMX973_RXC_COR				    	(1 << 6)
#define CMX973_RXC_ZERO				    	(1 << 5)
#define CMX973_RXC_VGB2				    	(1 << 4)
#define CMX973_RXC_VGB1				    	(1 << 3)
#define CMX973_RXC_VGB0				    	(1 << 2)
#define CMX973_RXC_VGA1	    				(1 << 1)
#define CMX973_RXC_VGA0 					(1 << 0)
#define CMX973_RXC_VGAMSK 					0x03
#define CMX973_RXC_VGBMSK 					0x1C


/* RX mode register bit definitions */
#define CMX973_RXM_M1				        	(1 << 7)
#define CMX973_RXM_M0				          	(1 << 6)
#define CMX973_RXM_DIFAMPI				    	(1 << 5)
#define CMX973_RXM_DIFAMPQ				    	(1 << 4)
#define CMX973_RXM_FREQ3				    	(1 << 3)
#define CMX973_RXM_FREQ2				    	(1 << 2)
#define CMX973_RXM_FREQ1	    				(1 << 1)
#define CMX973_RXM_FREQ0				    	(1 << 0)

/* TX control register bit definitions */
#define CMX973_TXC_LOS				        	(1 << 7)
#define CMX973_TXC_ZERO				          	(1 << 6)
#define CMX973_TXC_ZERO2			        	(1 << 5)
#define CMX973_TXC_ZERO1			        	(1 << 4)
#define CMX973_TXC_F3				        	(1 << 3)
#define CMX973_TXC_F2				        	(1 << 2)
#define CMX973_TXC_F1	    			    	(1 << 1)
#define CMX973_TXC_F0				        	(1 << 0)

/* Specifications */
#define CMX973_MAX_OUT_FREQ		300000000ULL /* Hz */
#define CMX973_MIN_OUT_FREQ		20000000 /* Hz */
#define CMX973_MIN_VCO_FREQ		2200000000ULL /* Hz */
#define CMX973_MAX_FREQ_45_PRESC	3000000000ULL /* Hz */
#define CMX973_MAX_FREQ_PFD		32000000 /* Hz */
#define CMX973_MAX_BANDSEL_CLK		125000 /* Hz */
#define CMX973_MAX_FREQ_REFIN		250000000 /* Hz */
#define CMX973_MAX_MODULUS			4095
#define CMX973_MAX_R_CNT			1023

struct st_cms973Params {
    uint8_t lo_rx_div = 1; // 0: LO is divided by 4 on the CMX973, 1: LO is divided by 2
    uint8_t lo_tx_div = 1; // 0: LO is divided by 4 on the CMX973, 1: LO is divided by 2
    uint8_t rxc = 0; // RXC register
    uint8_t gcr = 0; // GCR register
};

extern st_cms973Params cmx973State;

/******************************************************************************/
/************************ Functions Declarations ******************************/
/******************************************************************************/
/* Initializes the CMX973. */
int32_t cmx973_setup();
/* Write a command CMX973. */
uint8_t cmx973_write_cmd(uint8_t cmd);
/* Write a register CMX973. */
uint8_t cmx973_write_reg(uint8_t reg,uint8_t data);
/* Write a register CMX973. */
uint8_t cmx973_read_reg(uint8_t reg,uint8_t &data);
/* Update registers with the current state */
uint8_t cmx973_update();
void cmx973_sleep();
void cmx973_wakeup();

