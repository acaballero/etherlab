

#ifndef _FIR_FILTER_H
#define _FIR_FILTER_H

#include "main.h"
#include <arm_math.h>

#define MAX_FILTER_TAPS 1000

enum filter_type { LPF, HPF, BPF };

bool generate_fir_filter_taps(filter_type filt_t, float32_t *m_taps, int m_num_taps, float fs, float fx, float fu);
bool generate_fir_filter_taps_q15(filter_type filt_t, q15_t *m_taps, int m_num_taps, float fs, float fx, float fu);
void generate_raisedcosine_taps(float32_t *taps, int num_taps, int samples_per_symbol, float beta = 0.35f);
#endif
