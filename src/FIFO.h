//
// Created by Angel Dust on 09/04/2021.
//

#ifndef TRX_FRONTEND_FIFO_H
#define TRX_FRONTEND_FIFO_H

#include <stdint.h>

enum FIFO_ERROR { FIFO_ERROR_NONE, FIFO_ERROR_OVERRUN, FIFO_ERROR_UNDERRUN };

/*
 * This implementation counts the number of elements, sacrificing some speed, to
 * be able to use all the space available
 */
class FIFO {

  public:
    FIFO(char *buffer, uint32_t size) : data(buffer), size(size) {
        this->read_ix = this->write_ix = 0;
    }

    FIFO_ERROR write(char *origin, uint32_t size);
    FIFO_ERROR write_block(char *origin, uint32_t n);
    FIFO_ERROR feed(uint32_t size);
    FIFO_ERROR consume(uint32_t size, char **dest);
    void reset();
    uint32_t available(char **dest);
    uint32_t free(char **start);
    uint32_t free();
    uint32_t available();

    uint32_t get_size() const;

    void set_size(uint32_t size);
    bool is_closed() {
        return closed;
    }
    void close() {
        closed = true;
    }

    char *data_p() {
        return data;
    }

  protected:
    char *data;
    uint32_t size;
    uint32_t count;
    uint32_t read_ix, write_ix;
    // The stream is closed, so consumers know there won't be any more data
    volatile bool closed = false;

  private:
    inline void feed_unsafe(uint32_t n);
};

#endif // TRX_FRONTEND_FIFO_H
