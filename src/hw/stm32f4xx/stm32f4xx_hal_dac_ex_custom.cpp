//
// Created by Angel Dust on 16/06/2021.
//

#include "stm32f4xx_hal_dac_ex_custom.h"
#include "stdio.h"
#include "stm32f4xx.h"

/*
 * Custom functions, derived from the ones in HAL, enabling the use of the two DAC channel in simultaneous mode.
 */



/**
  * @brief  DMA conversion complete callback.
  * @param  hdma pointer to a DMA_HandleTypeDef structure that contains
  *                the configuration information for the specified DMA module.
  * @retval None
  */
void DAC_DMAConvCpltCh1(DMA_HandleTypeDef *hdma)
{
    DAC_HandleTypeDef* hdac = ( DAC_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;

#if (USE_HAL_DAC_REGISTER_CALLBACKS == 1)
    hdac->ConvCpltCallbackCh1(hdac);
#else
    HAL_DAC_ConvCpltCallbackCh1(hdac);
#endif /* USE_HAL_DAC_REGISTER_CALLBACKS */

    hdac->State= HAL_DAC_STATE_READY;
}

/**
  * @brief  DMA half transfer complete callback.
  * @param  hdma pointer to a DMA_HandleTypeDef structure that contains
  *                the configuration information for the specified DMA module.
  * @retval None
  */
void DAC_DMAHalfConvCpltCh1(DMA_HandleTypeDef *hdma)
{
    DAC_HandleTypeDef* hdac = ( DAC_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;
    /* Conversion complete callback */
#if (USE_HAL_DAC_REGISTER_CALLBACKS == 1)
    hdac->ConvHalfCpltCallbackCh1(hdac);
#else
    HAL_DAC_ConvHalfCpltCallbackCh1(hdac);
#endif  /* USE_HAL_DAC_REGISTER_CALLBACKS */
}

/**
  * @brief  DMA error callback
  * @param  hdma pointer to a DMA_HandleTypeDef structure that contains
  *                the configuration information for the specified DMA module.
  * @retval None
  */
void DAC_DMAErrorCh1(DMA_HandleTypeDef *hdma)
{
    DAC_HandleTypeDef* hdac = ( DAC_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;

    /* Set DAC error code to DMA error */
    hdac->ErrorCode |= HAL_DAC_ERROR_DMA;

#if (USE_HAL_DAC_REGISTER_CALLBACKS == 1)
    hdac->ErrorCallbackCh1(hdac);
#else
    HAL_DAC_ErrorCallbackCh1(hdac);
#endif /* USE_HAL_DAC_REGISTER_CALLBACKS */

    hdac->State= HAL_DAC_STATE_READY;
}

HAL_StatusTypeDef HAL_DAC_Start_DualDMA(DAC_HandleTypeDef *hdac, uint32_t Channel, uint32_t *pData, uint32_t Length, uint32_t Alignment) {

    uint32_t tmpreg = 0;

    /* Check the parameters */
    assert_param(IS_DAC_CHANNEL_INSTANCE(hdac->Instance, Channel));
    assert_param(IS_DAC_ALIGN(Alignment));

    /* Process locked */
    __HAL_LOCK(hdac);

    /* Change DAC state */
    hdac->State = HAL_DAC_STATE_BUSY;

    if (Channel == DAC_CHANNEL_1) {

        /* Set the DMA transfer complete callback for channel1 */
        hdac->DMA_Handle1->XferCpltCallback = DAC_DMAConvCpltCh1;

        /* Set the DMA half transfer complete callback for channel1 */
        hdac->DMA_Handle1->XferHalfCpltCallback = DAC_DMAHalfConvCpltCh1;

        /* Set the DMA error callback for channel1 */
        hdac->DMA_Handle1->XferErrorCallback = DAC_DMAErrorCh1;

        /* Enable the selected DAC channel1 DMA request */
        SET_BIT(hdac->Instance->CR, DAC_CR_DMAEN1);

        /* Case of use of channel 1 */
        switch (Alignment) {
            case DAC_ALIGN_12B_R:
                /* Get DHR12R1 address */
                tmpreg = (uint32_t) &hdac->Instance->DHR12R1;
                break;
            case DAC_ALIGN_12B_L:
                /* Get DHR12L1 address */
                tmpreg = (uint32_t) &hdac->Instance->DHR12L1;
                break;
            case DAC_ALIGN_8B_R:
                /* Get DHR8R1 address */
                tmpreg = (uint32_t) &hdac->Instance->DHR8R1;
                break;
            default:
                break;
        }
    } else if (Channel == DAC_CHANNEL_2) {

        /* Set the DMA transfer complete callback for channel2 */
        hdac->DMA_Handle2->XferCpltCallback = DAC_DMAConvCpltCh2;
        /* Set the DMA half transfer complete callback for channel2 */
        hdac->DMA_Handle2->XferHalfCpltCallback = DAC_DMAHalfConvCpltCh2;
        /* Set the DMA error callback for channel2 */
        hdac->DMA_Handle2->XferErrorCallback = DAC_DMAErrorCh2;
        /* Enable the selected DAC channel2 DMA request */
        SET_BIT(hdac->Instance->CR, DAC_CR_DMAEN2);

        /* Case of use of channel 2 */
        switch (Alignment) {
            case DAC_ALIGN_12B_R:
                /* Get DHR12R2 address */
                tmpreg = (uint32_t) &hdac->Instance->DHR12R2;
                break;
            case DAC_ALIGN_12B_L:
                /* Get DHR12L2 address */
                tmpreg = (uint32_t) &hdac->Instance->DHR12L2;
                break;
            case DAC_ALIGN_8B_R:
                /* Get DHR8R2 address */
                tmpreg = (uint32_t) &hdac->Instance->DHR8R2;
                break;
            default:
                break;
        }
    } else { /* Dual-channel mode - added by JG @ Det3 */
        /* Set the DMA transfer complete callback for channel1 */
        hdac->DMA_Handle1->XferCpltCallback = DAC_DMAConvCpltCh1;

        /* Set the DMA half transfer complete callback for channel1 */
        hdac->DMA_Handle1->XferHalfCpltCallback = DAC_DMAHalfConvCpltCh1;

        /* Set the DMA error callback for channel1 */
        hdac->DMA_Handle1->XferErrorCallback = DAC_DMAErrorCh1;

        /* Enable the selected DAC channel1 DMA request */
        SET_BIT(hdac->Instance->CR, DAC_CR_DMAEN1);
        /* Case of use of channel 1+2 - dual mode */
        switch (Alignment) {
            case DAC_ALIGN_12B_R:
                /* Get DHR12RD address */
                tmpreg = (uint32_t) &hdac->Instance->DHR12RD;
                break;
            case DAC_ALIGN_12B_L:
                /* Get DHR12LD address */
                tmpreg = (uint32_t) &hdac->Instance->DHR12LD;
                break;
            case DAC_ALIGN_8B_R:
                /* Get DHR8RD address */
                tmpreg = (uint32_t) &hdac->Instance->DHR8RD;
                break;
            default:
                break;
        }
    }

    /* Enable the DMA Channel */
    /* Reversed for dual-channel operation by JG @ Det3 */

    if (Channel == DAC_CHANNEL_2) {
        /* Enable the DAC DMA underrun interrupt */
        __HAL_DAC_ENABLE_IT(hdac, DAC_IT_DMAUDR2);

        /* Enable the DMA Channel */
        HAL_DMA_Start_IT(hdac->DMA_Handle2, (uint32_t) pData, tmpreg, Length);
    } else {
        /* Enable the DAC DMA underrun interrupt */
        __HAL_DAC_ENABLE_IT(hdac, DAC_IT_DMAUDR1);
        /* Enable the DMA Channel */
        HAL_DMA_Start_IT(hdac->DMA_Handle1, (uint32_t) pData, tmpreg, Length);
    }
    /* Process Unlocked */
    __HAL_UNLOCK(hdac);
    /* Enable the Peripheral */
    if (Channel == DAC_CHANNEL_12D) {
        __HAL_DAC_ENABLE(hdac, DAC_CHANNEL_1);
        __HAL_DAC_ENABLE(hdac, DAC_CHANNEL_2);
    } else {
        __HAL_DAC_ENABLE(hdac, Channel);
    }
    /* Return function status */
    return HAL_OK;
}