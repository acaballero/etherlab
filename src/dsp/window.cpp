//
// Created by Angel Dust on 19/10/2019.
//
#include "math.h"
#include "window.h"

// Calculate a Hann window function
void hann_window(float *window,int size)
{

    int ii;

    for (ii = 0; ii < size; ii++) {
        window[ii] = ( 0.5 * (1.0 - cos((2 * M_PI * ii) / (size -1))));
    }
}

// Calculate a Hamming window function
void hamm_window(float *w,int size)
{

    int ii;
    const float a0 = 0.35875;
    const float b0 = 0.48829;

    for (ii = 0; ii < size; ii++) {
        w[ii] = (a0 - b0 * (cos((2 * M_PI * ii) / (size -1))) );
    }
}

// Calculate a Blackman window function
void black_window(float *w,int size)
{

    int ii;
    const float a0 = (7938.0/18608.0);
    const float a1 = (9240.0/18608.0);
    const float a2 = (1430.0/18608.0);

    for (ii = 0; ii < size; ii++) {
        w[ii] = (a0 - a1 * (cos((2 * M_PI * ii) / (size -1))) + a2 * (cos((4 * M_PI * ii) / (size -1))) );
    }
}

// Calculate a Blackman-Harris window function
void blackharris_window(float *w,int size)
{

    int ii;
    const float a0 = 0.35875;
    const float a1 = 0.48829;
    const float a2 = 0.14128;
    const float a3 = 0.01168;

    for (ii = 0; ii < size; ii++) {
        w[ii] = (a0 - a1 * (cos((2 * M_PI * ii) / (size -1))) + a2 * (cos((4 * M_PI * ii) / (size -1))) - a3 * (cos((6 * M_PI * ii) / (size -1))));
    }
}

void generate_window(int win,float *window, int size)
{
    switch (win) {
        case 0:
            break;
        case 1:
            hann_window(window,size);
            break;
        case 2:
            hamm_window(window,size);
            break;
        case 3:
            black_window(window,size);
            break;
        case 4:
            blackharris_window(window,size);
            break;
    }
}

