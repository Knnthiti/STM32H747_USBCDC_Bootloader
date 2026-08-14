#include "main_BootloaderV2.h"


uint32_t Part_Time = 0;
uint32_t Time_tigger = 0;

  int main(){ 
	  
	HAL_Init();
	  
    SystemClock_Init();

	GPIO_Init();
	  
	GPIO_USB_Init();
	MX_USB_DEVICE_Init();
	
	USART6_Init();
	  
	SystemTickConfig(1000,ENABLE);
	  
    while(1){
//		if((GetTick() - Part_Time) > 10){
//			Part_Time = GetTick();
			
			if(LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_8) == 1){
			   bootJumpToApp1();
	        }else{
			   PocessCommand();
		    }
//		}
		
//		if((GetTick() - Time_tigger) > 1000){
//			Time_tigger = GetTick();
			
//			if(pri_currentState == PRI_STATE_IDLE || pri_currentState == PRI_STATE_ERROR){
//                pri_currentState = PRI_STATE_START_TX;
//            }
//		}
    }	   
}
