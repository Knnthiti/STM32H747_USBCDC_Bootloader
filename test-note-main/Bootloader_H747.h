#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "stm32h7xx.h"

#include "Clock_system.h"

#include "USB_CDC.h"

#include "USART.h"

#define FLASH_START_APP1 0x08020000

#define size_u32BufferProgram 4064

#define MaximumAddress_BufferFlash 16256

typedef enum{
   PC_CMD_START     = 0x07,
   PC_CMD_SENDING   = 0x67,
   PC_CMD_FINISHED  = 0x99,
   PC_CMD_WAIT      = 0x20,
	
   PC_CMD_START_PRI     = 0x08,
   PC_CMD_SENDING_PRI   = 0x68,
   PC_CMD_FINISHED_PRI  = 0x9A,
   PC_CMD_WAIT_PRI      = 0x21
}PC_Command_t;

typedef enum{
   STM_ACK_READY    = 0x49,
   STM_ACK_PAUSE    = 0x69,
   STM_ACK_FINISHED = 0x55,
	
   STM_ACK_READY_PRI    = 0x4A,
   STM_ACK_PAUSE_PRI    = 0x6A,
   STM_ACK_FINISHED_PRI = 0x56
}STM_ACK_t;

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

typedef enum {
    PRI_STATE_IDLE     = 0x00,
    PRI_STATE_START_TX = 0x01,
    PRI_STATE_TX_DATA  = 0x02,
    PRI_STATE_WAIT_TC  = 0x03,
    PRI_STATE_START_RX = 0x04,
    PRI_STATE_RX_DATA  = 0x05,
    PRI_STATE_PROCESS  = 0x06,
    PRI_STATE_ERROR    = 0x07
} PRI_State_t;

extern PRI_State_t pri_currentState;

extern uint16_t pri_txIndex;
extern uint16_t pri_rxIndex;
extern uint32_t pri_timeoutStart;

#define PRI_TIMEOUT_MS 30000

void PocessCommand_PRI(void);
#endif   
