#include "main_Bootloader.h"

uint32_t Part_Time = 0;


 int main(){
    SystemClock_Init();
	SystemTickConfig(1000,ENABLE);
	 
	CRC_DmaInit((uint32_t)RX_USART_Data.u32Data, 255);
	 
	UART5_UART_Init();

	Button_PC13_Init();
   while(1){
//	  if ((GetTick() - Part_Time) > 10) {
//			Part_Time = GetTick();
//            if (LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_13) == 1) {
//		       bootJumpToApp1();
//            }
//	  }
	  UART_ProcessState();
   }	   
}
