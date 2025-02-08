//
// Created by Angel Dust on 09/04/2021.
//

#include "FIFOv1.h"
#include <cstring> // memcpy
#include "hw/stm32.h"

#define FIFO_INCR_IX(ix, n)                                                                                                                                    \
    {                                                                                                                                                          \
        ix++;                                                                                                                                                  \
        if (ix == n)                                                                                                                                           \
            ix = 0;                                                                                                                                            \
    }

void FIFOv1::reset() { this->read_ix = this->write_ix = 0; }

uint16_t FIFOv1::available() {

    if (this->read_ix <= this->write_ix) {
        return this->write_ix - this->read_ix;
    } else {
        return this->size - (this->read_ix - this->write_ix);
    }
};

uint16_t FIFOv1::available(char **dest) {
    *dest = this->data + this->read_ix;
    return this->available();
};

uint16_t FIFOv1::free() {
    return (this->size - this->available()) - 1; // remember we sacrifice one byte to ease full/empty checks
};

uint16_t FIFOv1::free(char **start) {
    *start = this->data + this->write_ix;
    return this->free();
};

FIFO_ERROR FIFOv1::write(char *origin, uint16_t n) {

    uint16_t next_ix = this->write_ix;
    FIFO_INCR_IX(next_ix, this->size);

    while (next_ix != this->read_ix && n) {

        this->data[this->write_ix] = *(origin++);
        this->write_ix = next_ix;
        FIFO_INCR_IX(next_ix, this->size);
        n--;
    }

    return n ? FIFO_ERROR_OVERRUN : FIFO_ERROR_NONE;
}

/*
 * Writes a block
 * TODO: Caution. This function assumes the block is a multiple of the FIFO size
 */
FIFO_ERROR FIFOv1::writeBlock(char *origin, uint16_t n) {

    char *p;

    uint32_t free = this->free(&p);

    if (free >= n) {
        memcpy(p, origin, n);
        this->feed(n);
        return FIFO_ERROR_NONE;
    } else {
        return FIFO_ERROR_OVERRUN;
    }
};

FIFO_ERROR FIFOv1::feed(uint16_t n) {

    uint16_t free = this->free();
    if (free >= n) {

        this->write_ix += n;
        if (this->write_ix >= this->size) {
            this->write_ix = this->write_ix - this->size;
        }

        return FIFO_ERROR_NONE;
    } else {

        return FIFO_ERROR_OVERRUN;
    }
}

/*
 * Returns a pointer to the read position
 */
FIFO_ERROR FIFOv1::consume(uint16_t n, char **dest) {

    uint16_t av = this->available();

    if (av < n) {
        return FIFO_ERROR_UNDERRUN;
    } else {
        *dest = this->data + this->read_ix;

        this->read_ix += n;
        if (this->read_ix >= this->size) {
            this->read_ix = this->read_ix - this->size;
        }

        return FIFO_ERROR_NONE;
    }

    //    while (this->read_ix!=this->write_ix && n) {
    //        FIFO_INCR_IX(this->read_ix,this->size);
    //        n--;
    //    }

    // return n?FIFO_ERROR_UNDERRUN:FIFO_ERROR_NONE;
}
