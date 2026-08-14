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
	FLASH->CR1 &= ~FLASH_CR_PG;
	//clear EOP
	FLASH->CCR1 |= FLASH_CCR_CLR_EOP;
	
	return 0;
}

volatile _USBData TX_USBCDC_Data;

uint32_t u32offset_FlashAddress = 0;

uint32_t Triger_USB = 0;

PRI_State_t pri_currentState = PRI_STATE_IDLE;

uint16_t pri_txIndex = 0;
uint16_t pri_rxIndex = 0;
uint32_t pri_timeoutStart = 0;

volatile _USBData TX_USART_Data;
volatile _USBData RX_USART_Data;

static volatile uint8_t usb_ack_pending = 0;
static volatile uint8_t usb_ack_header = 0;

static uint8_t Is_PRI_Command(uint8_t header)
{
    return (header == PC_CMD_START_PRI) ||
           (header == PC_CMD_SENDING_PRI) ||
           (header == PC_CMD_WAIT_PRI) ||
           (header == PC_CMD_FINISHED_PRI);
}

static uint8_t CDC_SendAck(uint8_t header)
{
    TX_USBCDC_Data = (_USBData){0x00};
    TX_USBCDC_Data.u8setting1Byte.u8herder = header;
    return (CDC_Transmit_FS((uint8_t*)TX_USBCDC_Data.u8RxUSBData, u8APP_TX_DATA_SIZE) == USBD_OK);
}

static void Queue_USBAck(uint8_t header)
{
    usb_ack_header = header;
    usb_ack_pending = 1;
}

static uint8_t Flush_USBAck(void)
{
    if (usb_ack_pending == 0) {
        return 1;
    }

    if (CDC_SendAck(usb_ack_header)) {
        usb_ack_pending = 0;
        return 1;
    }

    return 0;
}

static void Copy_USB_To_USART(void)
{
    for(uint16_t i = 0; i < u8APP_RX_DATA_SIZE; i++){
        TX_USART_Data.u8RxUSBData[i] = RX_USBCDC_Data.u8RxUSBData[i];
    }
}

static void Copy_USB_To_FlashBuffer(void)
{
    for(uint16_t i = 0 ; i < 254 ; i++){
        u32BufferProgram[current_program + i] = RX_USBCDC_Data.u32RxUSBData[1 + i];
    }
    current_program += 254;
}

void PocessCommand(void)
{
    uint8_t header;

    PocessCommand_PRI();

    if (Flush_USBAck() == 0) {
        return;
    }

    if (USBCDC_FrameReady == 0) {
        return;
    }

    header = RX_USBCDC_Data.u8setting1Byte.u8herder;

    if (USBCDC_FrameCrcOk == 0) {
        Queue_USBAck(Is_PRI_Command(header) ? STM_ACK_PAUSE_PRI : STM_ACK_PAUSE);
        USBCDC_ReleaseRxFrame();
        return;
    }

    switch (header)
    {
        case PC_CMD_START: // 0x07
            __disable_irq();
            Erase_All_App();
            __enable_irq();

            current_program = 0;
            u32offset_FlashAddress = 0;
            Queue_USBAck(STM_ACK_READY);
            USBCDC_ReleaseRxFrame();
            break;

        case PC_CMD_SENDING: // 0x67
            if ((current_program + 254) <= size_u32BufferProgram) {
                Copy_USB_To_FlashBuffer();
            }

            Queue_USBAck((current_program < size_u32BufferProgram) ? STM_ACK_READY : STM_ACK_PAUSE);
            USBCDC_ReleaseRxFrame();
            break;

        case PC_CMD_WAIT: // 0x20
            if (current_program > 0) {
                __disable_irq();
                Flash_Write_B1((FLASH_START_APP1 + u32offset_FlashAddress), u32BufferProgram, current_program);
                __enable_irq();
                u32offset_FlashAddress += ((uint32_t)current_program * 4U);
            }

            current_program = 0;
            Queue_USBAck(STM_ACK_READY);
            USBCDC_ReleaseRxFrame();
            break;

        case PC_CMD_FINISHED: // 0x99
            if (current_program > 0) {
                __disable_irq();
                Flash_Write_B1((FLASH_START_APP1 + u32offset_FlashAddress), u32BufferProgram, current_program);
                __enable_irq();
            }

            Version_Edit = RX_USBCDC_Data.u8setting1Byte.u8version;
            current_program = 0;
            u32offset_FlashAddress = 0;
            Queue_USBAck(STM_ACK_FINISHED);
            USBCDC_ReleaseRxFrame();
            break;

        case PC_CMD_START_PRI:
        case PC_CMD_SENDING_PRI:
        case PC_CMD_WAIT_PRI:
        case PC_CMD_FINISHED_PRI:
            if ((pri_currentState == PRI_STATE_IDLE) || (pri_currentState == PRI_STATE_ERROR)) {
                Copy_USB_To_USART();
                USBCDC_ReleaseRxFrame();
                pri_currentState = PRI_STATE_START_TX;
            } else {
                Queue_USBAck(STM_ACK_PAUSE_PRI);
                USBCDC_ReleaseRxFrame();
            }
            break;

        default:
            Queue_USBAck(STM_ACK_PAUSE);
            USBCDC_ReleaseRxFrame();
            break;
    }
}

void PocessCommand_PRI(void) {
    switch (pri_currentState) {
        
        case PRI_STATE_IDLE:
            break;

        case PRI_STATE_START_TX:
            pri_txIndex = 0;
            pri_rxIndex = 0; 
            pri_timeoutStart = GetTick();
            RX_USART_Data = (_USBData){0x00};
            
            pri_currentState = PRI_STATE_TX_DATA;
			
            LL_USART_EnableIT_TXE(USART6);
            break;

        case PRI_STATE_TX_DATA:
        case PRI_STATE_WAIT_TC:
        case PRI_STATE_START_RX: 
        case PRI_STATE_RX_DATA:
            if ((GetTick() - pri_timeoutStart) > PRI_TIMEOUT_MS) {
                LL_USART_DisableIT_TXE(USART6);
                LL_USART_DisableIT_TC(USART6);
                LL_USART_DisableIT_RXNE(USART6);
                pri_currentState = PRI_STATE_ERROR;
            }
            break;

        case PRI_STATE_PROCESS:
            if (RX_USART_Data.u8setting1Byte.u8herder == 0x00) {
                Queue_USBAck(STM_ACK_PAUSE_PRI);
            } else {
                Queue_USBAck(RX_USART_Data.u8setting1Byte.u8herder);
            }

            if ((TX_USART_Data.u8setting1Byte.u8herder == PC_CMD_FINISHED_PRI) || (TX_USART_Data.u8setting1Byte.u8herder == PC_CMD_START_PRI)) {
                if (TX_USART_Data.u8setting1Byte.u8herder == PC_CMD_FINISHED_PRI) {
                    Version_Edit = TX_USART_Data.u8setting1Byte.u8version;
                }
            }

            TX_USART_Data = (_USBData){ 0 };
            RX_USART_Data = (_USBData){ 0 };
            
            pri_currentState = PRI_STATE_IDLE;
            break;

        case PRI_STATE_ERROR:
            LL_USART_DisableIT_TXE(USART6);
            LL_USART_DisableIT_TC(USART6);
            LL_USART_DisableIT_RXNE(USART6);
            Queue_USBAck(STM_ACK_PAUSE_PRI);
            pri_txIndex = 0;
            pri_rxIndex = 0;
            TX_USART_Data = (_USBData){ 0 };
            RX_USART_Data = (_USBData){ 0 };
            pri_currentState = PRI_STATE_IDLE; 
            break;
            
        default:
            pri_currentState = PRI_STATE_IDLE;
            break;
    }
}

void USART6_IRQHandler(void)
{

    if (LL_USART_IsActiveFlag_TXE(USART6) && LL_USART_IsEnabledIT_TXE(USART6) && (pri_currentState == PRI_STATE_TX_DATA)) {
        if (pri_txIndex < 1024) {
            LL_USART_TransmitData8(USART6, TX_USART_Data.u8RxUSBData[pri_txIndex++]);
        } 
        else {
            LL_USART_DisableIT_TXE(USART6);
            LL_USART_EnableIT_TC(USART6);
            pri_currentState = PRI_STATE_WAIT_TC;
        }
    }

    if (LL_USART_IsActiveFlag_TC(USART6) && LL_USART_IsEnabledIT_TC(USART6) && (pri_txIndex == 1024)) {
        LL_USART_ClearFlag_TC(USART6); 
        LL_USART_DisableIT_TC(USART6); 
        
        pri_rxIndex = 0;
        pri_currentState = PRI_STATE_RX_DATA;
        LL_USART_EnableIT_RXNE(USART6); 
    }

    if (LL_USART_IsActiveFlag_RXNE(USART6) && LL_USART_IsEnabledIT_RXNE(USART6) && (pri_currentState == PRI_STATE_RX_DATA)) {
        if (pri_rxIndex < 1024) {
            RX_USART_Data.u8RxUSBData[pri_rxIndex++] = LL_USART_ReceiveData8(USART6);
        }
        
        if (pri_rxIndex >= 1024) {
            LL_USART_DisableIT_RXNE(USART6);
            pri_currentState = PRI_STATE_PROCESS;
        }
    }

    if (LL_USART_IsActiveFlag_ORE(USART6)) {
        LL_USART_ClearFlag_ORE(USART6);
    }
}
