/**
  ******************************************************************************
  * @file    BSP/CM7/Src/audio_analog.c
  * @author  MCD Application Team
  * @brief   This example describes how to use DFSDM HAL API to realize analog
  *           audio recording.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup BSP_Examples
  * @{
  */

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/* Audio frequency */
#define AUDIO_FREQUENCY            16000U
#define AUDIO_IN_BUFFER_SIZE  (uint32_t)(128*AUDIO_FREQUENCY/16000*2)
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Define record Buf at D3SRAM @0x30020000 since the BDMA for SAI4 use only this memory */
#if defined ( __CC_ARM )  /* !< ARM Compiler */
  ALIGN_32BYTES (uint16_t recordBuf[AUDIO_IN_BUFFER_SIZE]) __attribute__((section(".RAM_D2")));

#elif defined ( __ICCARM__ )  /* !< ICCARM Compiler */
  #pragma location=0x30020000
ALIGN_32BYTES (uint16_t recordBuf[AUDIO_IN_BUFFER_SIZE]);

#elif defined ( __GNUC__ )  /* !< GNU Compiler */
  ALIGN_32BYTES (uint16_t recordBuf[AUDIO_IN_BUFFER_SIZE]) __attribute__((section(".RAM_D2")));
#endif
static uint32_t AudioFreq[9] = {8000 ,11025, 16000, 22050, 32000, 44100, 48000, 96000, 192000};
uint32_t AnalogVolumeLevel = 75;
uint32_t *AudioFreq_Ptr;
BSP_AUDIO_Init_t  AnalogAudioInInit;
BSP_AUDIO_Init_t  AnalogAudioOutInit;
/* Private functions ---------------------------------------------------------*/

/**
  * @brief Test Analog Audio record.
  *   The main objective of this test is to check the hardware connection of the
  *   Audio peripheral.
  * @param  None
  * @retval None
*/
void AnalogAudioRecord_demo(void)
{
  uint32_t channel_nbr = 2;

  uint32_t x_size, y_size;

  BSP_LCD_GetXSize(0, &x_size);
  BSP_LCD_GetYSize(0, &y_size);

  /* Clear the LCD */
  UTIL_LCD_Clear(UTIL_LCD_COLOR_WHITE);
  /* Set Audio Demo description */
  UTIL_LCD_FillRect(0, 0, x_size, 90, UTIL_LCD_COLOR_BLUE);
  UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
  UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_BLUE);
  UTIL_LCD_SetFont(&Font24);
  UTIL_LCD_DisplayStringAt(0, 0, (uint8_t *)"AUDIO RECORD SAI ANALOG EXAMPLE", CENTER_MODE);
  UTIL_LCD_SetFont(&Font16);
  UTIL_LCD_DisplayStringAt(0, 24,  (uint8_t *)"Press User button for next menu", CENTER_MODE);
  /* Set the LCD Text Color */
  UTIL_LCD_DrawRect(10, 100, x_size - 20, y_size - 110, UTIL_LCD_COLOR_BLUE);
  UTIL_LCD_DrawRect(11, 101, x_size - 22, y_size - 112, UTIL_LCD_COLOR_BLUE);

  AudioFreq_Ptr = AudioFreq+2; /* AUDIO_FREQUENCY_16K; */

  AnalogAudioOutInit.Device = AUDIO_OUT_DEVICE_AUTO;
  AnalogAudioOutInit.ChannelsNbr = channel_nbr;
  AnalogAudioOutInit.SampleRate = *AudioFreq_Ptr;
  AnalogAudioOutInit.BitsPerSample = AUDIO_RESOLUTION_16B;
  AnalogAudioOutInit.Volume = AnalogVolumeLevel;

  AnalogAudioInInit.Device = AUDIO_IN_DEVICE_ANALOG_MIC;
  AnalogAudioInInit.ChannelsNbr = channel_nbr;
  AnalogAudioInInit.SampleRate = *AudioFreq_Ptr;
  AnalogAudioInInit.BitsPerSample = AUDIO_RESOLUTION_16B;
  AnalogAudioInInit.Volume = AnalogVolumeLevel;

  BSP_JOY_Init(JOY1, JOY_MODE_GPIO, JOY_ALL);

  /* Initialize Audio Recorder with 2 channels to be used */
  BSP_AUDIO_OUT_Init(0, &AnalogAudioOutInit);
  BSP_AUDIO_IN_Init(0, &AnalogAudioInInit);

  BSP_AUDIO_OUT_SetDevice(0, AUDIO_OUT_DEVICE_HEADPHONE);

  /* Start Recording */
  UTIL_LCD_DisplayStringAt(0, 190, (uint8_t *)"Start Recording ", CENTER_MODE);
  BSP_AUDIO_IN_Record(0, (uint8_t*)&recordBuf, 2*AUDIO_IN_BUFFER_SIZE);

  /* Play the recorded buffer*/
  UTIL_LCD_DisplayStringAt(0, 220, (uint8_t *)"Play the recorded buffer... ", CENTER_MODE);
  BSP_AUDIO_OUT_Play(0, (uint8_t*)&recordBuf[0], 2*AUDIO_IN_BUFFER_SIZE);
  while (1)
  {
    if (CheckForUserInput() > 0)
    {
      ButtonState = 0;
      BSP_AUDIO_OUT_Stop(0);
      BSP_AUDIO_OUT_DeInit(0);
      BSP_AUDIO_IN_Stop(0);
      BSP_AUDIO_IN_DeInit(0);
      return;
    }
  }
}

/**
  * @brief  Audio IN Error callback function
  * @param  None
  * @retval None
  */
__weak void BSP_AUDIO_IN_Error_CallBack(uint32_t Instance)
{
  /* Stop the program with an infinite loop */
  Error_Handler();
}
/**
  * @}
  */

