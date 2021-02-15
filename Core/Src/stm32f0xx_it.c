/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f0xx_it.c
  * @brief   Interrupt Service Routines.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f0xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "common.h"
#include "spi.h"
#include "stdbool.h"
#include "hardware.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
void pull_chipselet_line(GPIO_PinState status)
{
	HAL_GPIO_WritePin(CHIP_SELECT_SERIAL_GPIO_Port, CHIP_SELECT_SERIAL_Pin, status);
}
void cs_generic(GPIO_PinState status, enum cs_decoder picked_cs, byte8_t_reg_cs* current)
{
	/***********************************************************************
	 * IMPORTANT: The usual "Hardware logic" is chipselect is active on low.
	 * But since we have a special case where the cs_register is reset to 0
	 * We have inverted all cs inputs to the hardware from cs_register
	 * so what needs to be done is that the register written logical 1
	 * to get an electrical VLow
	 ***********************************************************************/
	
	if (status == GPIO_PIN_SET)
	{
		//If we need to set high - we want to disable all CS_es
		pull_chipselet_line(HIGH);
	}
	else
	{
		/*
		 * Maybe should add check that the selected line was not already set as cs. That way we will not have to do additional data transfer.
		 */
		current->bits.DECODER = picked_cs;
		current->bits.INB_MOTOR = 0;
		HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, &(current->value), 1, 1);
		
		uint32_t start_time = HAL_GetTick();
		while (status != HAL_OK && ((HAL_GetTick() - start_time )< 2))
		{
			status = HAL_SPI_Transmit(&hspi1, &(current->value), 1, 1);
		}
		pull_chipselet_line(LOW);
	}
}
	
void cs3(GPIO_PinState status, byte8_t_reg_cs* current)
{
	enum cs_decoder picked_cs = CS_MEM3;
	cs_generic(status, picked_cs, current);
}

HAL_StatusTypeDef write_enable(void)
{
	uint8_t spi_write_enable[1];
	spi_write_enable[0] = WRITE_ENABLE;          // write enable
	return HAL_SPI_Transmit(&hspi1, spi_write_enable, 1, 1);
}

HAL_StatusTypeDef write_disable(void)
{
	uint8_t spi_write_disable[1];
	spi_write_disable[0] = WRITE_DISABLE;       // write enable
	return HAL_SPI_Transmit(&hspi1, spi_write_disable, 1, 2);
}

uint32_t read(uint32_t read_address)
{

	uint8_t spi_read_command[1];
	spi_read_command[0] = READ_ARRAY1;
	/*Read an array
	 *0x0B needs an additional dummy byte to transmit data
	 *difference is 0x03 can be used only at lower speeds
	 *0x0B can be used at any speed
	 *ONLY 0x0B needs a dummy byte
	*/
	HAL_SPI_Transmit(&hspi1, spi_read_command, 1, 1);

	uint8_t address[4];
	uint8_t spi_read_data[4];
	translate_int_to_address(read_address, address);
	address[3] = 0xFF;     //doesn't matter

	HAL_SPI_Transmit(&hspi1, address, 4, 1);
	HAL_SPI_Receive(&hspi1, spi_read_data, 4, 1);

	return translate_address_to_int(spi_read_data);
}
HAL_StatusTypeDef write(volatile uint32_t write_address, volatile uint32_t write_value)
{
	uint8_t spi_read_command[1];
	spi_read_command[0] = BYTE_PROGRAM;
	// Address start : where to write :
	uint8_t address[4];
	uint8_t counter_in_bytes[4];
	volatile uint8_t complete_write[8];

	address[3] = 0xFF;       //doesn't matter - is not sent
	translate_int_to_address(write_address, address);
	translate_int_to_arr(write_value, counter_in_bytes);
	// Address end :

	// after push address start to write data one by one ....
	// 		until length 256 total per page

	// datas start  from address  start 00-0F-FFh  00-00-00h

	complete_write[0] = spi_read_command[0];
	complete_write[1] = address[0];
	complete_write[2] = address[1];
	complete_write[3] = address[2];
	complete_write[4] = counter_in_bytes[0];
	complete_write[5] = counter_in_bytes[1];
	complete_write[6] = counter_in_bytes[2];
	complete_write[7] = counter_in_bytes[3];
	return HAL_SPI_Transmit(&hspi1, (uint8_t *)complete_write, 8, 1);
}
HAL_StatusTypeDef write_once(uint32_t write_address, uint32_t write_value)
{
	static byte8_t_reg_cs current;
	current.bits.INA_MOTOR = 0;
	current.bits.INB_MOTOR = 0;
	current.bits.DECODER = CS_MEM3;
	current.bits.QC = 0;
	current.bits.QG = 0;
	current.bits.QH = 0;
	// --------------------------------------------------------------
	// write  start  : Counter
	// counter value
	//spi_erase_4_kb_from_ff(ChipSelectNumber, write_adress);
	volatile uint32_t addr_temp = write_address;
	volatile uint32_t value_temp = write_value;
	cs3(LOW, &current);

	if (write_enable() != HAL_OK)
	{
		cs3(HIGH, &current);
		return HAL_ERROR;
	}
	cs3(HIGH, &current);
	delay(100);
	
	cs3(LOW, &current);
	write(addr_temp, value_temp);
	cs3(HIGH, &current);
	delay(100);

	cs3(LOW, &current);
	volatile uint32_t read_back = read(write_address);
	cs3(HIGH, &current);

	if (read_back != write_value)
	{
		return HAL_ERROR;
	}
	return HAL_OK;
}
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
extern struct PF_memory pf_memory;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t read_reset_button(void)
{
	return (uint16_t)HAL_GPIO_ReadPin(TOGGLE400_RESET_BTN_GPIO_Port, TOGGLE400_RESET_BTN_Pin);
}
bool test_for_reset_press(void) {
 
	static uint16_t button_history = 0;
	bool pressed = false;    
 
	button_history = button_history << 1;
	button_history |= read_reset_button();
	if ((button_history & 0xF0FF) == 0xFF)
	{ 
		pressed = true;
		button_history = 0xFFFF;
	}
	return pressed;
}
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_adc;
extern TIM_HandleTypeDef htim16;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim1;

extern TIM_HandleTypeDef htim14;
/* USER CODE BEGIN EV */
extern volatile bool RESET_PRESSED;
/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M0 Processor Interruption and Exception Handlers          */ 
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVC_IRQn 0 */

  /* USER CODE END SVC_IRQn 0 */
  /* USER CODE BEGIN SVC_IRQn 1 */

  /* USER CODE END SVC_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F0xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f0xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line 2 and 3 interrupts.
  */
void EXTI2_3_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI2_3_IRQn 0 */
	/**************************************************************************************/
	/**  Power Fail interrupt:                                                           **/
	/**  fast transfer last count of pulses before we stop the device                    **/
	/**************************************************************************************/
	//
	
	__disable_irq();
	static byte8_t_reg_cs current;
	current.bits.INA_MOTOR = 0;
	current.bits.INB_MOTOR = 0;
	current.bits.DECODER = CS_MEM3;
	current.bits.QC = 0;
	current.bits.QG = 0;
	current.bits.QH = 0;
	cs3(LOW, &current);
	//delay(10);
	cs3(HIGH, &current);
	

	// CS REGISTER ON CS3 low;
	if (*pf_memory.state != UartDebug)
	{
		if (pf_memory.pulses_last_time_written_v < pf_memory.pulses_need_to_write_on_PF_v)
		{
			pf_memory.AM_address_to_write_on_PF_v += 4;
			HAL_StatusTypeDef status = write_once(pf_memory.AM_address_to_write_on_PF_v, pf_memory.pulses_need_to_write_on_PF_v);
			while (status != HAL_OK)
			{
				
				status = write_once(pf_memory.AM_address_to_write_on_PF_v, pf_memory.pulses_need_to_write_on_PF_v);
				pf_memory.AM_address_to_write_on_PF_v += 4;
				//HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, GPIO_PIN_RESET);
			}
		}
	}
	else
	{
		// Buzzer on Power Fail if in DEBUG mode
		HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
	}

  /* USER CODE END EXTI2_3_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
  /* USER CODE BEGIN EXTI2_3_IRQn 1 */
	__enable_irq();
  /* USER CODE END EXTI2_3_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel 1 global interrupt.
  */
void DMA1_Channel1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel1_IRQn 0 */

  /* USER CODE END DMA1_Channel1_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_adc);
  /* USER CODE BEGIN DMA1_Channel1_IRQn 1 */

  /* USER CODE END DMA1_Channel1_IRQn 1 */
}

/**
  * @brief This function handles TIM1 break, update, trigger and commutation interrupts.
  */
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_BRK_UP_TRG_COM_IRQn 0 */

  /* USER CODE END TIM1_BRK_UP_TRG_COM_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_BRK_UP_TRG_COM_IRQn 1 */

  /* USER CODE END TIM1_BRK_UP_TRG_COM_IRQn 1 */
}

/**
  * @brief This function handles TIM16 global interrupt.
  */
void TIM16_IRQHandler(void)
{
  /* USER CODE BEGIN TIM16_IRQn 0 */

	RESET_PRESSED =  test_for_reset_press();
	
  /* USER CODE END TIM16_IRQn 0 */
  HAL_TIM_IRQHandler(&htim16);
  /* USER CODE BEGIN TIM16_IRQn 1 */

  /* USER CODE END TIM16_IRQn 1 */
}


/**
  * @brief This function handles USART1 global interrupt / USART1 wake-up interrupt through EXTI line 25.
  */
void USARTx_DMA_RX_IRQHandler(void)
{
	HAL_DMA_IRQHandler(huart1.hdmatx);
	HAL_DMA_IRQHandler(huart1.hdmarx);
}

void USART1_IRQHandler(void)
{
	//HAL_GPIO_TogglePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin);
	HAL_GPIO_TogglePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin);
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}
void TIM14_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&htim14);
}


/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
