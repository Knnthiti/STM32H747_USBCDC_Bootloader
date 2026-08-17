#include "main_Bootloader.h" // Include the main bootloader header file

// Global variable to track 1-millisecond time intervals
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

	HAL_Init(); // Initialize the Hardware Abstraction Layer (HAL)

	SystemClock_Init(); // Configure and initialize the system clock

	GPIO_Init(); // Initialize the general-purpose input/output (GPIO) pins

	// Initialize CRC calculation using DMA for the received USB CDC data
	CRC_DmaInit((uint32_t)RX_USBCDC_Data.u32RxUSBData, 255);

	GPIO_USB_Init(); // Initialize specific GPIO pins required for USB communication

	MX_USB_DEVICE_Init(); // Initialize the USB Device stack (CDC class)

	SystemTickConfig(1000, ENABLE); // Configure the System Tick timer for 1ms interrupts (1000 Hz)

	while (1)
	{ // Infinite main loop
		if (GetTick() - Part_time > 10)
		{						   // Check if more than 10 milliseconds have passed
			Part_time = GetTick(); // Reset the 1ms timer counter

			// Check if the input pin PA8 is set to High (e.g., button press or jumper)
			if (LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_8) == 1)
			{
				bootJumpToApp1(); // Jump execution to the main application in flash memory
			}
			else
			{
				PocessCommand(); // Process incoming bootloader commands via USB
			}
		}
	}
}
