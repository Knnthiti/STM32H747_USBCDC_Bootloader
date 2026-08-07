#ifndef USB_CDC_H
#define USB_CDC_H

#include "usbd_cdc.h"

#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_crc.h"
#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_utils.h"

#include "Bootloader.h"

#define u8USB_RX_DATA_SIZE  64
#define u8USB_TX_DATA_SIZE  64

#define u32USB_RX_DATA_SIZE  u8USB_RX_DATA_SIZE >> 2
#define u32USB_TX_DATA_SIZE  u8USB_TX_DATA_SIZE >> 2

#define u8APP_RX_DATA_SIZE  1024
#define u8APP_TX_DATA_SIZE  1024

#define u32APP_RX_DATA_SIZE  u8APP_RX_DATA_SIZE >> 2
#define u32APP_TX_DATA_SIZE  u8APP_TX_DATA_SIZE >> 2

typedef struct __attribute__((packed)){
   union{
	uint8_t u8RxBufferFS[u8USB_RX_DATA_SIZE];
	uint32_t u32RxBufferFS[u32USB_RX_DATA_SIZE];
   };
   
   union{
	uint8_t u8TxBufferFS[u8USB_TX_DATA_SIZE];
	uint32_t u32TxBufferFS[u32USB_TX_DATA_SIZE];
   };
}_USBBufferFS;
	
extern volatile _USBBufferFS USBBufferFS;


typedef struct __attribute__((packed)){
    union {
        //(Header 4 + Buffer 1016 + CRC 4 = 1024 Bytes)
        struct {
            union {
                struct setting {
                    uint8_t u8herder;
                    uint8_t u8version;
                    uint16_t u16size;
                } u8setting1Byte;
                uint32_t u32setting4Byte;
            };
            
            uint32_t u32BufferData[254]; // 254 * 4 = 1016 Bytes
            
            union {
                uint32_t u32CRC4Byte;
                struct u8CRC {
                    uint8_t CRCByte0;
                    uint8_t CRCByte1;
                    uint8_t CRCByte2;
                    uint8_t CRCByte3;
                } CRC1Byte;
            };
        };

        uint8_t u8RxUSBData[u8APP_RX_DATA_SIZE];
        uint32_t u32RxUSBData[u32APP_RX_DATA_SIZE];
    };
}_USBData;

extern volatile _USBData RX_USBCDC_Data;

/** CDC Interface callback. */
extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;

void GPIO_USB_Init(void);
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);

void CRC_APP_RX_DATA(void);
void CRC_DmaInit(uint32_t u32MemAddr, uint32_t u32memLength);
uint32_t software_crc32(uint32_t *data, uint16_t length);

#endif