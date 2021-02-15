/**
 * @file    xmodem.cpp
 * @author  Ferenc Nemeth
 * @date    21 Dec 2018
 * @brief   This module is the implementation of the Xmodem protocol.
 *
 *          Copyright (c) 2018 Ferenc Nemeth - https://github.com/ferenc-nemeth
 */

#include "xmodem.h"
#include <stdint.h>
#include "common.h"
#include <string.h>
#include "hardware.h"
#include "usart.h"
#include "mem_controller.h"
/* Global variables. */
static uint8_t xmodem_packet_number = 1u; /**< Packet number counter. */
static uint32_t xmodem_actual_flash_address = 0u; /**< Address where we have to write. */
static uint8_t x_first_packet_received = false; /**< First packet or not. */
extern uint8_t aPacketData[PACKET_1K_SIZE + PACKET_DATA_INDEX + PACKET_TRAILER_SIZE];
extern Mem_ctrl* memory_extern;
/* Local functions. */
static xmodem_status xmodem_handle_packet(uint8_t size);
static xmodem_status xmodem_error_handler(uint8_t* error_number, uint8_t max_error_number);


/**
 * @brief   This function is the base of the Xmodem protocol.
 *          When we receive a header from UART, it decides what action it shall take.
 * @param   void
 * @return  void
 */
void xmodem_receive(void)
{
	volatile xmodem_status status = X_OK;
	uint8_t error_number = 0u;

	x_first_packet_received = false;
	xmodem_packet_number = 1u;
	xmodem_actual_flash_address = APPLICATION_ADDRESS;

	/* Loop until there isn't any error (or until we jump to the user application). */
	while (X_OK == status)
	{
		uint8_t header = 0x00u;

		/* Get the header from UART. */
		HAL_StatusTypeDef comm_status = HAL_UART_Receive(SERIAL_PC, &header, 1u, 100);

		/* Spam the host (until we receive something) with ACSII "C", to notify it, we want to use CRC-16. */
		if ((HAL_OK != comm_status) && (false == x_first_packet_received))
		{
			(void)Serial_PutByte(X_C);
		}
			/* Uart timeout or any other errors. */
		else if ((HAL_OK != comm_status) && (true == x_first_packet_received))
		{
			status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
		}
		else
		{
			/* Do nothing. */
		}

		xmodem_status packet_status = X_ERROR;
		packet_status = xmodem_handle_packet(header);
		/* The header can be: SOH, STX, EOT and CAN. */

		switch (header)
		{
			/* 128 or 1024 bytes of data. */
		case X_SOH:
			/* If the handling was successful, then send an ACK. */
			if (X_OK == packet_status)
			{
				(void)Serial_PutByte(X_ACK);
			}
			/* If the error was flash related, then immediately set the error counter to max (graceful abort). */
			else if(X_ERROR_FLASH == packet_status)
			{
				error_number = X_MAX_ERRORS;
				status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
			}
			/* Error while processing the packet, either send a NAK or do graceful abort. */
			else
			{
				status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
			}
			break;
		case X_STX:
			/* If the handling was successful, then send an ACK. */
			if (X_OK == packet_status)
			{
				(void)Serial_PutByte(X_ACK);
			}
				/* If the error was flash related, then immediately set the error counter to max (graceful abort). */
			else if (X_ERROR_FLASH == packet_status)
			{
				error_number = X_MAX_ERRORS;
				status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
			}
				/* Error while processing the packet, either send a NAK or do graceful abort. */
			else
			{
				status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
			}
			break;
			/* End of Transmission. */
		case X_EOT:
			/* ACK, feedback to user (as a text), then jump to user application. */
			Serial_PutByte(X_ACK);
			Serial_PutString("\n\rFirmware updated!\n\r");

			// Write out size of app
			memory_extern->write(1, 0xfff04, xmodem_actual_flash_address);
			sprintf((char*)aPacketData, "Write Out: %ld\r\n", xmodem_actual_flash_address);
			Serial_PutString((char*)aPacketData);
			// Write out version of app
			sprintf((char*)aPacketData, "VERSION: %s\r\n", VERSION);
			Serial_PutString((char*)aPacketData);
			
			memory_extern->write(1, 0xfff08, MCU_VERSION_MAJOR);
			memory_extern->write(1, 0xfff0c, MCU_VERSION_MINOR);
			memory_extern->write(1, 0xfff10, MCU_VERSION_PATCH);
			memory_extern->write(1, 0xfff14, MCU_VERSION_RC);
			
			Serial_PutString("Jumping to user application...\n\r");
			HAL_Delay(2500);
			NVIC_SystemReset();
			break;
			/* Abort from host. */
		case X_CAN:
			status = X_ERROR;
			break;
		default:
			/* Wrong header. */
			if (HAL_OK == comm_status)
			{
				status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
			}
			break;
		}
	}
}

/**
 * @brief   Calculates the CRC-16 for the input package.
 * @param   *data:  Array of the data which we want to calculate.
 * @param   length: Size of the data, either 128 or 1024 bytes.
 * @return  status: The calculated CRC.
 */
uint16_t xmodem_calc_crc(uint8_t* data, uint16_t length)
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

/**
 * @brief   This function handles the data packet we get from the xmodem protocol.
 * @param   header: SOH or STX.
 * @return  status: Report about the packet.
 */
static xmodem_status xmodem_handle_packet(uint8_t header)
{
	xmodem_status status = X_OK;
	uint16_t size = 0u;
	if (X_SOH == header)
	{
		size = X_PACKET_128_SIZE;
	}
	else if (X_STX == header)
	{
		size = X_PACKET_1024_SIZE;
	}
	else if (0x00 == header || 0xff == header)
	{
		return X_OK;
	}
	else
	{
		/* Wrong header type. */
		status = X_ERROR;
	}
	uint16_t length = size + X_PACKET_DATA_INDEX + X_PACKET_CRC_SIZE;

	/* Get the packet (except for the header) from UART. */
	HAL_StatusTypeDef comm_status = HAL_OK;
	for (unsigned int j = 0; j < length; j++)
	{
		comm_status = HAL_UART_Receive(SERIAL_PC, &aPacketData[j], 1u, 1000);
	}
	/*
	for (unsigned int j = 0; j < length; j++)
	{
		HAL_UART_Transmit(SERIAL_PC, &aPacketData[j], 1u, 999);
	}
	*/
	/* The last two bytes are the CRC from the host. */
	uint16_t crc_received = ((uint16_t)aPacketData[size + 2u] << 8u) | ((uint16_t)aPacketData[size + 3u]);
	/* We calculate it too. */
	uint16_t crc_calculated = xmodem_calc_crc(&aPacketData[X_PACKET_DATA_INDEX], size);


	/* Error handling and flashing. */
	if (X_OK == status)
	{
		if (HAL_OK != comm_status)
		{
			/* UART error. */
			status = X_ERROR_UART;
		}
		if (xmodem_packet_number != aPacketData[X_PACKET_NUMBER_INDEX])
		{
			/* Packet number counter mismatch. */
			status = X_ERROR_NUMBER;
		}
		if (255u != (aPacketData[X_PACKET_NUMBER_INDEX] + aPacketData[X_PACKET_NUMBER_COMPLEMENT_INDEX]))
		{
			/* The sum of the packet number and packet number complement aren't 255. */
			/* The sum always has to be 255. */
			status = X_ERROR_NUMBER;
		}
		if (crc_calculated != crc_received)
		{
			/* The calculated and received CRC are different. */
			status = X_ERROR_CRC;
		}
		/* Do the actual flashing. */
		if (HAL_OK != FLASH_If_Write(xmodem_actual_flash_address, &aPacketData[2],(uint32_t)size / 4u))
		{
			/* Flashing error. */
			status = X_ERROR_FLASH;
		}
	}

	/* Raise the packet number and the address counters (if there wasn't any error). */
	if (X_OK == status)
	{
		xmodem_packet_number++;
		xmodem_actual_flash_address += size;
	}

	return status;
}

/**
 * @brief   Handles the xmodem error.
 *          Raises the error counter, then if the number of the errors reached critical, do a graceful abort, otherwise send a NAK.
 * @param   *error_number:    Number of current errors (passed as a pointer).
 * @param   max_error_number: Maximal allowed number of errors.
 * @return  status: X_ERROR in case of too many errors, X_OK otherwise.
 */
static xmodem_status xmodem_error_handler(uint8_t* error_number, uint8_t max_error_number)
{
	xmodem_status status = X_OK;
	/* Raise the error counter. */
	(*error_number)++;
	/* If the counter reached the max value, then abort. */
	if ((*error_number) >= max_error_number)
	{
		/* Graceful abort. */
		(void)Serial_PutByte(X_CAN);
		(void)Serial_PutByte(X_CAN);
		status = X_ERROR;
	}
		/* Otherwise send a NAK for a repeat. */
	else
	{
		(void)Serial_PutByte(X_NAK);
		status = X_OK;
	}
	return status;
}
