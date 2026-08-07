#ifndef CLOCK_SYSTEM_H // Include guard to prevent multiple inclusion of this header file
#define CLOCK_SYSTEM_H

#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_pwr.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_system.h"

void SystemClock_Init(void);
void SystemTickConfig(uint32_t u16Frequency,FunctionalState eSysTickSate);

void GPIO_Init(void);

#endif /* CLOCK_SYSTEM_H */