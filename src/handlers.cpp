//
// Created by Angel Dust on 06/03/2021.
//

#include "hw/stm32.h"
#include "handlers.h"
#include "hw/hw_config.h"
#include "status.h"

#include "printf.h"

#include <cstdint>

// TODO: Link hardware specific handlers in their respective /hw/* file

void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(FRONT_PANEL_INTERRUPT_PIN_A);
}

void EXTI4_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(ROT_BTN_PIN);
}

void EXTI9_5_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(TOUCH_IRQ_PIN);
    HAL_GPIO_EXTI_IRQHandler(BACK_BTN_PIN);
}

void EXTI15_10_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(ROT_A_PIN);
}

void NMI_Handler(void) {
}

// void HardFault_Handler(void) {
//
//     // TODO: Implement something like https://blog.feabhas.com/2013/02/developing-a-generic-hard-fault-handler-for-arm-cortex-m3cortex-m4/
//     printf("Hard Fault");
//     while (1) {
//
//     }
//
// }

namespace {

enum fault_id_t : uint32_t {
    FAULT_HARD = 1,
    FAULT_MEMMANAGE = 2,
    FAULT_BUS = 3,
    FAULT_USAGE = 4,
};

struct fault_context_t {
    // Stacked registers
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;

    // Fault status registers
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t dfsr;
    uint32_t afsr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t shcsr;

    // Stack pointers at time of handler
    uint32_t msp;
    uint32_t psp;

    // EXC_RETURN
    uint32_t exc_return;

    // Which fault handler
    uint32_t fault_id;
};

static volatile fault_context_t last_fault;

static const char *fault_name(uint32_t id) {
    switch (id) {
        case FAULT_HARD:
            return "HardFault";
        case FAULT_MEMMANAGE:
            return "MemManage";
        case FAULT_BUS:
            return "BusFault";
        case FAULT_USAGE:
            return "UsageFault";
        default:
            return "Fault";
    }
}

} // namespace

extern "C" void fault_handler_c(uint32_t *stacked, uint32_t exc_return, uint32_t fault_id) {

    // Capture stacked registers (Cortex-M exception stack frame).
    last_fault.r0 = stacked[0];
    last_fault.r1 = stacked[1];
    last_fault.r2 = stacked[2];
    last_fault.r3 = stacked[3];
    last_fault.r12 = stacked[4];
    last_fault.lr = stacked[5];
    last_fault.pc = stacked[6];
    last_fault.psr = stacked[7];

    // Capture system fault status.
    last_fault.cfsr = SCB->CFSR;
    last_fault.hfsr = SCB->HFSR;
    last_fault.dfsr = SCB->DFSR;
    last_fault.afsr = SCB->AFSR;
    last_fault.mmfar = SCB->MMFAR;
    last_fault.bfar = SCB->BFAR;
    last_fault.shcsr = SCB->SHCSR;

    last_fault.msp = __get_MSP();
    last_fault.psp = __get_PSP();
    last_fault.exc_return = exc_return;
    last_fault.fault_id = fault_id;

    // Print a compact dump. Keep this short to reduce risk of re-faulting.
    printf_("\n%s\n", fault_name(fault_id));
    printf_(" PC=0x%08lx LR=0x%08lx PSR=0x%08lx\n", (unsigned long)last_fault.pc, (unsigned long)last_fault.lr, (unsigned long)last_fault.psr);
    printf_(" R0=0x%08lx R1=0x%08lx R2=0x%08lx R3=0x%08lx R12=0x%08lx\n", (unsigned long)last_fault.r0, (unsigned long)last_fault.r1,
            (unsigned long)last_fault.r2, (unsigned long)last_fault.r3, (unsigned long)last_fault.r12);
    printf_(" CFSR=0x%08lx HFSR=0x%08lx DFSR=0x%08lx AFSR=0x%08lx\n", (unsigned long)last_fault.cfsr, (unsigned long)last_fault.hfsr,
            (unsigned long)last_fault.dfsr, (unsigned long)last_fault.afsr);
    printf_(" MMFAR=0x%08lx BFAR=0x%08lx SHCSR=0x%08lx\n", (unsigned long)last_fault.mmfar, (unsigned long)last_fault.bfar, (unsigned long)last_fault.shcsr);
    extern uint32_t _lstack;
    extern char _end;

    printf_(" MSP=0x%08lx PSP=0x%08lx EXC_RETURN=0x%08lx\n", (unsigned long)last_fault.msp, (unsigned long)last_fault.psp,
            (unsigned long)last_fault.exc_return);
    printf_(" stack_low=0x%08lx heap_start=0x%08lx\n", (unsigned long)&_lstack, (unsigned long)&_end);

    while (1) {
    }
}

// Naked wrappers to safely extract the active stack pointer (MSP/PSP).
__attribute__((naked)) void HardFault_Handler(void) {
    __asm volatile("tst lr, #4\n"
                   "ite eq\n"
                   "mrseq r0, msp\n"
                   "mrsne r0, psp\n"
                   "mov r1, lr\n"
                   "movs r2, #1\n"
                   "b fault_handler_c\n");
}

__attribute__((naked)) void MemManage_Handler(void) {
    __asm volatile("tst lr, #4\n"
                   "ite eq\n"
                   "mrseq r0, msp\n"
                   "mrsne r0, psp\n"
                   "mov r1, lr\n"
                   "movs r2, #2\n"
                   "b fault_handler_c\n");
}

__attribute__((naked)) void BusFault_Handler(void) {
    __asm volatile("tst lr, #4\n"
                   "ite eq\n"
                   "mrseq r0, msp\n"
                   "mrsne r0, psp\n"
                   "mov r1, lr\n"
                   "movs r2, #3\n"
                   "b fault_handler_c\n");
}

__attribute__((naked)) void UsageFault_Handler(void) {
    __asm volatile("tst lr, #4\n"
                   "ite eq\n"
                   "mrseq r0, msp\n"
                   "mrsne r0, psp\n"
                   "mov r1, lr\n"
                   "movs r2, #4\n"
                   "b fault_handler_c\n");
}

void FPU_IRQHandler(void) {
    while (1) {
    }
}

void SVC_Handler(void) {
}

void DebugMon_Handler(void) {
    printf("DebugMon_Handler");
}

void PendSV_Handler(void) {
    printf("PendSV_Handler");
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void) {
    /* USER CODE BEGIN SysTick_IRQn 0 */

    /* USER CODE END SysTick_IRQn 0 */
    uwTick += uwTickFreq;
    // HAL_IncTick();
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

    status::pop_alert(status::ERROR, "Fatal error");

    /* USER CODE END Error_Handler_Debug */
}
