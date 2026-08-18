#include "main_Bootloader.h"

volatile uint32_t Part_time = 0;

/******************************************************************************
  * @FunctionName : main()
  * @Description  : This function initializes the bootloader and handles USB commands.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
int main()
{

	HAL_Init();

	SystemClock_Init();

	GPIO_Init();

	// CRC is calculated over the first 255 words of each 1024-byte packet.
	CRC_DmaInit((uint32_t)RX_USBCDC_Data.u32RxUSBData, 255);

	GPIO_USB_Init();

	MX_USB_DEVICE_Init();

	SystemTickConfig(1000, ENABLE);

	while (1)
	{
		if (GetTick() - Part_time > 10)
		{
			Part_time = GetTick();

			if (LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_8) == 1)
			{
				bootJumpToApp1();
			}
			else
			{
				PocessCommand();
			}
		}
	}
}
