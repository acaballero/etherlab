#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdint.h>
#include <stddef.h>
#include "dsp_common.h"

struct Timestamp {
    uint32_t tv_date{0};
    uint32_t tv_time{0};
};


template<typename T>
struct buffer_t {

    T *const p;
    size_t count;
    uint32_t sample_rate;
    const Timestamp timestamp;
    uint32_t decimation_factor=1;
    uint32_t decimated_size_bytes=0;
    const uint32_t size_bytes=0;

    constexpr buffer_t() : p{nullptr},
                           count{0},
                           sample_rate{0},
                           timestamp{},
                           decimation_factor{1},
                           decimated_size_bytes{0},
                           size_bytes{count* sizeof(p[0])} {
    }

    constexpr buffer_t(const buffer_t<T> &other) : p{other.p},
                                                   count{other.count},
                                                   sample_rate{other.sample_rate},
                                                   timestamp{other.timestamp},
                                                   decimation_factor{other.decimation_factor},
                                                   decimated_size_bytes{other.decimated_size_bytes},
                                                   size_bytes{other.size_bytes}{
    }

    constexpr buffer_t(T *const p,
                       const size_t count,
                       const uint32_t sampling_rate = 0,
                       const Timestamp timestamp = {},
                       const uint32_t decimation_factor=1,
                       const uint32_t decimated_size_bytes=0
    ) : p{p},
        count{count},
        sample_rate{sampling_rate},
        timestamp{timestamp},
        decimation_factor{decimation_factor},
        decimated_size_bytes{decimated_size_bytes},
        size_bytes{count* sizeof(p[0])} {
    }

    operator bool() const {
        return (p != nullptr);
    }
};

#endif/*__BUFFER_H__*/
