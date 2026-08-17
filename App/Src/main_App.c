#include "main_App.h"

/******************************************************************************
  * @FunctionName : main()
  * @Description  : This function initializes the application and toggles GPIO outputs.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
int main(){
 SystemClock_Init(); // Configure and initialize the system clock

 GPIO_Init(); // Initialize the general-purpose input/output (GPIO) pins

 SystemTickConfig(1000,ENABLE);
 __enable_irq();

 while(1){
     if(GetTick() < 100){
        LL_GPIO_ResetOutputPin(GPIOA ,LL_GPIO_PIN_3);
				LL_GPIO_ResetOutputPin(GPIOB ,LL_GPIO_PIN_1);
     }else if(GetTick() < 200){
		    LL_GPIO_SetOutputPin(GPIOA ,LL_GPIO_PIN_3);
				LL_GPIO_SetOutputPin(GPIOB ,LL_GPIO_PIN_1);
		 }else{
		    TIME_1ms = 0;
		 }
   }

}
