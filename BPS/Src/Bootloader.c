#include "Bootloader.h"

volatile uint8_t __attribute__((section(".bss.Version_Program"))) Version_Edit;

uint32_t u32BufferProgram[size_u32BufferProgram];
volatile uint16_t current_program = 0;

//void bootJumpToApp1(){

//	typedef void (*pFunction)(void);
//	pFunction vJumpToApp;
//	uint32_t appStack;
//	uint32_t appEntry;

//	appStack = *(__IO uint32_t*)FLASH_START_APP1;
//	appEntry = *(__IO uint32_t*)(FLASH_START_APP1 + 4);
//	vJumpToApp = (pFunction)appEntry;

//	__disable_irq();

//	SysTick->CTRL = 0;
//	SysTick->LOAD = 0;
//	SysTick->VAL = 0;
//	SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;

//	for(uint32_t i = 0; i < (sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0])); i++){
//		NVIC->ICER[i] = 0xFFFFFFFF;
//		NVIC->ICPR[i] = 0xFFFFFFFF;
//	}

//	SCB->VTOR = FLASH_START_APP1;
//	__set_MSP(appStack);
//	__DSB();
//	__ISB();
//	__enable_irq();

//	vJumpToApp();
//}

void bootJumpToApp1(){ 
	
	typedef int (*pFunction)(void);
	static pFunction vJumpToApp;
	
	vJumpToApp	= (pFunction)(*(__IO uint32_t*)(FLASH_START_APP1 + 4));
	__set_MSP(*(__IO uint32_t*)FLASH_START_APP1);
	
	__disable_irq();
	
	vJumpToApp();
}

uint8_t IsFlash_WaitForOperation(void){
	return((FLASH->SR1 & FLASH_SR_BSY) ? 1 : 0);
}

void Clear_errorflags(void){
	FLASH->CCR1 = (FLASH_CCR_CLR_EOP | FLASH_CCR_CLR_WRPERR | FLASH_CCR_CLR_PGSERR 
	               | FLASH_CCR_CLR_STRBERR | FLASH_CCR_CLR_INCERR | FLASH_CCR_CLR_OPERR 
	               | FLASH_CCR_CLR_RDPERR | FLASH_CCR_CLR_RDSERR | FLASH_CCR_CLR_SNECCERR 
	               | FLASH_CCR_CLR_DBECCERR | FLASH_CCR_CLR_CRCEND | FLASH_CCR_CLR_CRCRDERR);
}

void FLASH_access_control(void){
	FLASH->ACR = FLASH_ACR_LATENCY_5WS;
	FLASH->ACR |= FLASH_ACR_WRHIGHFREQ;
	
	Clear_errorflags();
	Flash_lock();
}

void Flash_Unlock(void){
  FLASH->KEYR1 = 0x45670123;
  FLASH->KEYR1 = 0xCDEF89AB;
}

void Flash_lock(void){
  FLASH->CR1 |= FLASH_CR_LOCK;
}

uint8_t IsFlash_lock(void){
	return(FLASH->CR1 & FLASH_CR_LOCK) ? 1 : 0;
}

uint8_t Flash_erase_Sector(uint8_t u8Sector){
	while(IsFlash_WaitForOperation()) {
		return 1;
	}
	
	//1.Clear all the error flags
	Clear_errorflags();
	
	//2.Unlock Flash
	if(IsFlash_lock()){
        Flash_Unlock();
    }
	
	//3.Set SER1 for choose erase sector mode.
	FLASH->CR1 |= FLASH_CR_SER;
	
	//Clear sector
	FLASH->CR1 &= ~FLASH_CR_SNB_Msk;
	//4.Set SNB1 for target sector.
	FLASH->CR1 |= u8Sector << FLASH_CR_SNB_Pos;
	
	//5.Set START1
	FLASH->CR1 |= FLASH_CR_START;
	
	//Wait to finish erase
	while(IsFlash_WaitForOperation()) {
		return 1;
	}
	
	//clear CR->SER for Out erase sector mode.
	FLASH->CR1 &= ~FLASH_CR_SER;
	//clear EOP
	FLASH->CCR1 |= FLASH_CCR_CLR_EOP;
	
	return 0;
}

void Erase_All_App(void){
	for(uint8_t i = 0x01 ; i < 0x08 ; i++){
		Flash_erase_Sector(i);
	}
}

BufferFlash _32byteBufferFlash;

uint8_t Flash_Write_B1(uint32_t u32FlashAddress, uint32_t *u32Data32B, uint16_t u16DataCount){
	while(IsFlash_WaitForOperation()) {
		return 1;
	}
	
	//1.Clear all the error flags
	Clear_errorflags();
	
	//2.Unlock Flash
	if(IsFlash_lock()){
        Flash_Unlock();
    }		
		
	//3.Set SER1 for choose Write mode.
	FLASH->CR1 |= FLASH_CR_PG;
	
	//Address in Blank 1
	u32FlashAddress = u32FlashAddress & 0x080FFFFF;
	
	//Clear Buffer
	for(uint8_t i = 0; i < 8 ; i++){
		_32byteBufferFlash.u32Buffer[i] = 0xFFFFFFFF;
	}
	
	uint16_t u16WriteCount = (u16DataCount -1) >> 5;
	u16WriteCount++;
	
	while(u16WriteCount--){
		//Copy data to Buffer 256 bit
		if(u16DataCount > 8){
			for(uint8_t i = 0 ; i < 8 ; i++){
				_32byteBufferFlash.u32Buffer[i] = *(u32Data32B++);
			}
			u16DataCount -= 8;
		}else{
			for(uint8_t i = 0 ; i < u16DataCount ; i++){
				_32byteBufferFlash.u32Buffer[i] = *(u32Data32B++);
			}
			u16DataCount = 0;
		}
		
		*(uint32_t *)u32FlashAddress = _32byteBufferFlash.u32Buffer[0];
		__ISB();// Using instruction barier to make sure that be write a flash with the correct order.
		*(uint32_t *)(u32FlashAddress + 4U) = _32byteBufferFlash.u32Buffer[1];
		__ISB();// Using instruction barier to make sure that be write a flash with the correct order.
		*(uint32_t *)(u32FlashAddress + 8U) = _32byteBufferFlash.u32Buffer[2];
		__ISB();// Using instruction barier to make sure that be write a flash with the correct order.
		*(uint32_t *)(u32FlashAddress + 12U) = _32byteBufferFlash.u32Buffer[3];
		__ISB();// Using instruction barier to make sure that be write a flash with the correct order.
		*(uint32_t *)(u32FlashAddress + 16U) = _32byteBufferFlash.u32Buffer[4];
		__ISB();// Using instruction barier to make sure that be write a flash with the correct order.
		*(uint32_t *)(u32FlashAddress + 20U) = _32byteBufferFlash.u32Buffer[5];
		__ISB();// Using instruction barier to make sure that be write a flash with the correct order.
		*(uint32_t *)(u32FlashAddress + 24U) = _32byteBufferFlash.u32Buffer[6];
		__ISB();// Using instruction barier to make sure that be write a flash with the correct order.
		*(uint32_t *)(u32FlashAddress + 28U) = _32byteBufferFlash.u32Buffer[7];
		
		u32FlashAddress += 32;
	}
	
	while(IsFlash_WaitForOperation()) {
		return 1;
	}
	
	//clear CR->PG for Out Write mode.
	FLASH->CR1 &= ~FLASH_CR_PG;
	//clear EOP
	FLASH->CCR1 |= FLASH_CCR_CLR_EOP;
	
	return 0;
}

volatile _USBData TX_USBCDC_Data;

uint32_t u32offset_FlashAddress = 0;

void PocessCommand(void)
{
    switch (RX_USBCDC_Data.u8setting1Byte.u8herder)
    {
        case PC_CMD_START: // 0x07
			
		    __disable_irq();
        	Erase_All_App();
	        __enable_irq();	
		
            current_program = 0;
            u32offset_FlashAddress = 0;
            memset((uint32_t*)RX_USBCDC_Data.u32RxUSBData, 0x00, u32APP_RX_DATA_SIZE);
           
            TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_READY; 
            CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
            break;

        case PC_CMD_SENDING: // 0x67
            if (current_program < size_u32BufferProgram) {
                TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_READY; // 0x49
                CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
				
            } else {
                TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_PAUSE; // 0x69
                CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
            }
            break;

        case PC_CMD_WAIT: // 0x20
			__disable_irq();
		    Flash_Write_B1((FLASH_START_APP1 + u32offset_FlashAddress) ,u32BufferProgram, size_u32BufferProgram);
		    __enable_irq();
		
            u32offset_FlashAddress += MaximumAddress_BufferFlash;
            current_program = 0;
            
            TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_READY; // 0x49
            CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
            break;

        case PC_CMD_FINISHED: // 0x99
			__disable_irq();
		    Flash_Write_B1((FLASH_START_APP1 + u32offset_FlashAddress) ,u32BufferProgram, size_u32BufferProgram);
		    __enable_irq();
		
            TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_FINISHED; // 0x55
            CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
            
		    Version_Edit = RX_USBCDC_Data.u8setting1Byte.u8version;
            current_program = 0;
            u32offset_FlashAddress = 0; 
            memset((uint32_t*)RX_USBCDC_Data.u32RxUSBData, 0x00, u32APP_RX_DATA_SIZE);
            break;

        default:
            
            break;
    }
    
    RX_USBCDC_Data.u8setting1Byte.u8herder = 0x00;
}
