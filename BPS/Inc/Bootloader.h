#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "stm32h7xx.h"

#include "USB_CDC.h"

#define FLASH_START_APP1 0x08020000

#define size_u32BufferProgram 4064

#define MaximumAddress_BufferFlash 16256

#define PC_CMD_START     0x07
#define PC_CMD_SENDING   0x67
#define PC_CMD_FINISHED  0x99
#define PC_CMD_WAIT      0x20

#define STM_ACK_READY    0x49 
#define STM_ACK_PAUSE    0x69 
#define STM_ACK_FINISHED 0x55

extern volatile uint8_t __attribute__((section(".bss.Version_Program"))) Version_Edit;


extern uint32_t u32BufferProgram[size_u32BufferProgram];
volatile extern uint16_t current_program;

void bootJumpToApp1(void);

uint8_t IsFlash_WaitForLastOperation(void);
void Clear_errorflags(void);

void FLASH_access_control(void);

void Flash_Unlock(void);
void Flash_lock(void);
uint8_t IsFlash_lock(void);

uint8_t Flash_erase_Sector(uint8_t u8Sector);
void Erase_All_App(void);

typedef struct{
	union{
		uint32_t 	u32Buffer[8];
		uint16_t 	u16Buffer[16];
		uint8_t 	u8Buffer[32];
	};
}BufferFlash;

uint8_t Flash_Write_B1(uint32_t u32FlashAddress, uint32_t *u32Data32B, uint16_t u16DataCount);

void PocessCommand(void);
#endif   