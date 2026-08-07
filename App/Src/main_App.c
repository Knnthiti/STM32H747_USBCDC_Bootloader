#include "main_App.h"

volatile uint32_t TIME_1ms = 0;

int main(){
 SystemClock_Init(); // Configure and initialize the system clock

 GPIO_Init(); // Initialize the general-purpose input/output (GPIO) pins

 SystemTickConfig(1000,ENABLE); 
 
 while(1){
     if(TIME_1ms < 100){ // Check if more than 10 milliseconds have passed
        LL_GPIO_ResetOutputPin(GPIOA ,LL_GPIO_PIN_3);
				LL_GPIO_ResetOutputPin(GPIOB ,LL_GPIO_PIN_1);
     }else if(TIME_1ms < 200){
		    LL_GPIO_SetOutputPin(GPIOA ,LL_GPIO_PIN_3);
				LL_GPIO_SetOutputPin(GPIOB ,LL_GPIO_PIN_1);
		 }else{
		    TIME_1ms = 0; // Reset the 1ms timer counter
		 }
   } 

}

// System Tick timer interrupt handler

void SysTick_Handler(void) { 
    TIME_1ms += 1; // Increment the 1ms counter every time the interrupt triggers
} 
