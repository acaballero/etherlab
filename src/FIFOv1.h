//
// Created by Angel Dust on 09/04/2021.
//

#ifndef TRX_FRONTEND_FIFOV1_H
#define TRX_FRONTEND_FIFOV1_H

#include "hw/stm32.h"
#include "FIFO.h"
/*
enum FIFO_ERROR {
    FIFO_ERROR_NONE,FIFO_ERROR_OVERRUN,FIFO_ERROR_UNDERRUN
};
*/

class FIFOv1 {

public:

    FIFOv1(char *buffer, uint16_t size) : data(buffer), size(size) { // +1 since this implementation sacrifices one byte
        this->read_ix=this->write_ix=0;
    }

    FIFO_ERROR write(char *origin,uint16_t size);
    FIFO_ERROR  writeBlock(char *origin, uint16_t n);
    FIFO_ERROR feed(uint16_t size);
    FIFO_ERROR consume(uint16_t size, char **dest);
    void reset();
    uint16_t available(char **dest);
    uint16_t free(char **start);
    uint16_t free();
    uint16_t available();


protected:


    char *data;
    uint16_t size;
    uint16_t read_ix,write_ix;

};


#endif //TRX_FRONTEND_FIFOV1_H
