#include "Bootloader.h"

volatile uint8_t __attribute__((section(".bss.Version_Program"))) Version_Edit;

uint32_t u32BufferProgram[size_u32BufferProgram];
volatile uint16_t current_program = 0;

void bootJumpToApp1(){ 
	
	typedef int (*pFunction)(void);
	static pFunction JumpToApp;
	
	JumpToApp	= (pFunction)(*(__IO uint32_t*)(FLASH_START_APP1 + 4));
	__set_MSP(*(__IO uint32_t*)FLASH_START_APP1);
	
	__disable_irq();
	
	JumpToApp();
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

uint32_t Triger_USB = 0;

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
            RX_USBCDC_Data = (_USBData){0x00};
           
		    if((GetTick() - Triger_USB) > 10){
			Triger_USB = GetTick();
				
            TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_READY; 
            CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
			}
            break;

        case PC_CMD_SENDING: // 0x67
            if (current_program < size_u32BufferProgram) {
				if((GetTick() - Triger_USB) > 10){
			    Triger_USB = GetTick();
					
                TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_READY; // 0x49
                CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
				}
				
            } else {
				if((GetTick() - Triger_USB) > 10){
			    Triger_USB = GetTick();
					
                TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_PAUSE; // 0x69
                CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
				}
            }
            break;

        case PC_CMD_WAIT: // 0x20
			__disable_irq();
		    Flash_Write_B1((FLASH_START_APP1 + u32offset_FlashAddress) ,u32BufferProgram, size_u32BufferProgram);
		    __enable_irq();
		
            u32offset_FlashAddress += MaximumAddress_BufferFlash;
            current_program = 0;
            
		    if((GetTick() - Triger_USB) > 10){
			Triger_USB = GetTick();
			
            TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_READY; // 0x49
            CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
			}
            break;

        case PC_CMD_FINISHED: // 0x99
			__disable_irq();
		    Flash_Write_B1((FLASH_START_APP1 + u32offset_FlashAddress) ,u32BufferProgram, size_u32BufferProgram);
		    __enable_irq();
		
		    if((GetTick() - Triger_USB) > 10){
			Triger_USB = GetTick();
				
            TX_USBCDC_Data.u8setting1Byte.u8herder = STM_ACK_FINISHED; // 0x55
            CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);
				
			Version_Edit = RX_USBCDC_Data.u8setting1Byte.u8version;
            current_program = 0;
            u32offset_FlashAddress = 0; 
            RX_USBCDC_Data = (_USBData){0x00};
			}
            
            break;
/*-------------------------------------------------------------------------------------------------------------*/
		case PC_CMD_START_PRI:
//		    PocessCommand_PRI();
//		    break;
        case PC_CMD_SENDING_PRI:
//			PocessCommand_PRI();
//		    break;
		case PC_CMD_WAIT_PRI:
//			PocessCommand_PRI();
//			break;
        case PC_CMD_FINISHED_PRI:
			if((GetTick() - Triger_USB) > 10){
			  Triger_USB = GetTick();
              PocessCommand_PRI();
			}
			break;

        default:
            
            break;
    }
    
//    RX_USBCDC_Data.u8setting1Byte.u8herder = 0x00;
}

PRI_State_t pri_currentState = PRI_STATE_IDLE;

uint16_t pri_txIndex = 0;
uint16_t pri_rxIndex = 0;
uint32_t pri_timeoutStart = 0;

volatile _USBData TX_USART_Data __attribute__((aligned(32)));
volatile _USBData RX_USART_Data __attribute__((aligned(32)));

#define PRI_USART_FRAME_SIZE u8APP_RX_DATA_SIZE
#define USART6_RX_DMA_STREAM LL_DMA_STREAM_1
#define USART6_TX_DMA_STREAM LL_DMA_STREAM_2

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
#define USART6_DMA_CACHE_LINE_SIZE 32U

static void USART6_GetCacheAlignedRange(const volatile void *address, uint32_t length, uint32_t *start, uint32_t *size)
{
    uint32_t raw_start = (uint32_t)address;
    uint32_t raw_end = raw_start + length;

    *start = raw_start & ~(USART6_DMA_CACHE_LINE_SIZE - 1U);
    raw_end = (raw_end + USART6_DMA_CACHE_LINE_SIZE - 1U) & ~(USART6_DMA_CACHE_LINE_SIZE - 1U);
    *size = raw_end - *start;
}

static void USART6_CleanDCache(const volatile void *address, uint32_t length)
{
    if ((SCB->CCR & SCB_CCR_DC_Msk) != 0U) {
        uint32_t start;
        uint32_t size;

        USART6_GetCacheAlignedRange(address, length, &start, &size);
        SCB_CleanDCache_by_Addr((uint32_t *)start, (int32_t)size);
    }
}

static void USART6_InvalidateDCache(const volatile void *address, uint32_t length)
{
    if ((SCB->CCR & SCB_CCR_DC_Msk) != 0U) {
        uint32_t start;
        uint32_t size;

        USART6_GetCacheAlignedRange(address, length, &start, &size);
        SCB_InvalidateDCache_by_Addr((uint32_t *)start, (int32_t)size);
    }
}

static void USART6_CleanInvalidateDCache(const volatile void *address, uint32_t length)
{
    if ((SCB->CCR & SCB_CCR_DC_Msk) != 0U) {
        uint32_t start;
        uint32_t size;

        USART6_GetCacheAlignedRange(address, length, &start, &size);
        SCB_CleanInvalidateDCache_by_Addr((uint32_t *)start, (int32_t)size);
    }
}
#else
static void USART6_CleanDCache(const volatile void *address, uint32_t length)
{
    (void)address;
    (void)length;
}

static void USART6_InvalidateDCache(const volatile void *address, uint32_t length)
{
    (void)address;
    (void)length;
}

static void USART6_CleanInvalidateDCache(const volatile void *address, uint32_t length)
{
    (void)address;
    (void)length;
}
#endif

static void USART6_ClearRxDmaFlags(void)
{
    LL_DMA_ClearFlag_TC1(DMA1);
    LL_DMA_ClearFlag_TE1(DMA1);
    LL_DMA_ClearFlag_DME1(DMA1);
    LL_DMA_ClearFlag_FE1(DMA1);
}

static void USART6_ClearTxDmaFlags(void)
{
    LL_DMA_ClearFlag_TC2(DMA1);
    LL_DMA_ClearFlag_TE2(DMA1);
    LL_DMA_ClearFlag_DME2(DMA1);
    LL_DMA_ClearFlag_FE2(DMA1);
}

static void USART6_DisableRxDma(void)
{
    LL_USART_DisableDMAReq_RX(USART6);
    LL_DMA_DisableStream(DMA1, USART6_RX_DMA_STREAM);
    while (LL_DMA_IsEnabledStream(DMA1, USART6_RX_DMA_STREAM)) {}
}

static void USART6_DisableTxDma(void)
{
    LL_USART_DisableDMAReq_TX(USART6);
    LL_DMA_DisableStream(DMA1, USART6_TX_DMA_STREAM);
    while (LL_DMA_IsEnabledStream(DMA1, USART6_TX_DMA_STREAM)) {}
}

static void USART6_FlushRxFlags(void)
{
    while (LL_USART_IsActiveFlag_RXNE(USART6)) {
        (void)LL_USART_ReceiveData8(USART6);
    }

    if (LL_USART_IsActiveFlag_IDLE(USART6)) {
        LL_USART_ClearFlag_IDLE(USART6);
    }
    if (LL_USART_IsActiveFlag_ORE(USART6)) {
        LL_USART_ClearFlag_ORE(USART6);
    }
    if (LL_USART_IsActiveFlag_FE(USART6)) {
        LL_USART_ClearFlag_FE(USART6);
    }
    if (LL_USART_IsActiveFlag_NE(USART6)) {
        LL_USART_ClearFlag_NE(USART6);
    }
    if (LL_USART_IsActiveFlag_PE(USART6)) {
        LL_USART_ClearFlag_PE(USART6);
    }
}

static void USART6_StartPriTxDma(void)
{
    USART6_DisableTxDma();
    USART6_ClearTxDmaFlags();
    LL_USART_ClearFlag_TC(USART6);

    LL_DMA_ConfigAddresses(DMA1,
                           USART6_TX_DMA_STREAM,
                           (uint32_t)TX_USART_Data.u8RxUSBData,
                           (uint32_t)&USART6->TDR,
                           LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetDataLength(DMA1, USART6_TX_DMA_STREAM, PRI_USART_FRAME_SIZE);
    USART6_CleanDCache(TX_USART_Data.u8RxUSBData, PRI_USART_FRAME_SIZE);

    LL_DMA_EnableStream(DMA1, USART6_TX_DMA_STREAM);
    LL_USART_EnableDMAReq_TX(USART6);
}

static void USART6_StartPriRxDma(void)
{
    USART6_DisableRxDma();
    USART6_ClearRxDmaFlags();
    USART6_FlushRxFlags();

    pri_rxIndex = 0;
    RX_USART_Data = (_USBData){0x00};
    USART6_CleanInvalidateDCache(RX_USART_Data.u8RxUSBData, PRI_USART_FRAME_SIZE);

    LL_DMA_ConfigAddresses(DMA1,
                           USART6_RX_DMA_STREAM,
                           (uint32_t)&USART6->RDR,
                           (uint32_t)RX_USART_Data.u8RxUSBData,
                           LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetDataLength(DMA1, USART6_RX_DMA_STREAM, PRI_USART_FRAME_SIZE);

    LL_DMA_EnableStream(DMA1, USART6_RX_DMA_STREAM);
    LL_USART_EnableDMAReq_RX(USART6);
    LL_USART_EnableIT_IDLE(USART6);

    pri_timeoutStart = GetTick();
    pri_currentState = PRI_STATE_RX_DATA;
}

static void USART6_FinishPriRxDma(void)
{
    USART6_DisableRxDma();
    LL_USART_DisableIT_IDLE(USART6);
    USART6_InvalidateDCache(RX_USART_Data.u8RxUSBData, PRI_USART_FRAME_SIZE);
    pri_rxIndex = PRI_USART_FRAME_SIZE;
    pri_currentState = PRI_STATE_PROCESS;
}

static void USART6_AbortPriDma(void)
{
    USART6_DisableRxDma();
    USART6_DisableTxDma();
    USART6_ClearRxDmaFlags();
    USART6_ClearTxDmaFlags();
    LL_USART_DisableIT_TC(USART6);
    LL_USART_DisableIT_IDLE(USART6);
    LL_USART_DisableIT_TXE(USART6);
    LL_USART_DisableIT_RXNE(USART6);
    pri_currentState = PRI_STATE_ERROR;
}

void PocessCommand_PRI(void) {
    switch (pri_currentState) {
        
        case PRI_STATE_IDLE:
            break;

        case PRI_STATE_START_TX:
            pri_txIndex = 0;
            pri_rxIndex = 0; 
            pri_timeoutStart = GetTick();


            for(uint16_t i = 0; i < 1024; i++){
                TX_USART_Data.u8RxUSBData[i] = (uint8_t)RX_USBCDC_Data.u8RxUSBData[i]; 
            }
            
            pri_currentState = PRI_STATE_TX_DATA;
            USART6_StartPriTxDma();
            break;

        case PRI_STATE_TX_DATA:
        case PRI_STATE_WAIT_TC:
        case PRI_STATE_START_RX: 
        case PRI_STATE_RX_DATA:
            if ((GetTick() - pri_timeoutStart) > PRI_TIMEOUT_MS) {
                USART6_AbortPriDma();
            }
            break;

        case PRI_STATE_PROCESS:
            TX_USBCDC_Data = (_USBData){0x00};
            TX_USBCDC_Data.u8setting1Byte.u8herder = RX_USART_Data.u8RxUSBData[0];
            CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE);

            if ((TX_USART_Data.u8setting1Byte.u8herder == PC_CMD_FINISHED_PRI) || (TX_USART_Data.u8setting1Byte.u8herder == PC_CMD_START_PRI)) {
                if (TX_USART_Data.u8setting1Byte.u8herder == PC_CMD_FINISHED_PRI) {
                    Version_Edit = TX_USART_Data.u8setting1Byte.u8version;
                }

                TX_USART_Data = (_USBData){ 0 };
                RX_USART_Data = (_USBData){ 0 };
            }
            
            pri_currentState = PRI_STATE_IDLE;
            break;

        case PRI_STATE_ERROR:
            USART6_AbortPriDma();
            pri_txIndex = 0;
            pri_rxIndex = 0;
            pri_currentState = PRI_STATE_IDLE; 
            break;
            
        default:
            pri_currentState = PRI_STATE_IDLE;
            break;
    }
}

void USART6_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_TC(USART6) && LL_USART_IsEnabledIT_TC(USART6) && (pri_currentState == PRI_STATE_WAIT_TC)) {
        LL_USART_ClearFlag_TC(USART6); 
        LL_USART_DisableIT_TC(USART6); 
        USART6_StartPriRxDma();
    }

    if (LL_USART_IsActiveFlag_IDLE(USART6) && LL_USART_IsEnabledIT_IDLE(USART6)) {
        LL_USART_ClearFlag_IDLE(USART6);
        if (pri_currentState == PRI_STATE_RX_DATA) {
            uint16_t remaining = (uint16_t)LL_DMA_GetDataLength(DMA1, USART6_RX_DMA_STREAM);
            pri_rxIndex = PRI_USART_FRAME_SIZE - remaining;
            if (remaining == 0U) {
                USART6_FinishPriRxDma();
            }
        }
    }

    if (LL_USART_IsActiveFlag_ORE(USART6)) {
        LL_USART_ClearFlag_ORE(USART6);
        USART6_AbortPriDma();
    }
    if (LL_USART_IsActiveFlag_FE(USART6)) {
        LL_USART_ClearFlag_FE(USART6);
        USART6_AbortPriDma();
    }
    if (LL_USART_IsActiveFlag_NE(USART6)) {
        LL_USART_ClearFlag_NE(USART6);
        USART6_AbortPriDma();
    }
    if (LL_USART_IsActiveFlag_PE(USART6)) {
        LL_USART_ClearFlag_PE(USART6);
        USART6_AbortPriDma();
    }
}

void DMA1_Stream1_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_TC1(DMA1)) {
        LL_DMA_ClearFlag_TC1(DMA1);
        if (pri_currentState == PRI_STATE_RX_DATA) {
            USART6_FinishPriRxDma();
        }
    }

    if (LL_DMA_IsActiveFlag_TE1(DMA1)) {
        LL_DMA_ClearFlag_TE1(DMA1);
        USART6_AbortPriDma();
    }
    if (LL_DMA_IsActiveFlag_DME1(DMA1)) {
        LL_DMA_ClearFlag_DME1(DMA1);
        USART6_AbortPriDma();
    }
    if (LL_DMA_IsActiveFlag_FE1(DMA1)) {
        LL_DMA_ClearFlag_FE1(DMA1);
        USART6_AbortPriDma();
    }
}

void DMA1_Stream2_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_TC2(DMA1)) {
        LL_DMA_ClearFlag_TC2(DMA1);
        if (pri_currentState == PRI_STATE_TX_DATA) {
            USART6_DisableTxDma();
            pri_txIndex = PRI_USART_FRAME_SIZE;
            pri_timeoutStart = GetTick();
            pri_currentState = PRI_STATE_WAIT_TC;
            LL_USART_EnableIT_TC(USART6);
        }
    }

    if (LL_DMA_IsActiveFlag_TE2(DMA1)) {
        LL_DMA_ClearFlag_TE2(DMA1);
        USART6_AbortPriDma();
    }
    if (LL_DMA_IsActiveFlag_DME2(DMA1)) {
        LL_DMA_ClearFlag_DME2(DMA1);
        USART6_AbortPriDma();
    }
    if (LL_DMA_IsActiveFlag_FE2(DMA1)) {
        LL_DMA_ClearFlag_FE2(DMA1);
        USART6_AbortPriDma();
    }
}
