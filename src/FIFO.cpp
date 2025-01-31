//
// Created by Angel Dust on 09/04/2021.
//

#include "FIFO.h"
#include <string.h> // memcpy

#define FIFO_INCR_IX(ix, n)                                                                                                                                    \
    {                                                                                                                                                          \
        ix++;                                                                                                                                                  \
        if (ix == n)                                                                                                                                           \
            ix = 0;                                                                                                                                            \
    }
#define FREE() ((uint32_t)(size - count))
#define AVAILABLE() ((uint32_t)this->count)

void FIFO::reset() {
    this->read_ix = this->write_ix = 0;
    this->count = 0;
}

uint32_t FIFO::available() { return AVAILABLE(); };

uint32_t FIFO::available(char **dest) {
    *dest = this->data + this->read_ix;
    return this->available();
};

uint32_t FIFO::free() { return FREE(); };

uint32_t FIFO::free(char **start) {
    *start = this->data + this->write_ix;
    return FREE();
};

FIFO_ERROR FIFO::write(char *origin, uint32_t n) {

    // Don't use this to store more than 1 item when possible. It's slow

    uint32_t next_ix = this->write_ix;
    FIFO_INCR_IX(next_ix, this->size);

    while (next_ix != this->read_ix && n) {

        this->data[this->write_ix] = *(origin++);
        this->write_ix = next_ix;
        FIFO_INCR_IX(next_ix, this->size);
        n--;
    }

    this->count += n;

    return n ? FIFO_ERROR_OVERRUN : FIFO_ERROR_NONE;
}

/*
 * Writes a block
 * TODO: Caution. This function assumes the block is a divisor of the FIFO size
 */
FIFO_ERROR FIFO::writeBlock(char *origin, uint32_t n) {

    char *p = this->data + this->write_ix;
    uint32_t free = FREE();

    if (free >= n) {
        memcpy(p, origin, n);
        this->feed_unsafe(n);
        return FIFO_ERROR_NONE;
    } else {
        return FIFO_ERROR_OVERRUN;
    }
}

inline void FIFO::feed_unsafe(uint32_t n) {
    this->write_ix += n;
    this->count += n;
    if (this->write_ix >= this->size) {
        this->write_ix = this->write_ix - this->size;
    }
}

FIFO_ERROR FIFO::feed(uint32_t n) {

    uint32_t free = this->free();
    if (free >= n) {
        feed_unsafe(n);
        return FIFO_ERROR_NONE;
    } else {
        return FIFO_ERROR_OVERRUN;
    }
}

/*
 * Returns a pointer to the read position
 */
FIFO_ERROR FIFO::consume(uint32_t n, char **dest) {

    uint32_t av = AVAILABLE();
    if (av < n) {
        return FIFO_ERROR_UNDERRUN;
    } else {
        *dest = this->data + this->read_ix;
        this->read_ix += n;
        if (this->read_ix >= this->size) {
            this->read_ix = this->read_ix - this->size;
        }
        this->count -= n;
        return FIFO_ERROR_NONE;
    }

    //    while (this->read_ix!=this->write_ix && n) {
    //        FIFO_INCR_IX(this->read_ix,this->size);
    //        n--;
    //    }

    // return n?FIFO_ERROR_UNDERRUN:FIFO_ERROR_NONE;
}

uint32_t FIFO::getSize() const { return size; }

void FIFO::setSize(uint32_t size) { FIFO::size = size; }
