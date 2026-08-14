#include "Bootloader.h"

void bootJumpToApp1(){ 
	
	typedef int (*pFunction)(void);
	static pFunction vJumpToApp;
	
	vJumpToApp	= (pFunction)(*(__IO uint32_t*)(FLASH_START_APP1 + 4));
	__set_MSP(*(__IO uint32_t*)FLASH_START_APP1);
	
	__disable_irq();
	
	vJumpToApp();
}

uint32_t u32CrcCalculateLength;

void CRC_DmaInit(uint32_t u32MemAddr, uint32_t u32memLength) {
    
    /* 1. ???? Clock ????????????????????? */ 
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMAMUX1); 

    /* 2. ??????? CRC Hardware */
    LL_CRC_ResetCRCCalculationUnit(CRC);
    LL_CRC_SetPolynomialSize(CRC, LL_CRC_POLYLENGTH_32B);
    LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_NONE);
    LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_NONE);
    LL_CRC_SetInitialData(CRC, 0xFFFFFFFF);
    LL_CRC_SetPolynomialCoef(CRC, 0x04C11DB7); // ????????????????????????????????????????? CRC32
    
    /* 3. ??? DMA ??????????????????????????? */
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);

    /* 4. ?????????????? DMA */
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_1, LL_DMA_DIRECTION_MEMORY_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PRIORITY_HIGH);
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MODE_NORMAL);
    
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PERIPH_INCREMENT); // ???? RAM ????????? Address
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MEMORY_NOINCREMENT); // ???? CRC Register ?????????????
    
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PDATAALIGN_WORD);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MDATAALIGN_WORD);
    
    /* 5. ???? Address ??? Length ?????????????????????? */
    LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_1, 
                           u32MemAddr,             // Source: RAM
                           (uint32_t)&(CRC->DR),   // Destination: Hardware CRC
                           LL_DMA_DIRECTION_MEMORY_TO_MEMORY);
    
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, u32memLength);
 
    /* 6. ????????????????????????????? */
    LL_DMA_ClearFlag_TC1(DMA1);

    /* 7. ??????? DMA ?????????! (???????) */
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);

    /* 8. ?????????? DMA ????????????????? (Polling) */
    while(LL_DMA_IsActiveFlag_TC1(DMA1) == 0) {
        // ???????????...
    }

    /* 9. ??????????????? DMA */
    LL_DMA_ClearFlag_TC1(DMA1);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
}

uint32_t software_crc32(uint32_t *data, uint16_t length) {
    uint32_t crc = 0xFFFFFFFF;

    for (uint16_t i = 0; i < length; i++) {
		
		crc ^= data[i];

        for (uint8_t j = 0; j < 32; j++) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ 0x04C11DB7;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

volatile uint32_t My_CRC = 0;
volatile uint32_t My_CRC_SW = 0;
volatile uint32_t Received_CRC = 67;

void CRC_APP_RX_DATA(uint32_t *buffer, uint32_t len) {
//    LL_CRC_ResetCRCCalculationUnit(CRC);

//    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);
//    
//    while(LL_DMA_IsActiveFlag_TC1(DMA1) == 0) {
//    }
//    
//    LL_DMA_ClearFlag_TC1(DMA1);
//	
//    My_CRC = LL_CRC_ReadData32(CRC);
//	
//    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
// 
    My_CRC_SW = software_crc32(buffer, len);

    Received_CRC = RX_USART_Data.u32CRC4Byte; 
}

uint8_t IsFlash_WaitForOperation(void){
	return((FLASH->SR & FLASH_SR_BSY) ? 1 : 0);
}

void Clear_errorflags(void){
	FLASH->SR = (FLASH_SR_FASTERR 	| \
				FLASH_SR_PGSERR 	| \
				FLASH_SR_SIZERR 	| \
				FLASH_SR_PGAERR 	| \
				FLASH_SR_WRPERR 	| \
				FLASH_SR_PROGERR 	| \
				FLASH_SR_OPERR);
}

void Flash_Unlock(void){
  FLASH->KEYR = 0x45670123;
  FLASH->KEYR = 0xCDEF89AB;
}

void Flash_lock(void){
  FLASH->CR |= FLASH_CR_LOCK;
}

uint8_t IsFlash_lock(void){
	return(FLASH->CR & FLASH_CR_LOCK) ? 1 : 0;
}


uint8_t B1_Flash_erase_Page(uint8_t Page){
	
	//EOP Interrupt enabled
	FLASH->CR |= FLASH_CR_EOPIE;
	
	while(IsFlash_WaitForOperation()) {
		return 1;
	}
	
	//1.Clear all the error flags
	Clear_errorflags();
	
	//2.Unlock Flash
	if(IsFlash_lock()){
        Flash_Unlock();
    }
	
	//Set BKER for choose Blank1.
	FLASH->CR &= ~FLASH_CR_BKER_Msk;
	
	//Set PER for choose erase Page mode.
	FLASH->CR |= FLASH_CR_PER;
	
	//Clear sector
	FLASH->CR &= ~FLASH_CR_PNB_Msk;
	//4.Set PNB for target sector.
	FLASH->CR |= Page << FLASH_CR_PNB_Pos;
	
	//5.Set START1
	FLASH->CR |= FLASH_CR_STRT;
	
	//Wait to finish erase
	while(IsFlash_WaitForOperation()) {
		return 1;
	}
	
	//clear CR->SER for Out erase sector mode.
	FLASH->CR &= ~FLASH_CR_PER_Msk;
	//clear EOP
	FLASH->SR |= FLASH_SR_EOP_Msk; 	 
	return 0;
}

void B1_Erase_All_App(void){
	for(uint8_t i = 0x04 ; i < 0xFF ; i++){
		B1_Flash_erase_Page(i);
	}
}

BufferFlash _32byteBufferFlash;

uint8_t B1_Flash_Write(uint32_t u32FlashAddress, uint32_t *u32Data32B, uint16_t u16DataCount){
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
	FLASH->CR |= FLASH_CR_PG;
	
	//Address in Blank 1
	u32FlashAddress = u32FlashAddress & 0x0803FFFF;
	
	//Clear Buffer
	for(uint8_t i = 0; i < 8 ; i++){
		_32byteBufferFlash.u32Buffer[i] = 0xFFFFFFFF;
	}
	
	uint16_t u16WriteCount = (u16DataCount -1) >> 3;
	u16WriteCount++;
	
	while(u16WriteCount--){
		for(uint8_t i = 0; i < 8 ; i++){
			_32byteBufferFlash.u32Buffer[i] = 0xFFFFFFFF;
		}
		
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
	FLASH->CR &= ~FLASH_CR_PG_Msk;
	//clear EOP
	FLASH->SR |= FLASH_SR_EOP_Msk; 
	
	return 0;
}

uint32_t u32BufferProgram[size_u32BufferProgram];

volatile _USARTData TX_USART_Data;
volatile _USARTData RX_USART_Data;

volatile uint8_t __attribute__((section(".bss.Version_Program"))) Version_Edit;

uint32_t u32offset_FlashAddress = 0;
volatile uint16_t current_program;

uint32_t timeoutStart = 0;
volatile uint16_t g4_rxIndex = 0;
volatile uint16_t g4_txIndex = 0;
volatile G4_State_t g4_currentState = G4_STATE_INIT_RX;

//void UART_ProcessState(void) {
//    switch (g4_currentState) {

//        /* --- STATE: Initialize RX --- */
//        case G4_STATE_INIT_RX:
//            g4_rxIndex = 0; // Reset receive index
//            LL_USART_EnableIT_RXNE(UART5); // Make sure RX interrupt is on
//            timeoutStart = GetTick();
//            g4_currentState = G4_STATE_WAIT_RX;
//            break;

//        /* --- STATE: Wait for RX Completion or Timeout --- */
//        case G4_STATE_WAIT_RX:
//            // Data reception and state transition to PROCESS_DATA is handled in UART5_IRQHandler
//            
//            // Check for Timeout
//            if ((GetTick() - timeoutStart) > USART_TIMEOUT_MS) {
//                if (g4_rxIndex == 0) { 
//                    // No data arrived yet, reset timeout
//                    timeoutStart = GetTick();
//                } else { 
//                    // Partial data arrived but hung, trigger timeout reset
//                    g4_currentState = G4_STATE_TIMEOUT_RESET;
//                }
//            }
//            break;

//        /* --- STATE: Process Received Data --- */
//        case G4_STATE_PROCESS_DATA:
//            // Validate incoming header
//            if (RX_USART_Data.u8setting1Byte.u8herder == PC_CMD_START_PRI    || 
//                RX_USART_Data.u8setting1Byte.u8herder == PC_CMD_SENDING_PRI  || 
//                RX_USART_Data.u8setting1Byte.u8herder == PC_CMD_FINISHED_PRI || 
//                RX_USART_Data.u8setting1Byte.u8herder == PC_CMD_WAIT_PRI) 
//           {
//                // Prepare ACK response packet
////                TX_USART_Data.u8setting1Byte.u8herder = STM_ACK_READY_PRI;
//				
//				PocessCommand();
//                
//                g4_currentState = G4_STATE_START_TX;
//            }else{
//                g4_currentState = G4_STATE_TIMEOUT_RESET; // Invalid header, restart RX
//            }
//            break;

//        /* --- STATE: Initialize TX --- */
//        case G4_STATE_START_TX:
//            g4_txIndex = 0; // Reset transmit index
//            timeoutStart = GetTick();
//            
//            // Trigger transmission by enabling TXE Interrupt
//            LL_USART_EnableIT_TXE(UART5); 
//            g4_currentState = G4_STATE_WAIT_TX;
//            break;

//        /* --- STATE: Wait for TX Completion or Timeout --- */
//        case G4_STATE_WAIT_TX:
//            // Data transmission and state transition is handled in UART5_IRQHandler (TC flag)
//            
//            // Check for Timeout during transmission
//            if ((GetTick() - timeoutStart) > USART_TIMEOUT_MS) {
//                g4_currentState = G4_STATE_TIMEOUT_RESET;
//            }
//            break;

//        /* --- STATE: Handle Timeout / Errors --- */
//        case G4_STATE_TIMEOUT_RESET:
//            // Disable transmission interrupts to safely reset
//            LL_USART_DisableIT_TXE(UART5);
//            LL_USART_DisableIT_TC(UART5);
//            g4_currentState = G4_STATE_INIT_RX;
//            break;

//        default:
//            g4_currentState = G4_STATE_INIT_RX;
//            break;
//    }
//}

//// ========================================================
//// UART5 Interrupt Handler
//// ========================================================
//void UART5_IRQHandler(void){
//    
//    // 1. Check if data is received (RXNE)
//    if (LL_USART_IsActiveFlag_RXNE(UART5) && LL_USART_IsEnabledIT_RXNE(UART5)) {
////        if ((g4_currentState == G4_STATE_WAIT_RX) && (g4_rxIndex < size_u8USARTdata)) {
//		if (g4_rxIndex < size_u8USARTdata) {
//                RX_USART_Data.u8Data[g4_rxIndex++] = LL_USART_ReceiveData8(UART5);        
//        }
//	}

//    // 2. Check for End of Packet (IDLE Line)
//    if (LL_USART_IsActiveFlag_IDLE(UART5) && LL_USART_IsEnabledIT_IDLE(UART5)) {
//        LL_USART_ClearFlag_IDLE(UART5); // Clear the IDLE flag to avoid infinite loop
//        
//        if (g4_currentState == G4_STATE_WAIT_RX && g4_rxIndex >= size_u8USARTdata) {
//            LL_USART_DisableIT_RXNE(UART5); // Disable RX interrupt temporarily
//			
//			CRC_APP_RX_DATA((uint32_t*)RX_USART_Data.u32Data, 255);
//			
//			if((My_CRC_SW == Received_CRC) && (RX_USART_Data.u8setting1Byte.u8herder == PC_CMD_SENDING_PRI)){
//		    for(uint16_t i = 0 ; i < 254 ; i++){
//			    u32BufferProgram[current_program + i] = RX_USART_Data.u32Data[ 1 + i ];
//		    }	  
//		    current_program += 254;

//	        }
//			
//            g4_currentState = G4_STATE_PROCESS_DATA; // Move to processing state
//        }
//    }

//    // 3. Check if Transmit Register is Empty (TXE)
//    if (LL_USART_IsActiveFlag_TXE(UART5) && LL_USART_IsEnabledIT_TXE(UART5)) {
//        if (g4_txIndex < size_u8USARTdata) {
//            // Transmit the next byte
//            LL_USART_TransmitData8(UART5, TX_USART_Data.u8Data[g4_txIndex++]);
//        } else {
//            // All bytes written to TDR, stop TXE and wait for complete transmission (TC)
//            LL_USART_DisableIT_TXE(UART5);
//            LL_USART_EnableIT_TC(UART5);
//        }
//    }

//    // 4. Check if Transmission is Complete (TC)
//    if (LL_USART_IsActiveFlag_TC(UART5) && LL_USART_IsEnabledIT_TC(UART5)) {
//        LL_USART_ClearFlag_TC(UART5);
//        LL_USART_DisableIT_TC(UART5); // Turn off TC Interrupt

//        // Return to receiving mode
//        if (g4_currentState == G4_STATE_WAIT_TX) {
//            g4_currentState = G4_STATE_INIT_RX; 
//        }
//    }
//    
//    // 5. Handle Error Flags cleanly
//    if (LL_USART_IsActiveFlag_ORE(UART5)) LL_USART_ClearFlag_ORE(UART5);
//    if (LL_USART_IsActiveFlag_FE(UART5))  LL_USART_ClearFlag_FE(UART5);
//    if (LL_USART_IsActiveFlag_NE(UART5))  LL_USART_ClearFlag_NE(UART5);
//}

//void PocessCommand(void)
//{
//    switch (RX_USART_Data.u8setting1Byte.u8herder)
//    {
//        case PC_CMD_START_PRI: // 0x08
//			
////		    __disable_irq();
////        	B1_Erase_All_App();
////	        __enable_irq();	
//		
//            current_program = 0;
//            u32offset_FlashAddress = 0;
//            RX_USART_Data = (_USARTData){ 0x00 };
//			TX_USART_Data = (_USARTData){ 0x00 };
//           
//            TX_USART_Data.u8setting1Byte.u8herder = STM_ACK_READY_PRI; //0x4A
////            CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
//			
//            break;

//        case PC_CMD_SENDING_PRI: // 0x68
//            if (current_program < size_u32BufferProgram) {
//				TX_USART_Data = (_USARTData){ 0x00 };
//                TX_USART_Data.u8setting1Byte.u8herder = STM_ACK_READY_PRI; // 0x4A
////                CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
//				
//            } else {
//				TX_USART_Data = (_USARTData){ 0x00 };
//                TX_USART_Data.u8setting1Byte.u8herder = STM_ACK_PAUSE_PRI; // 0x6A
////                CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
//            }
//            break;

//        case PC_CMD_WAIT_PRI: // 0x21
////			__disable_irq();
////		    B1_Flash_Write((FLASH_START_APP1 + u32offset_FlashAddress) ,u32BufferProgram, size_u32BufferProgram);
////		    __enable_irq();
//		
//            u32offset_FlashAddress += MaximumAddress_BufferFlash;
//            current_program = 0;
//            
//		    TX_USART_Data = (_USARTData){ 0x00 };
//            TX_USART_Data.u8setting1Byte.u8herder = STM_ACK_READY_PRI; // 0x4A
////            CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
//            break;

//        case PC_CMD_FINISHED_PRI: // 0x9A
////			__disable_irq();
////		    B1_Flash_Write((FLASH_START_APP1 + u32offset_FlashAddress) ,u32BufferProgram, size_u32BufferProgram);
////		    __enable_irq();
//		
//		    TX_USART_Data = (_USARTData){ 0x00 };
//            TX_USART_Data.u8setting1Byte.u8herder = STM_ACK_FINISHED_PRI; // 0x56
////            CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
//            
//		    Version_Edit = RX_USART_Data.u8setting1Byte.u8version;
//            current_program = 0;
//            u32offset_FlashAddress = 0; 
//            RX_USART_Data = (_USARTData){ 0x00 };
//            break;

//        default:
//            
//            break;
//    }
//    
////    RX_USART_Data.u8setting1Byte.u8herder = 0x00;
//}

//void UART5_IRQHandler(void){
//    
//    // 1. ????????? (RXNE)
//    if (LL_USART_IsActiveFlag_RXNE(UART5) && LL_USART_IsEnabledIT_RXNE(UART5)) {
//        if (g4_rxIndex < size_u8USARTdata) {
//            RX_USART_Data.u8Data[g4_rxIndex++] = LL_USART_ReceiveData8(UART5);        
//        }
//        
//        // ??????????????????? (???? 1024 ????) ?????????? State ?????
//        if (g4_rxIndex >= size_u8USARTdata) {
//            LL_USART_DisableIT_RXNE(UART5); 
//            g4_currentState = G4_STATE_PROCESS_DATA; // ???????? Main Loop ??????
//        }
//    }
//	
//	if (LL_USART_IsActiveFlag_IDLE(UART5)) {
//        LL_USART_ClearFlag_IDLE(UART5);
//    }

//    // 2. ????????? (TXE)
//    if (LL_USART_IsActiveFlag_TXE(UART5) && LL_USART_IsEnabledIT_TXE(UART5)) {
//        if (g4_txIndex < size_u8USARTdata) {
//            LL_USART_TransmitData8(UART5, TX_USART_Data.u8Data[g4_txIndex++]);
//        } else {
//            LL_USART_DisableIT_TXE(UART5);
//            LL_USART_EnableIT_TC(UART5); // ???????????????
//        }
//    }

//    // 3. ?????????????? (TC)
//    if (LL_USART_IsActiveFlag_TC(UART5) && LL_USART_IsEnabledIT_TC(UART5)) {
//        LL_USART_ClearFlag_TC(UART5);
//        LL_USART_DisableIT_TC(UART5); 

//        if (g4_currentState == G4_STATE_WAIT_TX) {
//            g4_currentState = G4_STATE_INIT_RX; // ??????????????????????
//        }
//    }
//    
//    // 4. ??????? Error Flags
//    if (LL_USART_IsActiveFlag_ORE(UART5)) LL_USART_ClearFlag_ORE(UART5);
//    if (LL_USART_IsActiveFlag_FE(UART5))  LL_USART_ClearFlag_FE(UART5);
//    if (LL_USART_IsActiveFlag_NE(UART5))  LL_USART_ClearFlag_NE(UART5);
//}

void UART5_IRQHandler(void){
    
    // 1. ???????????????? (RXFNE / RXNE)
    // ??????? LL_USART_EnableIT_RXFNE ???????????????? Trigger ??????
    if (LL_USART_IsActiveFlag_RXNE(UART5) && LL_USART_IsEnabledIT_RXNE(UART5)) {
        
        // [????????] ?????????????????????????????????????? ???????? Hardware ???????????? RXFNE ??????
        uint8_t received_byte = LL_USART_ReceiveData8(UART5);

        if (g4_rxIndex < size_u8USARTdata) {
            RX_USART_Data.u8Data[g4_rxIndex++] = received_byte;        
        }
        
        // ????????? 1024 ???? ?????? Interrupt ?????????? State
        if (g4_rxIndex >= size_u8USARTdata) {
            LL_USART_DisableIT_RXNE(UART5); // ??? RXFNE/RXNE
            g4_currentState = G4_STATE_PROCESS_DATA; 
        }
    }

    // 2. ??????????????????? (IDLE)
    // ??????????????????????????? ???????????????????????????????
    if (LL_USART_IsActiveFlag_IDLE(UART5) && LL_USART_IsEnabledIT_IDLE(UART5)) {
        LL_USART_ClearFlag_IDLE(UART5); 
    }

    // 3. ??????????????? (TXFNF / TXE)
    // ??????? LL_USART_EnableIT_TXFNF ???????????????? Trigger ?????????????????????? FIFO
    if (LL_USART_IsActiveFlag_TXE(UART5) && LL_USART_IsEnabledIT_TXE(UART5)) {
        if (g4_txIndex < size_u8USARTdata) {
            // ???????????? TDR (?????????????????????????? TXFNF ????????????)
            LL_USART_TransmitData8(UART5, TX_USART_Data.u8Data[g4_txIndex++]);
        } else {
            // ??????????????? ?????????????? TXFNF ??????? TC ????????????????????????????????????????
            LL_USART_DisableIT_TXE(UART5); // ??? TXFNF/TXE
            LL_USART_EnableIT_TC(UART5); 
        }
    }

    // 4. ????????????????????????????????????? (TC)
    if (LL_USART_IsActiveFlag_TC(UART5) && LL_USART_IsEnabledIT_TC(UART5)) {
        LL_USART_ClearFlag_TC(UART5); // ????????? TC
        LL_USART_DisableIT_TC(UART5); 

        if (g4_currentState == G4_STATE_WAIT_TX) {
            g4_currentState = G4_STATE_INIT_RX; // ??????????????????????
        }
    }
    
    // 5. ?????? Error Flags 
    // ?????????????????? LL_USART_EnableIT_ERROR(UART5) ??????????????????????????????????
    if (LL_USART_IsActiveFlag_ORE(UART5)) { LL_USART_ClearFlag_ORE(UART5); }
    if (LL_USART_IsActiveFlag_FE(UART5))  { LL_USART_ClearFlag_FE(UART5); }
    if (LL_USART_IsActiveFlag_NE(UART5))  { LL_USART_ClearFlag_NE(UART5); }
    if (LL_USART_IsActiveFlag_PE(UART5))  { LL_USART_ClearFlag_PE(UART5); }
}

void UART_ProcessState(void) {
    switch (g4_currentState) {

        case G4_STATE_INIT_RX:
            g4_rxIndex = 0;
            g4_txIndex = 0;
            TX_USART_Data = (_USARTData){ 0x00 }; // ??????? Array ?????????????
            RX_USART_Data = (_USARTData){ 0x00 };
            
            LL_USART_EnableIT_RXNE(UART5); 
            timeoutStart = GetTick();
            g4_currentState = G4_STATE_WAIT_RX;
            break;

        case G4_STATE_WAIT_RX:
            if ((GetTick() - timeoutStart) > USART_TIMEOUT_MS) {
                if (g4_rxIndex == 0) { 
                    timeoutStart = GetTick(); // ???????????? ??????????????
                } else { 
                    g4_currentState = G4_STATE_TIMEOUT_RESET; // ?????????????? ????? ???????? ??? Reset
                }
            }
            break;

        case G4_STATE_PROCESS_DATA:
            // ??????????????????? ???? CRC ??????????? Flash Buffer ???????????
            PocessCommand();
                
            // ????? PocessCommand ?????????? ???????? Header ???????????? TX_USART_Data
            if (TX_USART_Data.u8setting1Byte.u8herder != 0x00) {
                g4_currentState = G4_STATE_START_TX;
            } else {
                g4_currentState = G4_STATE_TIMEOUT_RESET; // ???? CRC ??? ???? Header ??????
            }
			
//			PocessCommand();
            break;

        case G4_STATE_START_TX:
            g4_txIndex = 0; 
            timeoutStart = GetTick();
            LL_USART_EnableIT_TXE(UART5); 
            g4_currentState = G4_STATE_WAIT_TX;
            break;

        case G4_STATE_WAIT_TX:
            if ((GetTick() - timeoutStart) > USART_TIMEOUT_MS) {
                g4_currentState = G4_STATE_TIMEOUT_RESET;
            }
            break;

        case G4_STATE_TIMEOUT_RESET:
            LL_USART_DisableIT_TXE(UART5);
            LL_USART_DisableIT_TC(UART5);
            LL_USART_DisableIT_RXNE(UART5);
            TX_USART_Data = (_USARTData){ 0x00 };
            RX_USART_Data = (_USARTData){ 0x00 };
            g4_currentState = G4_STATE_INIT_RX;
            break;

        default:
            g4_currentState = G4_STATE_INIT_RX;
            break;
    }
}

static void USART_PrepareAck(uint8_t header)
{
    TX_USART_Data = (_USARTData){ 0x00 };
    TX_USART_Data.u8setting1Byte.u8herder = header;
}

static void USART_StorePayload(void)
{
    for(uint16_t i = 0 ; i < 254 ; i++){
        u32BufferProgram[current_program + i] = RX_USART_Data.u32Data[1 + i];
    }

    current_program += 254;
}

static void USART_WritePendingBuffer(void)
{
    if (current_program == 0) {
        return;
    }

    __disable_irq();
    B1_Flash_Write((FLASH_START_APP1 + u32offset_FlashAddress), u32BufferProgram, current_program);
    __enable_irq();

    u32offset_FlashAddress += ((uint32_t)current_program * 4U);
    current_program = 0;
}

void PocessCommand(void)
{
    CRC_APP_RX_DATA((uint32_t*)RX_USART_Data.u32Data, 255);

    if (My_CRC_SW != Received_CRC) {
        USART_PrepareAck(STM_ACK_PAUSE_PRI);
        return;
    }

    switch (RX_USART_Data.u8setting1Byte.u8herder)
    {
        case PC_CMD_START_PRI: // 0x08
            __disable_irq();
            B1_Erase_All_App();
            __enable_irq();

            current_program = 0;
            u32offset_FlashAddress = 0;
            USART_PrepareAck(STM_ACK_READY_PRI); // 0x4A
            break;

        case PC_CMD_SENDING_PRI: // 0x68
            if ((current_program + 254) <= size_u32BufferProgram) {
                USART_StorePayload();
            }

            USART_PrepareAck((current_program < size_u32BufferProgram) ? STM_ACK_READY_PRI : STM_ACK_PAUSE_PRI);
            break;

        case PC_CMD_WAIT_PRI: // 0x21
            USART_WritePendingBuffer();
            USART_PrepareAck(STM_ACK_READY_PRI); // 0x4A
            break;

        case PC_CMD_FINISHED_PRI: // 0x9A
            USART_WritePendingBuffer();
            Version_Edit = RX_USART_Data.u8setting1Byte.u8version;
            current_program = 0;
            u32offset_FlashAddress = 0;
            USART_PrepareAck(STM_ACK_FINISHED_PRI); // 0x56
            break;

        default:
            USART_PrepareAck(STM_ACK_PAUSE_PRI);
            break;
    }
}
