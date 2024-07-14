/*
 * FIR filter class, by Mike Perkins
 * 
 * a simple C++ class for linear phase FIR filtering
 *
 * For background, see the post http://www.cardinalpeak.com/blog?p=1841
 *
 * Copyright (c) 2013, Cardinal Peak, LLC.  http://www.cardinalpeak.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1) Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2) Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 
 * 3) Neither the name of Cardinal Peak nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * CARDINAL PEAK, LLC BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "firFilter.h"
#include <arm_math.h>

static double sinc(const double x) {
    if (x == 0)
        return 1;

    return sin(M_PI * x) / (M_PI * x);
}

void designLPF(float *m_taps, int m_num_taps, float fs, float fx) {

    int n;

    double f = fx / fs;

    for (n = 0; n < m_num_taps; n++) {
        int nn = n - int(m_num_taps / 2);
        m_taps[n] = 2.0 * f * sinc(2.0 * f * (double) nn);
    }


}

void designHPF(float *m_taps, int m_num_taps, float Fs, float Fx) {

    int n;
    double mm;

    float m_lambda = M_PI * Fx / (Fs / 2);

    for (n = 0; n < m_num_taps; n++) {
        mm = n - (m_num_taps - 1.0) / 2.0;
        if (mm == 0.0) m_taps[n] = 1.0 - m_lambda / M_PI;
        else m_taps[n] = -sin(mm * m_lambda) / (mm * M_PI);
    }

}

void designBPF(float *m_taps, int m_num_taps,
               float Fs,
               float Fx,
               float Fu
) {
    int n;
    double mm;

    float m_lambda = M_PI * Fx / (Fs / 2);
    float m_phi = M_PI * Fu / (Fs / 2);

    for (n = 0; n < m_num_taps; n++) {

        mm = n - (m_num_taps - 1.0) / 2.0;

        if (mm == 0.0) {
            m_taps[n] = (m_phi - m_lambda) / M_PI;
        } else {
            m_taps[n] = (sin(mm * m_phi) - sin(mm * m_lambda)) / (mm * M_PI);
        }
    }

    return;
}


// Handles LPF and HPF case
void generateFIRFilterCoeffs(filterType filt_t, float32_t *m_taps, int m_num_taps, float fs, float fx, float fu) {

    if (
            (fs >= 0)
            && (fx >= 0 && fx < fs / 2)
            && (m_num_taps >= 0 && m_num_taps <= MAX_FILTER_TAPS)
            ) {


        if (filt_t == LPF) designLPF(m_taps, m_num_taps, fs, fx);
        else if (filt_t == HPF) designHPF(m_taps, m_num_taps, fs, fx);
        else designBPF(m_taps, m_num_taps, fs, fx, fu);


    }
}

void generateFIRFilterCoeffsq15(filterType filt_t, q15_t *m_taps, int m_num_taps, float fs, float fx, float fu) {

   float f_taps[m_num_taps];
   generateFIRFilterCoeffs(filt_t,f_taps,m_num_taps,fs,fx,fu);
   arm_float_to_q15(f_taps,m_taps,m_num_taps);
}










