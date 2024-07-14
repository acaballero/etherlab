

#ifndef _FILTER_H
#define _FILTER_H

#include "main.h"
#include <arm_math.h>

#define MAX_FILTER_TAPS 1000

enum filterType {LPF, HPF, BPF};

void generateFIRFilterCoeffs(filterType filt_t, float32_t *m_taps, int m_num_taps, float fs, float fx,float fu);
void generateFIRFilterCoeffsq15(filterType filt_t, q15_t *m_taps, int m_num_taps, float fs, float fx,float fu);

#endif
