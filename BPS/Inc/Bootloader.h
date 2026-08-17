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

/******************************************************************************
  * @FunctionName : bootJumpToApp1()
  * @Description  : This function jumps from bootloader to application 1.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void bootJumpToApp1(void);

/******************************************************************************
  * @FunctionName : IsFlash_WaitForLastOperation()
  * @Description  : This function checks whether the last Flash operation is busy.
  * @note         :
  * @Param        : None.
  * @Return       : 1 when Flash is busy, otherwise 0.
  ******************************************************************************/
uint8_t IsFlash_WaitForLastOperation(void);

/******************************************************************************
  * @FunctionName : Clear_errorflags()
  * @Description  : This function clears Flash error and status flags.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void Clear_errorflags(void);

/******************************************************************************
  * @FunctionName : FLASH_access_control()
  * @Description  : This function configures Flash access latency and locks Flash.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void FLASH_access_control(void);

/******************************************************************************
  * @FunctionName : Flash_Unlock()
  * @Description  : This function unlocks Flash bank 1 control register.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void Flash_Unlock(void);

/******************************************************************************
  * @FunctionName : Flash_lock()
  * @Description  : This function locks Flash bank 1 control register.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void Flash_lock(void);

/******************************************************************************
  * @FunctionName : IsFlash_lock()
  * @Description  : This function checks whether Flash bank 1 is locked.
  * @note         :
  * @Param        : None.
  * @Return       : 1 when Flash is locked, otherwise 0.
  ******************************************************************************/
uint8_t IsFlash_lock(void);

/******************************************************************************
  * @FunctionName : Flash_erase_Sector()
  * @Description  : This function erases one Flash sector in bank 1.
  * @note         :
  * @Param        : u8Sector: Flash sector number to erase.
  * @Return       : 0 when erase is started and completed, otherwise 1.
  ******************************************************************************/
uint8_t Flash_erase_Sector(uint8_t u8Sector);

/******************************************************************************
  * @FunctionName : Erase_All_App()
  * @Description  : This function erases all application Flash sectors.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void Erase_All_App(void);

typedef struct{
	union{
		uint32_t 	u32Buffer[8];
		uint16_t 	u16Buffer[16];
		uint8_t 	u8Buffer[32];
	};
}BufferFlash;

/******************************************************************************
  * @FunctionName : Flash_Write_B1()
  * @Description  : This function writes 32-bit data words to Flash bank 1.
  * @note         :
  * @Param        : u32FlashAddress: Destination Flash address.
  * @Param        : u32Data32B: Pointer to source data buffer.
  * @Param        : u16DataCount: Number of 32-bit words to write.
  * @Return       : 0 when write is completed, otherwise 1.
  ******************************************************************************/
uint8_t Flash_Write_B1(uint32_t u32FlashAddress, uint32_t *u32Data32B, uint16_t u16DataCount);

/******************************************************************************
  * @FunctionName : PocessCommand()
  * @Description  : This function processes USB CDC bootloader commands.
  * @note         :
  * @Param        : None.
  * @Return       : None.
  ******************************************************************************/
void PocessCommand(void);
#endif
