/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "hardware.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RTC_TIMESTAMP_Pin GPIO_PIN_13
#define RTC_TIMESTAMP_GPIO_Port GPIOC
#define ADC_CURRENT_SENSE_Pin GPIO_PIN_0
#define ADC_CURRENT_SENSE_GPIO_Port GPIOC
#define TOGGLE400_RESET_BTN_Pin GPIO_PIN_1
#define TOGGLE400_RESET_BTN_GPIO_Port GPIOC
#define ENABLE_POWER_12V_Pin GPIO_PIN_2
#define ENABLE_POWER_12V_GPIO_Port GPIOC
#define SIMPLE_INTERLOCK_Pin GPIO_PIN_3
#define SIMPLE_INTERLOCK_GPIO_Port GPIOC
#define PA1_RESERVED_ASK_SHAUL_Pin GPIO_PIN_1
#define PA1_RESERVED_ASK_SHAUL_GPIO_Port GPIOA
#define UART2_SCREEN_TX_Pin GPIO_PIN_2
#define UART2_SCREEN_TX_GPIO_Port GPIOA
#define UART2_SCREEN_RX_Pin GPIO_PIN_3
#define UART2_SCREEN_RX_GPIO_Port GPIOA
#define CHIP_SELECT_SERIAL_Pin GPIO_PIN_4
#define CHIP_SELECT_SERIAL_GPIO_Port GPIOA
#define LVDS_RESERVED_Pin GPIO_PIN_4
#define LVDS_RESERVED_GPIO_Port GPIOC
#define CHIP_SELECT_RESERVED_Pin GPIO_PIN_5
#define CHIP_SELECT_RESERVED_GPIO_Port GPIOC
#define CHIP_SELECT_MCU_MEM_Pin GPIO_PIN_0
#define CHIP_SELECT_MCU_MEM_GPIO_Port GPIOB
#define WATCHDOG_OUT_Pin GPIO_PIN_1
#define WATCHDOG_OUT_GPIO_Port GPIOB
#define POWER_FAIL_MCU_Pin GPIO_PIN_2
#define POWER_FAIL_MCU_GPIO_Port GPIOB
#define POWER_FAIL_MCU_EXTI_IRQn EXTI2_3_IRQn
#define DEBUG_CLK_AUX_Pin GPIO_PIN_10
#define DEBUG_CLK_AUX_GPIO_Port GPIOB
#if DEBUG_NO_BEEP
#define BUZZER_Pin GPIO_PIN_12
#define BUZZER_GPIO_Port GPIOB
#else
#define BUZZER_Pin GPIO_PIN_11
#define BUZZER_GPIO_Port GPIOB
#endif
#define BLINKING_LED_Pin GPIO_PIN_12
#define BLINKING_LED_GPIO_Port GPIOB
#define DEBUG_AM_OK_Pin GPIO_PIN_13
#define DEBUG_AM_OK_GPIO_Port GPIOB
#define DEBUG_APT_OK_Pin GPIO_PIN_14
#define DEBUG_APT_OK_GPIO_Port GPIOB
#define DEBUG_MCU_OK_Pin GPIO_PIN_15
#define DEBUG_MCU_OK_GPIO_Port GPIOB
#define RESERVED_UART6_TX_Pin GPIO_PIN_6
#define RESERVED_UART6_TX_GPIO_Port GPIOC
#define RESERVED_UART6_RX_Pin GPIO_PIN_7
#define RESERVED_UART6_RX_GPIO_Port GPIOC
#define DEBUG_MOTOR_RUNNING_Pin GPIO_PIN_8
#define DEBUG_MOTOR_RUNNING_GPIO_Port GPIOC
#define DEBUG_PRESSURE_OK_Pin GPIO_PIN_9
#define DEBUG_PRESSURE_OK_GPIO_Port GPIOC
#define UART1_PC_TX_Pin GPIO_PIN_9
#define UART1_PC_TX_GPIO_Port GPIOA
#define UART1_PC_RX_Pin GPIO_PIN_10
#define UART1_PC_RX_GPIO_Port GPIOA
#define USB_DM_RESERVED_Pin GPIO_PIN_11
#define USB_DM_RESERVED_GPIO_Port GPIOA
#define USB_DP_RESERVED_Pin GPIO_PIN_12
#define USB_DP_RESERVED_GPIO_Port GPIOA
#define LOGIC_BYPASS_4_Pin GPIO_PIN_15
#define LOGIC_BYPASS_4_GPIO_Port GPIOA
#define LOGIC_BYPASS_3_Pin GPIO_PIN_10
#define LOGIC_BYPASS_3_GPIO_Port GPIOC
#define LOGIC_BYPASS_2_Pin GPIO_PIN_11
#define LOGIC_BYPASS_2_GPIO_Port GPIOC
#define LOGIC_BYPASS_1_Pin GPIO_PIN_12
#define LOGIC_BYPASS_1_GPIO_Port GPIOC
#define DEBUG_LATCH_AUX_Pin GPIO_PIN_2
#define DEBUG_LATCH_AUX_GPIO_Port GPIOD
#define SYS_SWO_RESERVERED_Pin GPIO_PIN_3
#define SYS_SWO_RESERVERED_GPIO_Port GPIOB
#define DEBUG_DATA_AUX_Pin GPIO_PIN_4
#define DEBUG_DATA_AUX_GPIO_Port GPIOB
#define DEBUG_STATE_3_Pin GPIO_PIN_5
#define DEBUG_STATE_3_GPIO_Port GPIOB
#define DEBUG_STATE_2_Pin GPIO_PIN_6
#define DEBUG_STATE_2_GPIO_Port GPIOB
#define DEBUG_STATE_1_Pin GPIO_PIN_7
#define DEBUG_STATE_1_GPIO_Port GPIOB
#define DEBUG_BRK1_Pin GPIO_PIN_8
#define DEBUG_BRK1_GPIO_Port GPIOB
#define DEBUG_BRK2_Pin GPIO_PIN_9
#define DEBUG_BRK2_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
#define DMA_LENGTH 4
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
