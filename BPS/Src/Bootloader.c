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
	
	typedef int (*pFunction)(void);
	static pFunction vJumpToApp;
	
	// Load MSP and reset handler from the application vector table.
	vJumpToApp	= (pFunction)(*(__IO uint32_t*)(FLASH_START_APP1 + 4));
	__set_MSP(*(__IO uint32_t*)FLASH_START_APP1);
	
	__disable_irq();
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
	FLASH->ACR = FLASH_ACR_LATENCY_5WS;
	FLASH->ACR |= FLASH_ACR_WRHIGHFREQ;
	
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
	while(IsFlash_WaitForOperation()) {
		return 1;
	}
	
	Clear_errorflags();
	
	if(IsFlash_lock()){
        Flash_Unlock();
    }
	
	// Select the target sector and start a bank 1 sector erase.
	FLASH->CR1 |= FLASH_CR_SER;
	FLASH->CR1 &= ~FLASH_CR_SNB_Msk;
	FLASH->CR1 |= u8Sector << FLASH_CR_SNB_Pos;
	FLASH->CR1 |= FLASH_CR_START;
	
	while(IsFlash_WaitForOperation()) {
		return 1;
	}
	
	FLASH->CR1 &= ~FLASH_CR_SER;
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
	while(IsFlash_WaitForOperation()) {
		return 1;
	}
	
	Clear_errorflags();
	
	if(IsFlash_lock()){
        Flash_Unlock();
    }		
		
	FLASH->CR1 |= FLASH_CR_PG;
	
	u32FlashAddress = u32FlashAddress & 0x080FFFFF;
	
	for(uint8_t i = 0; i < 8 ; i++){
		_32byteBufferFlash.u32Buffer[i] = 0xFFFFFFFF;
	}
	
	uint16_t u16WriteCount = (u16DataCount -1) >> 5;
	u16WriteCount++;
	
	// STM32H7 Flash is programmed in 32-byte blocks.
	while(u16WriteCount--){
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
		
		// Use barriers to keep the 256-bit Flash write order stable.
		*(uint32_t *)u32FlashAddress = _32byteBufferFlash.u32Buffer[0];
		__ISB();
		*(uint32_t *)(u32FlashAddress + 4U) = _32byteBufferFlash.u32Buffer[1];
		__ISB();
		*(uint32_t *)(u32FlashAddress + 8U) = _32byteBufferFlash.u32Buffer[2];
		__ISB();
		*(uint32_t *)(u32FlashAddress + 12U) = _32byteBufferFlash.u32Buffer[3];
		__ISB();
		*(uint32_t *)(u32FlashAddress + 16U) = _32byteBufferFlash.u32Buffer[4];
		__ISB();
		*(uint32_t *)(u32FlashAddress + 20U) = _32byteBufferFlash.u32Buffer[5];
		__ISB();
		*(uint32_t *)(u32FlashAddress + 24U) = _32byteBufferFlash.u32Buffer[6];
		__ISB();
		*(uint32_t *)(u32FlashAddress + 28U) = _32byteBufferFlash.u32Buffer[7];
		
		u32FlashAddress += 32;
	}
	
	while(IsFlash_WaitForOperation()) {
		return 1;
	}
	
	FLASH->CR1 &= ~FLASH_CR_PG;
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
    switch (RX_USBCDC_Data.u8setting1Byte.u8herder)
    {
        case PC_CMD_START: // 0x07
			
			// Erase old application firmware before receiving the new image.
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
				// Pause the PC so the bootloader can flush RAM data to Flash.
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
