/**
  ******************************************************************************
  * @file    common.cpp
  * @author  MCD Application Team
  * @version 1.0.0
  * @date    8-April-2015
  * @brief   This file provides all the common functions.
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

/** @addtogroup STM32L0xx_IAP_Main
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "main.h"
#include "hardware.h"
#include "usart.h"
#include "string.h"
#include "tim.h"
#include "stdint.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
extern uint8_t BUFFER_SCREEN_WITH_CRC[RXBUFFERSIZE];

/**
  * @brief  Convert an Integer to a string
  * @param  p_str: The string output pointer
  * @param  intnum: The integer to be converted
  * @retval None
  */
void Int2Str(uint8_t *p_str, uint32_t intnum)
{
  uint32_t i, divider = 1000000000, pos = 0, status = 0;

  for (i = 0; i < 10; i++)
  {
    p_str[pos++] = (intnum / divider) + 48;

    intnum = intnum % divider;
    divider /= 10;
    if ((p_str[pos-1] == '0') & (status == 0))
    {
      pos = 0;
    }
    else
    {
      status++;
    }
  }
}

/**
  * @brief  Convert a string to an integer
  * @param  p_inputstr: The string to be converted
  * @param  p_intnum: The integer value
  * @retval 1: Correct
  *         0: Error
  */
uint32_t Str2Int(uint8_t *p_inputstr, uint32_t *p_intnum)
{
  uint32_t i = 0, res = 0;
  uint32_t val = 0;

  if ((p_inputstr[0] == '0') && ((p_inputstr[1] == 'x') || (p_inputstr[1] == 'X')))
  {
    i = 2;
    while ( ( i < 11 ) && ( p_inputstr[i] != '\0' ) )
    {
      if (ISVALIDHEX(p_inputstr[i]))
      {
        val = (val << 4) + CONVERTHEX(p_inputstr[i]);
      }
      else
      {
        /* Return 0, Invalid input */
        res = 0;
        break;
      }
      i++;
    }

    /* valid result */
    if (p_inputstr[i] == '\0')
    {
      *p_intnum = val;
      res = 1;
    }
  }
  else /* max 10-digit decimal input */
  {
    while ( ( i < 11 ) && ( res != 1 ) )
    {
      if (p_inputstr[i] == '\0')
      {
        *p_intnum = val;
        /* return 1 */
        res = 1;
      }
      else if (((p_inputstr[i] == 'k') || (p_inputstr[i] == 'K')) && (i > 0))
      {
        val = val << 10;
        *p_intnum = val;
        res = 1;
      }
      else if (((p_inputstr[i] == 'm') || (p_inputstr[i] == 'M')) && (i > 0))
      {
        val = val << 20;
        *p_intnum = val;
        res = 1;
      }
      else if (ISVALIDDEC(p_inputstr[i]))
      {
        val = val * 10 + CONVERTDEC(p_inputstr[i]);
      }
      else
      {
        /* return 0, Invalid input */
        res = 0;
        break;
      }
      i++;
    }
  }

  return res;
}


/**
  * @brief  Print a string on the HyperTerminal
  * @param  p_string: The string to be printed
  * @retval None
  */
void Serial_PutString(const char* p_string)
{
	uint16_t length = 0;

	while (p_string[length] != '\0')
	{
		length++;
	}
	HAL_UART_Transmit(SERIAL_PC, (uint8_t*)p_string, length, TX_TIMEOUT);
  //static const char end_line[] = "\r\n";
  //HAL_UART_Transmit(SERIAL_PC, (uint8_t*)end_line, 3, TX_TIMEOUT);
}

/**
  * @brief  Print a string on the Serial bus to monitor
  * @param  p_string: The string to be printed
  * @retval None
  */
void Monitor_PutString(const char* p_string)
{
	uint16_t size = prepare_packet((uint8_t*)p_string, BUFFER_SCREEN_WITH_CRC);
	HAL_UART_Transmit(SERIAL_SCREEN, (uint8_t*)BUFFER_SCREEN_WITH_CRC, size + 4, TX_TIMEOUT);
}

/**
  * @brief  Transmit a byte to the HyperTerminal
  * @param  param The byte to be sent
  * @retval HAL_StatusTypeDef HAL_OK if OK
  */
HAL_StatusTypeDef Serial_PutByte( uint8_t param )
{

	return HAL_UART_Transmit(SERIAL_PC, &param, 1, TX_TIMEOUT);
}

uint32_t prepare_packet(uint8_t *p_source, uint8_t *p_packet)
{
	uint32_t size = 0;
	while (p_source[size] != '\0')
	{
		size++;
	}
	p_packet[0] = 0x01;
	p_packet[1] = size;

	/* Filename packet has valid data */
	for (uint32_t i = 0; i < size; i++)
	{
		p_packet[2 + i] = *p_source++;
	}
	p_packet[2 + size] = 0;
	uint16_t checksum = calc_crc(&p_packet[2], size);
	p_packet[2 + size + 0] = (checksum >> 8) & 0xff;
	p_packet[2 + size + 1] = (checksum >> 0) & 0xff;
	p_packet[2 + size + 2] = 0;

	return size;
}

uint16_t calc_crc(uint8_t* data, int length)
{
	uint16_t crc = 0u;
	while (length)
	{
		length--;
		crc = crc ^ ((uint16_t)*data++ << 8u);
		for (uint8_t i = 0u; i < 8u; i++)
		{
			if (crc & 0x8000u)
			{
				crc = (crc << 1u) ^ 0x1021u;
			}
			else
			{
				crc = crc << 1u;
			}
		}
	}
	return crc;
}

void delay(uint16_t delay_)
{
	uint32_t delay_48mhz_ticks = delay_ * 1;   // PRESCALER IS ADDED
	__HAL_TIM_SET_COUNTER(&htim2, 0);
	{
		while (__HAL_TIM_GET_COUNTER(&htim2) < delay_48mhz_ticks) ;
	}
}

void translate_int_to_address(uint32_t read_address, uint8_t* address_buf)
{
	address_buf[2] = (uint8_t)(read_address & 0xFF);              //A0-A7
	address_buf[1] = (uint8_t)(read_address >> 8 & 0xFF);          //A8-A15
	address_buf[0] = (uint8_t)(read_address >> 16 & 0xFF);         //A16-A23
}

uint32_t translate_address_to_int(uint8_t* spi_read_data)
{
	uint32_t spi_rcv = 0;
	spi_rcv = spi_rcv + ((uint32_t)spi_read_data[3]);
	spi_rcv = spi_rcv + ((uint32_t)spi_read_data[2] << 8);
	spi_rcv = spi_rcv + ((uint32_t)spi_read_data[1] << 16);
	spi_rcv = spi_rcv + ((uint32_t)spi_read_data[0] << 24);
	return spi_rcv;
}

void translate_int_to_arr(uint32_t input, uint8_t* arr)
{
	arr[3] = (uint8_t)(input & 0xFF);                    //A24-A31
	arr[2] = (uint8_t)(input >> 8 & 0xFF);               //A16-A23
	arr[1] = (uint8_t)(input >> 16 & 0xFF);              //A8-A15
	arr[0] = (uint8_t)(input >> 24 & 0xFF);              //A0-A7
}

uint8_t read_reset_button_aux(void)
{
	if (HARDWARE_V == 2)
	{
		return (uint8_t)!HAL_GPIO_ReadPin(TOGGLE400_RESET_BTN_GPIO_Port, TOGGLE400_RESET_BTN_Pin);
	}
	else if (HARDWARE_V == 1)
	{
		return (uint8_t)HAL_GPIO_ReadPin(TOGGLE400_RESET_BTN_GPIO_Port, TOGGLE400_RESET_BTN_Pin);
	}
	return 0;
}

bool test_for_reset_press_aux(void) {
 
	static uint16_t button_history = 0;
	bool pressed = false;    
 
	button_history = button_history << 1;
	button_history |= read_reset_button_aux();
	if ((button_history & 0xF0FF) == 0xFF)
	{ 
		pressed = true;
		button_history = 0xFFFF;
	}
	return pressed;
}

void delay_with_watchdog(uint32_t delay_time)
{
	uint32_t updates_last = HAL_GetTick();
	while (HAL_GetTick() - updates_last < delay_time)
	{
		HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
		delay(250);
	}
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
