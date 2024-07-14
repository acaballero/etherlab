//
// Created by Angel Dust on 25/04/2021.
//
/*
#include "fft_types_q15.h"


#if FFT_N==64

const q15_t *twiddle = twiddleCoef_64_q15;
const uint16_t *bitRevTable = armBitRevIndexTable_fixed_64;
uint16_t bitRevTableLength = ARMBITREVINDEXTABLE_FIXED___64_TABLE_LENGTH;

#elif FFT_N==128

const q15_t *twiddle = twiddleCoef_128_q15;
const uint16_t *bitRevTable = armBitRevIndexTable_fixed_128;
uint16_t bitRevTableLength = ARMBITREVINDEXTABLE_FIXED__128_TABLE_LENGTH;

elif FFT_N==256

const q15_t *twiddle = twiddleCoef_256_q15;
const uint16_t *bitRevTable = armBitRevIndexTable_fixed_256;
uint16_t bitRevTableLength = ARMBITREVINDEXTABLE_FIXED__256_TABLE_LENGTH;

#endif

// cfft instance
arm_cfft_instance_q15 S_cfft =
        {FFT_N,
         twiddle,
         bitRevTable,
         bitRevTableLength
        };

void (*arm_cfft)(
        const arm_cfft_instance_q15 *,
        q15_t *,
        uint8_t,
        uint8_t) = arm_cfft_q15;

void (*arm_cmplx_mag)(
        q15_t *pSrc,
        q15_t *pDst,
        uint32_t numSamples) = arm_cmplx_mag_q15;

int16_t window[FFT_N] = {2621, 2693, 2910, 3270, 3768, 4401, 5161, 6042, 7036, 8132, 9320, 10588, 11926, 13318,
                         14753,
                         16216, 17694, 19171, 20634, 22069, 23462, 24799, 26068, 27256, 28352, 29345, 30226,
                         30987,
                         31619, 32117, 32477, 32694, 32766, 32694, 32477, 32117, 31619, 30987, 30226, 29345,
                         28352,
                         27256, 26068, 24799, 23462, 22069, 20634, 19171, 17694, 16216, 14753, 13318, 11926,
                         10588,
                         9320, 8132, 7036, 6042, 5161, 4401, 3768, 3270, 2910, 2693}; */