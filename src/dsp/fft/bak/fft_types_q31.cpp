//
// Created by Angel Dust on 25/04/2021.
//
/*
#include "fft_types_q31.h"


#if FFT_N==64

const q31_t *twiddle = twiddleCoef_64_q31;
const uint16_t *bitRevTable = armBitRevIndexTable_fixed_64;
uint16_t bitRevTableLength = ARMBITREVINDEXTABLE_FIXED___64_TABLE_LENGTH;

#elif FFT_N==128

const q31_t *twiddle = twiddleCoef_128_q31;
const uint16_t *bitRevTable = armBitRevIndexTable_fixed_128;
uint16_t bitRevTableLength = ARMBITREVINDEXTABLE_FIXED__128_TABLE_LENGTH;

#elif FFT_N==256

const q31_t *twiddle = twiddleCoef_256_q31;
const uint16_t *bitRevTable = armBitRevIndexTable_fixed_256;
uint16_t bitRevTableLength = ARMBITREVINDEXTABLE_FIXED__256_TABLE_LENGTH;

#endif

// cfft instance
arm_cfft_instance_q31 S_cfft =
        {FFT_N,
         twiddle,
         bitRevTable,
         bitRevTableLength
        };

void (*arm_cfft)(
        const arm_cfft_instance_q31 *,
        q31_t *,
        uint8_t,
        uint8_t) = arm_cfft_q31;

void (*arm_cmplx_mag)(
        q31_t *pSrc,
        q31_t *pDst,
        uint32_t numSamples) = arm_cmplx_mag_q31;

int32_t window[FFT_N] = {2621<<16, 2693<<16, 2910<<16, 3270<<16, 3768<<16, 4401<<16, 5161<<16, 6042<<16, 7036<<16, 8132<<16, 9320<<16, 10588<<16, 11926<<16, 13318<<16,
                         14753<<16,
                         16216<<16, 17694<<16, 19171<<16, 20634<<16, 22069<<16, 23462<<16, 24799<<16, 26068<<16, 27256<<16, 28352<<16, 29345<<16, 30226<<16,
                         30987<<16,
                         31619<<16, 32117<<16, 32477<<16, 32694<<16, 32766<<16, 32694<<16, 32477<<16, 32117<<16, 31619<<16, 30987<<16, 30226<<16, 29345<<16,
                         28352<<16,
                         27256<<16, 26068<<16, 24799<<16, 23462<<16, 22069<<16, 20634<<16, 19171<<16, 17694<<16, 16216<<16, 14753<<16, 13318<<16, 11926<<16,
                         10588<<16,
                         9320<<16, 8132<<16, 7036<<16, 6042<<16, 5161<<16, 4401<<16, 3768<<16, 3270<<16, 2910<<16, 2693<<16};

                         */