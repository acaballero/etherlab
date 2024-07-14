//
// Created by Angel Dust on 20/05/2021.
//

#ifndef TRX_FRONTEND_CLOCKS_H
#define TRX_FRONTEND_CLOCKS_H

#ifdef __cplusplus
extern "C" {
#endif

void HAL_MspInit(void);


#ifdef __cplusplus
}
#endif

void SystemClock_Config(void);

#endif //TRX_FRONTEND_CLOCKS_H
