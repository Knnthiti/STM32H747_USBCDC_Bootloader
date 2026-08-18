#include "USB_CDC.h"

volatile _USBBufferFS USBBufferFS;
volatile _USBData RX_USBCDC_Data;


extern USBD_HandleTypeDef hUsbDeviceFS;

static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t *Len);
static int8_t CDC_TransmitCplt_FS(uint8_t *pbuf, uint32_t *Len, uint8_t epnum);

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS =
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,
  CDC_Receive_FS,
  CDC_TransmitCplt_FS
};


/******************************************************************************
  * @FunctionName : GPIO_USB_Init()
  * @Description  : This function configures GPIO pins used by USB CDC.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void GPIO_USB_Init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOA);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/******************************************************************************
  * @FunctionName : CDC_Init_FS()
  * @Description  : This function initializes the USB CDC interface buffers.
  * @note         :
  * @Param        : None.
  * @Return       : USBD_OK when initialization is completed.
  ******************************************************************************/
static int8_t CDC_Init_FS(void)
{
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, (uint8_t*)USBBufferFS.u8TxBufferFS, 0);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, (uint8_t*)USBBufferFS.u8RxBufferFS);
  return (USBD_OK);
}

/******************************************************************************
  * @FunctionName : CDC_DeInit_FS()
  * @Description  : This function deinitializes the USB CDC interface.
  * @note         :
  * @Param        : None.
  * @Return       : USBD_OK when deinitialization is completed.
  ******************************************************************************/
static int8_t CDC_DeInit_FS(void)
{
  return (USBD_OK);
}

/******************************************************************************
  * @FunctionName : CDC_Control_FS()
  * @Description  : This function handles USB CDC class control commands.
  * @note         :
  * @Param        : cmd: CDC control command.
  * @Param        : pbuf: Pointer to command data buffer.
  * @Param        : length: Length of command data.
  * @Return       : USBD_OK when command handling is completed.
  ******************************************************************************/
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
  switch(cmd)
  {
    case CDC_SEND_ENCAPSULATED_COMMAND:

    break;

    case CDC_GET_ENCAPSULATED_RESPONSE:

    break;

    case CDC_SET_COMM_FEATURE:

    break;

    case CDC_GET_COMM_FEATURE:

    break;

    case CDC_CLEAR_COMM_FEATURE:

    break;

    case CDC_SET_LINE_CODING:

    break;

    case CDC_GET_LINE_CODING:

    break;

    case CDC_SET_CONTROL_LINE_STATE:

    break;

    case CDC_SEND_BREAK:

    break;

  default:
    break;
  }

  return (USBD_OK);
}

/******************************************************************************
  * @FunctionName : CDC_TransmitCplt_FS()
  * @Description  : This function handles USB CDC transmit completion callback.
  * @note         :
  * @Param        : Buf: Pointer to transmitted data buffer.
  * @Param        : Len: Pointer to transmitted data length.
  * @Param        : epnum: Endpoint number.
  * @Return       : USBD_OK when callback handling is completed.
  ******************************************************************************/
static int8_t CDC_TransmitCplt_FS(uint8_t *Buf, uint32_t *Len, uint8_t epnum)
{
  uint8_t result = USBD_OK;

  UNUSED(Buf);
  UNUSED(Len);
  UNUSED(epnum);

  return result;
}



/******************************************************************************
  * @FunctionName : CDC_Transmit_FS()
  * @Description  : This function transmits data through USB CDC.
  * @note         :
  * @Param        : Buf: Pointer to transmit data buffer.
  * @Param        : Len: Number of bytes to transmit.
  * @Return       : USBD_OK when transmit starts, otherwise USBD_BUSY.
  ******************************************************************************/
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;

  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;

  if (hcdc->TxState != 0){
    return USBD_BUSY;
  }

  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, Buf, Len);
  result = USBD_CDC_TransmitPacket(&hUsbDeviceFS);

  return result;
}

volatile uint32_t My_CRC = 0;
volatile uint32_t My_CRC_SW = 0;
volatile uint32_t Received_CRC = 67;

volatile uint32_t current_rx_index = 0;

/******************************************************************************
  * @FunctionName : CDC_Receive_FS()
  * @Description  : This function receives USB CDC data and stores bootloader packets.
  * @note         :
  * @Param        : Buf: Pointer to received data buffer.
  * @Param        : Len: Pointer to received data length.
  * @Return       : USBD_OK when receive handling is completed.
  ******************************************************************************/
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
	if ((current_rx_index + *Len) <= u8APP_RX_DATA_SIZE){
        for(uint8_t i = 0 ; i < *Len ; i++){
			RX_USBCDC_Data.u8RxUSBData[current_rx_index + i] = Buf[i];
		}
        current_rx_index += *Len;
    }
	
	/* Process the packet only after a complete 1024-byte frame is collected. */
	if (current_rx_index >= u8APP_RX_DATA_SIZE){
	   CRC_APP_RX_DATA();
		
	   current_rx_index = 0;
		
		/* Accept firmware payload only when CRC is valid and command is SENDING. */
		if((My_CRC == Received_CRC) && (RX_USBCDC_Data.u8setting1Byte.u8herder == PC_CMD_SENDING)){
		/* Skip the first 32-bit header word and copy 254 payload words. */
		for(uint16_t i = 0 ; i < 254 ; i++){
			u32BufferProgram[current_program + i] = RX_USBCDC_Data.u32RxUSBData[ 1 + i ];
		}	

		current_program += 254;
	    }
    }
	
	
	USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
	USBD_CDC_ReceivePacket(&hUsbDeviceFS);

	return (USBD_OK);
}

/******************************************************************************
  * @FunctionName : CRC_APP_RX_DATA()
  * @Description  : This function calculates and reads CRC values for received app data.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void CRC_APP_RX_DATA(void){
	LL_CRC_ResetCRCCalculationUnit(CRC);

	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_0);
	
	while(LL_DMA_IsActiveFlag_TC0(DMA1) == 0) {
		
    }

    LL_DMA_ClearFlag_TC0(DMA1);
	
	My_CRC = LL_CRC_ReadData32(CRC);
	
    LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_0);
	
	My_CRC_SW = software_crc32((uint32_t*)RX_USBCDC_Data.u32RxUSBData, 255);
	
	Received_CRC = RX_USBCDC_Data.u32RxUSBData[255];
}

/******************************************************************************
  * @FunctionName : CRC_DmaInit()
  * @Description  : This function initializes CRC hardware and DMA transfer settings.
  * @note         :
  * @Param        : u32MemAddr: Source memory address for CRC calculation.
  * @Param        : u32memLength: Number of 32-bit words for DMA transfer.
  * @Return       : None.
  ******************************************************************************/
void CRC_DmaInit(uint32_t u32MemAddr, uint32_t u32memLength) {

    /* 1. Initialize the CRC unit */ 
    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_CRC); // CRC
    
    LL_CRC_ResetCRCCalculationUnit(CRC);
    LL_CRC_SetPolynomialSize(CRC, LL_CRC_POLYLENGTH_32B);
    LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_NONE);
    LL_CRC_SetInitialData(CRC, 0xFFFFFFFF);
    LL_CRC_SetPolynomialCoef(CRC, 0x4C11DB7);
    
    /* 2. Enable Clocks */
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1); // DMA1

    /* 3. Configure DMA1 Stream 0 */
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_STREAM_0, LL_DMA_DIRECTION_MEMORY_TO_MEMORY);
    LL_DMA_SetStreamPriorityLevel(DMA1, LL_DMA_STREAM_0, LL_DMA_PRIORITY_HIGH);
    LL_DMA_SetMode(DMA1, LL_DMA_STREAM_0, LL_DMA_MODE_NORMAL);
    
    /* 4. Peripheral/Memory Increment */
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_STREAM_0, LL_DMA_PERIPH_INCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_STREAM_0, LL_DMA_MEMORY_NOINCREMENT);
    
    /* 5. Set 32-bit word alignment */
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_STREAM_0, LL_DMA_PDATAALIGN_WORD);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_STREAM_0, LL_DMA_MDATAALIGN_WORD);
    
    /* 6. Configure addresses */
    LL_DMA_ConfigAddresses(
            DMA1, LL_DMA_STREAM_0,
            u32MemAddr,
            (uint32_t)&(CRC->DR),
            LL_DMA_DIRECTION_MEMORY_TO_MEMORY
        );
    
    /* 7. Set Data Length */
    LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_0, u32memLength);
    
    /* 8. Enable Interrupts */
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_0);
    LL_DMA_ClearFlag_TC0(DMA1);
}


/******************************************************************************
  * @FunctionName : software_crc32()
  * @Description  : This function calculates CRC32 in software.
  * @note         :
  * @Param        : data: Pointer to source data buffer.
  * @Param        : length: Number of 32-bit words to calculate.
  * @Return       : Calculated CRC32 value.
  ******************************************************************************/
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

/******************************************************************************
  * @FunctionName : HAL_PWR_PVDCallback()
  * @Description  : This function handles the PVD callback.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void HAL_PWR_PVDCallback(void)
{

}

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;

/******************************************************************************
  * @FunctionName : OTG_FS_IRQHandler()
  * @Description  : This function handles the USB OTG FS interrupt.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void OTG_FS_IRQHandler(void)
{
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}
