#ifndef CLOCK_SYSTEM_H // Include guard to prevent multiple inclusion of this header file
#define CLOCK_SYSTEM_H

#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_pwr.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_system.h"

#include "stm32h7xx_ll_crs.h"

/******************************************************************************
  * @FunctionName : SystemClock_Init()
  * @Description  : This function configures the system clock and CRS settings.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void SystemClock_Init(void);

/******************************************************************************
  * @FunctionName : SystemTickConfig()
  * @Description  : This function configures SysTick frequency and interrupt state.
  * @note         :
  * @Param        : u16Frequency: SysTick tick frequency.
  * @Param        : eSysTickSate: SysTick interrupt enable state.
  * @Return       : None.
  ******************************************************************************/
void SystemTickConfig(uint32_t u16Frequency,FunctionalState eSysTickSate);

/******************************************************************************
  * @FunctionName : GetTick()
  * @Description  : This function returns the current millisecond tick value.
  * @note         :
  * @Param        : None.
  * @Return       : Current tick value in milliseconds.
  ******************************************************************************/
uint32_t GetTick(void);

extern volatile uint32_t TIME_1ms;

/******************************************************************************
  * @FunctionName : GPIO_Init()
  * @Description  : This function configures GPIO pins used by the application.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void GPIO_Init(void);

#endif /* CLOCK_SYSTEM_H */
