//
// Created by Angel Dust on 06/03/2021.
//

#include "hw/stm32.h"
#include "handlers.h"
#include "hw/hw_config.h"
#include "status.h"

// TODO: Link hardware specific handlers in their respective /hw/* file

void EXTI0_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(FRONT_PANEL_INTERRUPT_PIN_A); }

void EXTI4_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(ROT_BTN_PIN); }

void EXTI9_5_IRQHandler(void) {
   HAL_GPIO_EXTI_IRQHandler(TOUCH_IRQ_PIN);
   HAL_GPIO_EXTI_IRQHandler(BACK_BTN_PIN);
}

void EXTI15_10_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(ROT_A_PIN); }

void NMI_Handler(void) {}

// void HardFault_Handler(void) {
//
//     // TODO: Implement something like https://blog.feabhas.com/2013/02/developing-a-generic-hard-fault-handler-for-arm-cortex-m3cortex-m4/
//     printf("Hard Fault");
//     while (1) {
//
//     }
//
// }

void FPU_IRQHandler(void) {
   while (1) {
   }
}

void MemManage_Handler(void) {
   printf("MemManage_Handler");
   while (1) {
   }
}

void BusFault_Handler(void) {
   printf("BusFault_Handler");
   while (1) {
   }
}

void UsageFault_Handler(void) {
   while (1) {
   }
}

void SVC_Handler(void) {}

void DebugMon_Handler(void) { printf("DebugMon_Handler"); }

void PendSV_Handler(void) { printf("PendSV_Handler"); }

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void) {
   /* USER CODE BEGIN SysTick_IRQn 0 */

   /* USER CODE END SysTick_IRQn 0 */
   HAL_IncTick();
   /* USER CODE BEGIN SysTick_IRQn 1 */

   /* USER CODE END SysTick_IRQn 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
   /* USER CODE BEGIN Error_Handler_Debug */
   /* User can add his own implementation to report the HAL error return state */

   status::handleError(status::ST_ERROR, "Fatal error");

   /* USER CODE END Error_Handler_Debug */
}

void HardFault_Handler(void) {

   // TODO: Implement something like https://blog.feabhas.com/2013/02/developing-a-generic-hard-fault-handler-for-arm-cortex-m3cortex-m4/
   printf("Hard Fault");
   while (1) {
   }
}
