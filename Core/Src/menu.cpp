/**
  ******************************************************************************
  * @file    menu.cpp
  * @author  MCD Application Team
  * @version 1.0.0
  * @date    8-April-2015
  * @brief   This file provides the software which contains the main menu routine.
  *          The main menu gives the options of:
  *             - downloading a new binary file, 
  *             - uploading internal flash memory,
  *             - executing the binary file already loaded 
  *             - configuring the write protection of the Flash sectors where the 
  *               user loads his binary file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/** @addtogroup STM32L0xx_IAP
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "common.h"
#include "flash_if.h"
#include "menu.h"
#include <xmodem.h>
#include <string.h>
#include "hardware.h"
#include "usart.h"
#include "mem_controller.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction JumpToApplication;
uint32_t JumpAddress;
uint32_t FlashProtection = 0;
extern uint8_t aFileName[FILE_NAME_LENGTH];
extern Mem_ctrl* memory_extern;
/* Private function prototypes -----------------------------------------------*/
void dump_mem();


/* Private functions ---------------------------------------------------------*/

void dump_mem()
{
	static char dump_c[20];
	for (int i = 0; i < APP_SIZE; i++)
	{
		if (i % 16 == 0 && i > 0)
		{
			sprintf(dump_c, " 0x%02x \r\n", *(volatile uint8_t*)(APP_ADDRESS+i));
		}
		else
		{
			sprintf(dump_c, " 0x%02x", *(volatile uint8_t*)(APP_ADDRESS + i));
		}
		Serial_PutString(dump_c);
	}
}

/**
  * @brief  Display the Main Menu on HyperTerminal
  * @param  None
  * @retval None
  */
void Main_Menu(void)
{
	uint8_t key = 0;

	Serial_PutString("\r\n======================================================================");
	Serial_PutString("\r\n=              (C) COPYRIGHT 2019 ARMenta                            =");
	Serial_PutString("\r\n=                                                                    =");
	Serial_PutString("\r\n=  STM32F0xx In-Application Programming Application  (Version 1.0.0) =");
	Serial_PutString("\r\n=                                                                    =");
	Serial_PutString("\r\n======================================================================");
	Serial_PutString("\r\n\r\n");


	while (1)
	{
		Serial_PutString("\r\n=================== Main Menu ============================\r\n\n");
		Serial_PutString("  Download image to the internal Flash ----------------- 1\r\n\n");
		Serial_PutString("  Download the image of the internal Flash ------------- 2\r\n\n");
		Serial_PutString("  Execute the loaded application ----------------------- 3\r\n\n");
		Serial_PutString("==========================================================\r\n\n");

		/* Clean the input path */
		//__HAL_UART_FLUSH_DRREGISTER(SERIAL_PC);
		//__HAL_UART_CLEAR_IT(SERIAL_PC, UART_CLEAR_OREF);

		/* Receive key */
		HAL_UART_Receive(SERIAL_PC, &key, 1, RX_TIMEOUT);

		switch (key)
		{
		case '1':
			/* Download user application in the Flash */
			xmodem_receive();
			break;
		case '2':
			dump_mem();
			break;
		case '3':
			Serial_PutString("Start program execution......\r\n\n");
			NVIC_SystemReset();
			break;
		default:
			Serial_PutString("Invalid Number ! ==> The number should be either 1 or 2\r\n");
			break;
		}
	}
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
