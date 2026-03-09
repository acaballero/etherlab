#include "utils.hpp"
#include "arm_math.h"

#define ATANHALFF32 0.463648f
#define PIHALFF32 1.5707963267948966192313f

#define ATAN2_NB_COEFS_F32 10

static const float32_t atan2_coefs_f32[ATAN2_NB_COEFS_F32] = {0.0f,
                                                              1.0000001638308195518f,
                                                              -0.0000228941363602264f,
                                                              -0.3328086544578890873f,
                                                              -0.004404814619311061f,
                                                              0.2162217461808173258f,
                                                              -0.0207504842057097504f,
                                                              -0.1745263362250363339f,
                                                              0.1340557235283553386f,
                                                              -0.0323664125927477625f};

__STATIC_FORCEINLINE float32_t arm_atan_limited_f32(float32_t x) {
    float32_t res = atan2_coefs_f32[ATAN2_NB_COEFS_F32 - 1];
    int i = 1;
    for (i = 1; i < ATAN2_NB_COEFS_F32; i++) {
        res = x * res + atan2_coefs_f32[ATAN2_NB_COEFS_F32 - 1 - i];
    }

    return (res);
}

__STATIC_FORCEINLINE float32_t arm_atan_f32(float32_t x) {
    int sign = 0;
    float32_t res = 0.0f;

    if (x < 0.0f) {
        sign = 1;
        x = -x;
    }

    if (x > 1.0f) {
        x = 1.0f / x;
        res = PIHALFF32 - arm_atan_limited_f32(x);
    } else {
        res += arm_atan_limited_f32(x);
    }

    if (sign) {
        res = -res;
    }

    return (res);
}

bool atan2_f32(float32_t y, float32_t x, float32_t *result) {
    if (x > 0.0f) {
        *result = arm_atan_f32(y / x);
        return (ARM_MATH_SUCCESS);
    }
    if (x < 0.0f) {
        if (y > 0.0f) {
            *result = arm_atan_f32(y / x) + PI;
        } else if (y < 0.0f) {
            *result = arm_atan_f32(y / x) - PI;
        } else {
            if (signbit(y)) {
                *result = -PI;
            } else {
                *result = PI;
            }
        }
        return true;
    }
    if (x == 0.0f) {
        if (y > 0.0f) {
            *result = PIHALFF32;
            return true;
        }
        if (y < 0.0f) {
            *result = -PIHALFF32;
            return true;
        }
    }

    return false;
}
