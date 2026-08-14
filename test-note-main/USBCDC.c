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


void GPIO_USB_Init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOA);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

static int8_t CDC_Init_FS(void)
{
  /* USER CODE BEGIN 3 */
  /* Set Application Buffers */
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, (uint8_t*)USBBufferFS.u8TxBufferFS, 0);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, (uint8_t*)USBBufferFS.u8RxBufferFS);
  return (USBD_OK);
  /* USER CODE END 3 */
}

static int8_t CDC_DeInit_FS(void)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
}

static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
  /* USER CODE BEGIN 5 */
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

  /*******************************************************************************/
  /* Line Coding Structure                                                       */
  /*-----------------------------------------------------------------------------*/
  /* Offset | Field       | Size | Value  | Description                          */
  /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
  /* 4      | bCharFormat |   1  | Number | Stop bits                            */
  /*                                        0 - 1 Stop bit                       */
  /*                                        1 - 1.5 Stop bits                    */
  /*                                        2 - 2 Stop bits                      */
  /* 5      | bParityType |  1   | Number | Parity                               */
  /*                                        0 - None                             */
  /*                                        1 - Odd                              */
  /*                                        2 - Even                             */
  /*                                        3 - Mark                             */
  /*                                        4 - Space                            */
  /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
  /*******************************************************************************/
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
  /* USER CODE END 5 */
}

static int8_t CDC_TransmitCplt_FS(uint8_t *Buf, uint32_t *Len, uint8_t epnum)
{
  uint8_t result = USBD_OK;
  /* USER CODE BEGIN 13 */
  UNUSED(Buf);
  UNUSED(Len);
  UNUSED(epnum);
  /* USER CODE END 13 */
  return result;
}



uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;
  /* USER CODE BEGIN 7 */
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
  if (hcdc->TxState != 0){
    return USBD_BUSY;
  }
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, Buf, Len);
  result = USBD_CDC_TransmitPacket(&hUsbDeviceFS);
  /* USER CODE END 7 */
  return result;
}

volatile uint32_t My_CRC = 0;
volatile uint32_t My_CRC_SW = 0;
volatile uint32_t Received_CRC = 67;

volatile uint32_t current_rx_index = 0;

static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
	if ((current_rx_index + *Len) <= u8APP_RX_DATA_SIZE){
        for(uint8_t i = 0 ; i < *Len ; i++){
			RX_USBCDC_Data.u8RxUSBData[current_rx_index + i] = Buf[i];
		}
        current_rx_index += *Len;
    }
	
	if (current_rx_index >= u8APP_RX_DATA_SIZE){
	   CRC_APP_RX_DATA();
		
	   current_rx_index = 0;
		
		if((My_CRC_SW == Received_CRC) && (RX_USBCDC_Data.u8setting1Byte.u8herder == PC_CMD_SENDING)){
		for(uint16_t i = 0 ; i < 254 ; i++){
			u32BufferProgram[current_program + i] = RX_USBCDC_Data.u32RxUSBData[ 1 + i ];
		}	
		current_program += 254;

	    }
		
//		((My_CRC_SW == Received_CRC) && (RX_USBCDC_Data.u8setting1Byte.u8herder == PC_CMD_START_PRI))
		if(My_CRC_SW == Received_CRC){
//			if(RX_USBCDC_Data.u8setting1Byte.u8herder == PC_CMD_START_PRI){
		       pri_currentState = PRI_STATE_START_TX;
//			}
	    }

    }
	
	USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
	USBD_CDC_ReceivePacket(&hUsbDeviceFS);
	return (USBD_OK);
}

void CRC_APP_RX_DATA(void){
//	LL_CRC_ResetCRCCalculationUnit(CRC);
//	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_0);
//	
//	while(LL_DMA_IsActiveFlag_TC0(DMA1) == 0) {
//		
//    }
//    LL_DMA_ClearFlag_TC0(DMA1);
//	
//	My_CRC = LL_CRC_ReadData32(CRC);
//	
//    LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_0);
	
	My_CRC_SW = software_crc32((uint32_t*)RX_USBCDC_Data.u32RxUSBData, 255);
	
	Received_CRC = RX_USBCDC_Data.u32RxUSBData[255];
}

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

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;

void OTG_FS_IRQHandler(void)
{
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}
