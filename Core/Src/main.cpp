/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.cpp
  * @brief          : Main program body
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
/**
  ******************************************************************************
						CHANGELOG + NOTES
  ******************************************************************************
  *
  * 05.05.2020:
  * 1. First alpha is tested on live electronics - we have all parts moving
  *
  * Thoughts about possible QA issues:
  * 1. Serial buffers - We should restrict usage of buffers and reuse some
  *		Global buffers that are not being used ( like the xmodem one)
  *	2. Delays (where necessary) remove or use interrupt based routines instead
  *	3. General QA - use common.h
  *	-- so far DONE
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "crc.h"
#include "dma.h"
#include "rtc.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "cs_register.h"
#include "status_reg.h"
#include "mem_controller.h"
#include "screen_controller.h"
#include "monitor.h"
#include "state.h"
#include "debug_facilities.h"
#include "peripherial_controller.h"
#include "adc_controller.h"
#include "common.h"
#include "main_controller.h"
#include "tim.h"
#include "xmodem.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
bool Start_Test = 0; 
static uint8_t Start_Test_Am = 0;
__IO ITStatus UartReady;
extern char   aTxBuffer[];
char aRxBuffer[4];
uint16_t num_beeps_need_play = 0;
uint16_t buzz_time1 = 0;
uint16_t buzz_time2 = 0;
bool toggle_buzz = 1;
DebugHelper debug = DebugHelper();
CS_reg cs_register = CS_reg();
STATUS_reg status_register = STATUS_reg();
Memory memory_am = Memory(3);
Memory memory_apt = Memory(2);
Memory memory_mcu = Memory(1);
Mem_ctrl memory_controller = Mem_ctrl(&memory_am, &memory_apt, &memory_mcu, &cs_register);
PeripherialDevices ph_device = PeripherialDevices(&cs_register, &status_register);
State state = State();
Screen_ctrl screen = Screen_ctrl();
adc_controller c_adc_controller = adc_controller(&cs_register);
uint32_t cycle = 0;
main_controller grand_ctrl = main_controller(&debug, &memory_controller, &ph_device, &state, &screen, &c_adc_controller);
monitor monitor_controller = monitor(&grand_ctrl);
main_controller* atp_extern = &grand_ctrl;
Mem_ctrl* memory_extern = &memory_controller;
uint8_t aPacketData[PACKET_1K_SIZE + PACKET_DATA_INDEX + PACKET_TRAILER_SIZE];
uint8_t BUFFER_RX[RXBUFFERSIZE];
uint8_t BUFFER_SCREEN[RXBUFFERSIZE];
uint8_t BUFFER_SCREEN_WITH_CRC[RXBUFFERSIZE];
volatile bool RESET_PRESSED = false;
uint32_t timer_last_update;
uint32_t timer_last_update_diff;
volatile bool WATCHDOG_IS_ENABLED = true;
volatile uint8_t COUNTER_V = 2;
bool reset_button_instead_of_fire = false;
struct PF_memory pf_memory;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
void remapMemToSRAM(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	c_adc_controller.current_sense  = c_adc_controller.dma_ring_buffer[0];
	c_adc_controller.temperature_mcu  = c_adc_controller.dma_ring_buffer[1];
	c_adc_controller.vref  = c_adc_controller.dma_ring_buffer[2];
	c_adc_controller.vbat  = c_adc_controller.dma_ring_buffer[3];
}

#define __HAL_SYSCFG_REMAPMEMORY_SRAM() \
 do {SYSCFG->CFGR1 &= ~(SYSCFG_CFGR1_MEM_MODE); \
     SYSCFG->CFGR1 |= (SYSCFG_CFGR1_MEM_MODE_0 | SYSCFG_CFGR1_MEM_MODE_1); \
 }while(0) 

 
/**Force VectorTable to specific memory position defined in linker*/
volatile uint32_t VectorTable[48] __attribute__((section(".RAMVectorTable")));
 
//copy the vector table to SRAM
void remapMemToSRAM(void)
{
	uint32_t vecIndex = 0;
	uint32_t prim;
	prim = __get_PRIMASK();

	__disable_irq();
 
	for (vecIndex = 0; vecIndex < 48; vecIndex++) {
		VectorTable[vecIndex] = *(volatile uint32_t*)(APP_ADDRESS + (vecIndex << 2));
	}

	//__HAL_RCC_APB2_FORCE_RESET();
	__HAL_SYSCFG_REMAPMEMORY_SRAM();
	
	if (!prim) {
		__enable_irq();
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	#if FROM_BOOTLOADER
	remapMemToSRAM();
	#endif

  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
 // MX_ADC_Init();
 // MX_RTC_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  //MX_CRC_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	MX_TIM16_Init();
	MX_TIM2_Init();
	MX_TIM3_Init();
  //MX_TIM14_Init();
	//delay_with_watchdog(2000);
	grand_ctrl.init_before_while();
	timer_last_update = HAL_GetTick();
	
	HAL_TIM_Base_Start_IT(&htim16);
	HAL_TIM_Base_Start(&htim2);
	HAL_TIM_Base_Start(&htim3);
	
	//debug.fast_beeps_3();
	
	//grand_ctrl.get_device()->stop_motor_init();
	//grand_ctrl.get_device()->read();
	//grand_ctrl.get_device()->read();
	//grand_ctrl.get_screen()->update_pulse_counter(0);
	
	char state_enter[20];
	//Serial_PutString(state_enter);
	sprintf(state_enter, "S: %d\r\n", (int)state.current);
	Serial_PutString(state_enter);
	HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);

	// INIT IN CASE OF POWER FAIL
//	grand_ctrl.init_PF_struct(&pf_memory);
	//
	delay_with_watchdog(1000);
	HAL_GPIO_WritePin(CHIP_SELECT_SERIAL_GPIO_Port, CHIP_SELECT_SERIAL_Pin, GPIO_PIN_SET);
	//grand_ctrl.show_version();
	//grand_ctrl.get_mem()->erase(3);
	//HAL_Delay(20);
	
	//grand_ctrl.get_mem()->write(3, 0, 12);
	//grand_ctrl.get_mem()->read(3, 0);
  /* USER CODE END 2 */
     
   
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while(true)
  {
  	/***********************************************************************************
  	 *26.03.20: alpha release 1:
  	 *changelog:
  	 *So far we have all peripherials working together to create the basic flow
  	 *we can read all memories, query adc, and get status.
  	 *29.03.20 alphaR2:
  	 *add a main class to abstract the different classes
  	 *add qa reduced functionality
  	 *add timer on error messages for tx_send_general error
  	 *30.03.20:
  	 *add xmodem update/upload
  	 *add rx_routines class to instrument functions from uart
  	 *01/04/20:
  	 *add TOGGLE_RESET_BUTTON read
  	 *add stalling procedures
  	 *FIX:
  	 *adc design
  	 *read back cs register function:
  	 *	1. change CS_REG status
  	 *	2. change STATUS_REG
  	 *When cs_low at cs_register, devices online will be listening to the cs_register setup commands.
  	 *
  	 *20.05.20:
  	 *IMPORTANT NOTE!
  	 *some splitters will not have the reset hardware available / or otherwise will be faulty
  	 *If the INA is not powered fully every cycle.
  	 *So a function to ~Reinforce~ the cs register must be added
	************************************************************************************/
	 // state.current = AM_TEST_COMMAND;
	  switch (state.current)
		{
		case Init:
			grand_ctrl.init_state();
			break;
		case Idle:
			grand_ctrl.idle_state();
			break;
		case UpdateData:
			grand_ctrl.update_state();
			break;
		case Active:
			grand_ctrl.active_state();
			break;
		case UartDebug:
			while (state.current == UartDebug)
			{
			//	grand_ctrl.read_interlock();
			//	
			//	monitor_controller.serial_consume();
				while (1)
				{
					Start_Test = test_for_reset_press_aux();
					if (Start_Test == 1)
					{
						monitor_controller.test_memo();
						Start_Test = 0;
						//while (1) ;
					}
					//delay_with_watchdog(20);
				}
				if (WATCHDOG_IS_ENABLED)
				{
					HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
				}
				cycle++;
				if (cycle % 100 == 0)
				{
				//	
					HAL_GPIO_TogglePin(BLINKING_LED_GPIO_Port, BLINKING_LED_Pin);
					monitor_controller.rx_adc();
					grand_ctrl.update_debug();
					grand_ctrl.get_screen()->update_version();
					grand_ctrl.get_mem()->check_connections();
				}
				timer_last_update_diff = HAL_GetTick() - timer_last_update;
				timer_last_update = HAL_GetTick();
//				if ((cycle % 100 == 0) && (Start_Test_Am==0))
//				{
//					Start_Test_Am = 0;
//					monitor_controller.test_memo();
//				}
				
			}
			break;
		case QA:
			grand_ctrl.qa_state();
			break;
		case TEST:
			{  
				grand_ctrl.Test_Status_Machine();
			  
			  
				break;  
			}

		default :
			{
		  
			}
			break; 
		}
	  

	  if (state.current != Init && state.current != QA && state.current != UpdateData)
	  {
		  if (HAL_GetTick() - timer_last_update > TICKS_FOR_UPDATE)
		  {
			  // See how we make an update without disabling Active State
			  state.current = UpdateData;
			  timer_last_update = HAL_GetTick();
		  }
	  }
	  else
	  {
		  state.current = state.next;
	  }
	  if (WATCHDOG_IS_ENABLED)
	  {
		  HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
	  }
	  cycle++;
	  grand_ctrl.read_interlock();

	  if (cycle % 100 == 0)
	  {
		  HAL_GPIO_TogglePin(BLINKING_LED_GPIO_Port, BLINKING_LED_Pin);
		  //
		  grand_ctrl.update_debug();
	  }
	  //HAL_GPIO_TogglePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin);
      monitor_controller.parse_input_uart();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure LSE Drive Capability 
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI14|RCC_OSCILLATORTYPE_HSI48
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
	  Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART2
                              |RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_SYSCLK;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_1);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	static uint16_t num_of_beeps = 0;
	/* USER CODE BEGIN Callback 0 */

	/* USER CODE END Callback 0 */
	if (htim->Instance == TIM1) {
		HAL_IncTick();
	}
	if (htim->Instance == TIM14) {
		if (num_of_beeps < 2*num_beeps_need_play)
		{
			num_of_beeps++;
			HAL_GPIO_TogglePin(BUZZER_GPIO_Port, BUZZER_Pin);
			//HAL_GPIO_TogglePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin);
			if (toggle_buzz)
			{  
				HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin,GPIO_PIN_SET);
				TIM14->ARR = buzz_time1;
				toggle_buzz = 0;
			}
			else 
			{
				HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
				toggle_buzz = 1;
				TIM14->ARR = buzz_time2;  
			}
		}
		else
		{
			num_of_beeps = 0;
			HAL_TIM_Base_Stop_IT(&htim14);
			//HAL_NVIC_DisableIRQ(TIM14_IRQn); 
		}
		/* USER CODE BEGIN Callback 1 */

		/* USER CODE END Callback 1 */
	}
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	/* Set transmission flag: transfer complete */
	
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{  

//	if (aRxBuffer[1] == 't' || aRxBuffer[2] == 'e' || aRxBuffer[3] == 's' || aRxBuffer[4] == 't')
//   {
//	   flag_uart_test = 1;
//	}
}
	void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_1_Pin, (GPIO_PinState)false);
	HAL_GPIO_WritePin(DEBUG_STATE_2_GPIO_Port, DEBUG_STATE_2_Pin, (GPIO_PinState)false);
	HAL_GPIO_WritePin(DEBUG_STATE_3_GPIO_Port, DEBUG_STATE_3_Pin, (GPIO_PinState)false);
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
