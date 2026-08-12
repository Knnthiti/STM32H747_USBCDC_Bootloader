#ifndef MAIN_BOOTLOADER_H
#define MAIN_BOOTLOADER_H
/* Custom User Headers */
#include "Clock_system.h" // Include system clock configuration header
#include "Bootloader.h"   // Include bootloader logic and definitions

/* STM32 HAL Library Header */
#include "stm32h7xx_hal.h" // Include STM32H7 Hardware Abstraction Layer (HAL)

/* STM32 Low-Layer (LL) Drivers Headers */
#include "stm32h7xx_ll_pwr.h"    // Include LL Power control
#include "stm32h7xx_ll_system.h" // Include LL System configuration
#include "stm32h7xx_ll_exti.h"   // Include LL External Interrupt/Event controller
#include "stm32h7xx_ll_rcc.h"    // Include LL Reset and Clock Control
#include "stm32h7xx_ll_crs.h"    // Include LL Clock Recovery System
#include "stm32h7xx_ll_bus.h"    // Include LL Bus configuration
#include "stm32h7xx_ll_cortex.h" // Include LL Cortex-M core features
#include "stm32h7xx_ll_utils.h"  // Include LL Utility functions (e.g., delays)
#include "stm32h7xx_ll_dma.h"    // Include LL Direct Memory Access (DMA)
#include "stm32h7xx_ll_gpio.h"   // Include LL General Purpose I/O (GPIO)

/* USB Device Headers */
#include "usb_device.h" // Include USB device initialization header
#include "USB_CDC.h"    // Include USB Communication Device Class (CDC) header

#endif
