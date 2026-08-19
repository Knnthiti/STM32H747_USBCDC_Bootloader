#include "Bootloader.h"

volatile uint8_t __attribute__((section(".bss.Version_Program"))) Version_Edit;

uint32_t u32BufferProgram[size_u32BufferProgram];
volatile uint16_t current_program = 0;

/******************************************************************************
  * @FunctionName : bootJumpToApp1()
  * @Description  : This function jumps from bootloader to application 1.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void bootJumpToApp1(){ 
	
	// Define a function pointer type for the application reset handler.
	typedef int (*pFunction)(void);
	static pFunction vJumpToApp;
	
	// Read the reset handler address from the application vector table.
	vJumpToApp	= (pFunction)(*(__IO uint32_t*)(FLASH_START_APP1 + 4));

	// Load the application stack pointer before jumping to application code.
	__set_MSP(*(__IO uint32_t*)FLASH_START_APP1);
	
	// Disable interrupts so bootloader interrupts do not run inside the application.
	__disable_irq();
	
	// Branch to the application reset handler.
	vJumpToApp();
}

/******************************************************************************
  * @FunctionName : IsFlash_WaitForOperation()
  * @Description  : This function checks whether Flash operation is still busy.
  * @note         :
  * @Param        : None.
  * @Return       : 1 when Flash is busy, otherwise 0.
  ******************************************************************************/
uint8_t IsFlash_WaitForOperation(void){
	// Return the current Flash busy state from the Flash status register.
	return((FLASH->SR1 & FLASH_SR_BSY) ? 1 : 0);
}

/******************************************************************************
  * @FunctionName : Clear_errorflags()
  * @Description  : This function clears Flash error and status flags.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void Clear_errorflags(void){
	// Write clear bits to CCR1 to reset all Flash operation and error flags.
	FLASH->CCR1 = (FLASH_CCR_CLR_EOP | FLASH_CCR_CLR_WRPERR | FLASH_CCR_CLR_PGSERR 
	               | FLASH_CCR_CLR_STRBERR | FLASH_CCR_CLR_INCERR | FLASH_CCR_CLR_OPERR 
	               | FLASH_CCR_CLR_RDPERR | FLASH_CCR_CLR_RDSERR | FLASH_CCR_CLR_SNECCERR 
	               | FLASH_CCR_CLR_DBECCERR | FLASH_CCR_CLR_CRCEND | FLASH_CCR_CLR_CRCRDERR);
}

/******************************************************************************
  * @FunctionName : FLASH_access_control()
  * @Description  : This function configures Flash access latency and locks Flash.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void FLASH_access_control(void){
	// Set Flash wait states for the configured high-speed system clock.
	FLASH->ACR = FLASH_ACR_LATENCY_5WS;

	// Enable high-frequency Flash write configuration.
	FLASH->ACR |= FLASH_ACR_WRHIGHFREQ;
	
	// Start from a clean Flash status and keep Flash locked until needed.
	Clear_errorflags();
	Flash_lock();
}

/******************************************************************************
  * @FunctionName : Flash_Unlock()
  * @Description  : This function unlocks Flash bank 1 control register.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void Flash_Unlock(void){
  // Send the two-key unlock sequence required by STM32 Flash bank 1.
  FLASH->KEYR1 = 0x45670123;
  FLASH->KEYR1 = 0xCDEF89AB;
}

/******************************************************************************
  * @FunctionName : Flash_lock()
  * @Description  : This function locks Flash bank 1 control register.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void Flash_lock(void){
  // Set the LOCK bit to protect Flash bank 1 control register.
  FLASH->CR1 |= FLASH_CR_LOCK;
}

/******************************************************************************
  * @FunctionName : IsFlash_lock()
  * @Description  : This function checks whether Flash bank 1 is locked.
  * @note         :
  * @Param        : None.
  * @Return       : 1 when Flash is locked, otherwise 0.
  ******************************************************************************/
uint8_t IsFlash_lock(void){
	// Read the Flash lock bit and convert it to a 1/0 result.
	return(FLASH->CR1 & FLASH_CR_LOCK) ? 1 : 0;
}

/******************************************************************************
  * @FunctionName : Flash_erase_Sector()
  * @Description  : This function erases one Flash sector in bank 1.
  * @note         :
  * @Param        : u8Sector: Flash sector number to erase.
  * @Return       : 0 when erase is started and completed, otherwise 1.
  ******************************************************************************/
uint8_t Flash_erase_Sector(uint8_t u8Sector){
	// Do not start a new erase operation while Flash is busy.
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

/******************************************************************************
  * @FunctionName : Erase_All_App()
  * @Description  : This function erases all application Flash sectors.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void Erase_All_App(void){
	// Erase all sectors reserved for the application firmware.
	for(uint8_t i = 0x01 ; i < 0x08 ; i++){
		Flash_erase_Sector(i);
	}
}

BufferFlash _32byteBufferFlash;

/******************************************************************************
  * @FunctionName : Flash_Write_B1()
  * @Description  : This function writes 32-bit data words to Flash bank 1.
  * @note         :
  * @Param        : u32FlashAddress: Destination Flash address.
  * @Param        : u32Data32B: Pointer to source data buffer.
  * @Param        : u16DataCount: Number of 32-bit words to write.
  * @Return       : 0 when write is completed, otherwise 1.
  ******************************************************************************/
uint8_t Flash_Write_B1(uint32_t u32FlashAddress, uint32_t *u32Data32B, uint16_t u16DataCount){
	// Do not start a new program operation while Flash is busy.
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
	
	// Program Flash in 32-byte blocks as required by STM32H7 Flash programming.
	while(u16WriteCount--){
		//Copy data to Buffer 256 bit
		if(u16DataCount > 8){
			// Fill one complete 32-byte Flash programming buffer.
			for(uint8_t i = 0 ; i < 8 ; i++){
				_32byteBufferFlash.u32Buffer[i] = *(u32Data32B++);
			}
			u16DataCount -= 8;
		}else{
			// Copy the remaining words and leave unused buffer words as 0xFFFFFFFF.
			for(uint8_t i = 0 ; i < u16DataCount ; i++){
				_32byteBufferFlash.u32Buffer[i] = *(u32Data32B++);
			}
			u16DataCount = 0;
		}
		
		// Write the prepared 256-bit Flash word using eight 32-bit accesses.
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
	
	// Wait until the last Flash programming operation is complete.
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

/******************************************************************************
  * @FunctionName : PocessCommand()
  * @Description  : This function processes USB CDC bootloader commands.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void PocessCommand(void)
{
	// Decode the command byte stored in the received USB CDC packet header.
    switch (RX_USBCDC_Data.u8setting1Byte.u8herder)
    {
        case PC_CMD_START: // 0x07
			
			// Erase the old application firmware before receiving the new image.
		    __disable_irq();
        	Erase_All_App();
	        __enable_irq();	
		
			// Reset write indexes and clear the RX packet buffer.
            current_program = 0;
            u32offset_FlashAddress = 0;
            memset((uint32_t*)RX_USBCDC_Data.u32RxUSBData, 0x00, u32APP_RX_DATA_SIZE);
           
			// Tell the PC that the bootloader is ready for firmware packets.
            TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_READY; 
            CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
            break;

        case PC_CMD_SENDING: // 0x67
			// Continue receiving packets until the firmware RAM buffer is full.
            if (current_program < size_u32BufferProgram) {
                TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_READY; // 0x49
                CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
				
            } else {
				// Ask the PC to pause so the bootloader can write buffered data to Flash.
                TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_PAUSE; // 0x69
                CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
            }
            break;

        case PC_CMD_WAIT: // 0x20
			// Write the current RAM buffer to the next application Flash address.
			__disable_irq();
		    Flash_Write_B1((FLASH_START_APP1 + u32offset_FlashAddress) ,u32BufferProgram, size_u32BufferProgram);
		    __enable_irq();
		
			// Advance the Flash offset and reset the RAM buffer write index.
            u32offset_FlashAddress += MaximumAddress_BufferFlash;
            current_program = 0;
            
			// Notify the PC that Flash writing is complete and transfer can continue.
            TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_READY; // 0x49
            CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
            break;

        case PC_CMD_FINISHED: // 0x99
			// Write the final buffered firmware block to Flash.
			__disable_irq();
		    Flash_Write_B1((FLASH_START_APP1 + u32offset_FlashAddress) ,u32BufferProgram, size_u32BufferProgram);
		    __enable_irq();
		
			// Send the final success ACK to the PC.
            TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_FINISHED; // 0x55
            CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
            
			// Store the received firmware version and reset update state.
		    Version_Edit = RX_USBCDC_Data.u8setting1Byte.u8version;
            current_program = 0;
            u32offset_FlashAddress = 0; 
            memset((uint32_t*)RX_USBCDC_Data.u32RxUSBData, 0x00, u32APP_RX_DATA_SIZE);
            break;

        default:
            
            break;
    }
    
	// Clear the command byte so the same command is not processed again.
    RX_USBCDC_Data.u8setting1Byte.u8herder = 0x00;
}
