#include "Clock_system.h"


/******************************************************************************
  * @FunctionName : SystemClock_Init()
  * @Description  : This function configures the system clock and CRS settings.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void SystemClock_Init(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_2)
  {
  }
  LL_PWR_ConfigSupply(LL_PWR_SMPS_2V5_SUPPLIES_LDO);
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  while (LL_PWR_IsActiveFlag_VOS() == 0)
  {
  }
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_HSI48_Enable();

   /* Wait till HSI48 is ready */
  while(LL_RCC_HSI48_IsReady() != 1)
  {

  }
  LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_HSE);
  LL_RCC_PLL1P_Enable();
  LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_8_16);
  LL_RCC_PLL1_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
  LL_RCC_PLL1_SetM(2);
  LL_RCC_PLL1_SetN(64);
  LL_RCC_PLL1_SetP(2);
  LL_RCC_PLL1_SetQ(13);
  LL_RCC_PLL1_SetR(2);
  LL_RCC_PLL1_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL1_IsReady() != 1)
  {
  }

   /* Intermediate AHB prescaler 2 when target frequency clock is higher than 80 MHz */
   LL_RCC_SetAHBPrescaler(LL_RCC_AHB_DIV_2);

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1)
  {

  }
  LL_RCC_SetSysPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAHBPrescaler(LL_RCC_AHB_DIV_2);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
  LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_2);
  LL_RCC_SetAPB4Prescaler(LL_RCC_APB4_DIV_2);

  LL_SetSystemCoreClock(400000000);
	LL_Init1msTick(400000000);

   /* Update the time base */
//  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
//  {
//    Error_Handler();
//  }

  LL_CRS_SetSyncDivider(LL_CRS_SYNC_DIV_1);
  LL_CRS_SetSyncPolarity(LL_CRS_SYNC_POLARITY_RISING);
  LL_CRS_SetSyncSignalSource(LL_CRS_SYNC_SOURCE_USB);
  LL_CRS_SetReloadCounter(__LL_CRS_CALC_CALCULATE_RELOADVALUE(48000000,1000));
  LL_CRS_SetFreqErrorLimit(34);
  LL_CRS_SetHSI48SmoothTrimming(32);
}

/******************************************************************************
  * @FunctionName : SystemTickConfig()
  * @Description  : This function configures SysTick frequency and interrupt state.
  * @note         :
  * @Param        : u16Frequency: SysTick tick frequency.
  * @Param        : eSysTickSate: SysTick interrupt enable state.
  * @Return       : None.
  ******************************************************************************/
void SystemTickConfig(uint32_t u16Frequency,FunctionalState eSysTickSate){
	LL_InitTick(SystemCoreClock,u16Frequency);
	if(eSysTickSate == ENABLE)
	{
		NVIC_SetPriority(SysTick_IRQn, 1U);
		SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	}
}


volatile uint32_t TIME_1ms = 0;

/******************************************************************************
  * @FunctionName : HAL_IncTick()
  * @Description  : This function increments the millisecond tick counter.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void HAL_IncTick(void)
{
	TIME_1ms += 1;
}

/******************************************************************************
  * @FunctionName : HAL_GetTick()
  * @Description  : This function returns the current millisecond tick value.
  * @note         :
  * @Param        : None.
  * @Return       : Current tick value in milliseconds.
  ******************************************************************************/
uint32_t HAL_GetTick(void)
{
	return TIME_1ms;
}

/******************************************************************************
  * @FunctionName : SysTick_Handler()
  * @Description  : This function handles the SysTick interrupt.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void SysTick_Handler(void)
{
	HAL_IncTick();
}

/******************************************************************************
  * @FunctionName : GetTick()
  * @Description  : This function returns the current millisecond tick value.
  * @note         :
  * @Param        : None.
  * @Return       : Current tick value in milliseconds.
  ******************************************************************************/
uint32_t GetTick(void) {
    return TIME_1ms;
}

/******************************************************************************
  * @FunctionName : GPIO_Init()
  * @Description  : This function configures GPIO pins used by the application.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void GPIO_Init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOA);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_8;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /**/
  LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_1);

  /**/
  LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_3);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}
