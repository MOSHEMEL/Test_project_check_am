#include "cs_register.h"
#include "spi.h"
#include "main.h"
#include "usart.h"
#include <string.h>

#include "common.h"
#include "hardware.h"
#include "xmodem.h"
extern uint8_t aPacketData[PACKET_1K_SIZE + PACKET_DATA_INDEX + PACKET_TRAILER_SIZE];

CS_reg::CS_reg()  // NOLINT(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
{
	this->readback_register.value = 0;
	this->cs1_value = false;
	this->cs2_value = false;
	this->cs3_value = false;
	this->cs_a2d_value = false;
	this->cs_clear_opto_value = false;
	this->cs_status_value = false;
	this->cs_ENA_value = false;
	this->cs_ENB_value = false;
	this->current.value = 0;
}

CS_reg::~CS_reg()
{
}

void CS_reg::set_value(uint8_t value)
{
	// CS is low active so we reset pin and then set to HIGH
	this->pull_chipselet_line(HIGH);
	HAL_SPI_Transmit(&hspi1, &value, 1, 10);
	this->current.value = value;
	this->pull_chipselet_line(LOW);
	delay(100);
	this->store_register();
	this->pull_chipselet_line(HIGH);
}

void CS_reg::cs1(GPIO_PinState status)
{
	if (!bool(status))
	{

		this->current.bits.DECODER = Y0;
		HAL_StatusTypeDef status_HAL = HAL_SPI_Transmit(&hspi1, &(this->current.value), 1, 1);
		while (status_HAL != HAL_OK)
		{
			if (status_HAL == HAL_TIMEOUT)
			{
				HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_HIGH);
			}
			if (status_HAL == HAL_ERROR)
			{
				HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_HIGH);
				//Serial_PutString("HAL ERROR IN CS\r\n");
			}
			if (status_HAL == HAL_BUSY)
			{
				HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_HIGH);
				HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_HIGH);
			}
			status_HAL = HAL_SPI_Transmit(&hspi1, &(this->current.value), 1, 1);
		}

		HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_LOW);
		HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_LOW);

		//If we need to set high - we want to disable all CS_es
		this->pull_chipselet_line(HIGH);
	}


	HAL_GPIO_WritePin(CHIP_SELECT_MCU_MEM_GPIO_Port, CHIP_SELECT_MCU_MEM_Pin, status);
	this->cs1_value = !(bool)status;
}

void CS_reg::cs_generic(GPIO_PinState status,enum cs_decoder picked_cs)
{
	/***********************************************************************
	 * IMPORTANT: The usual "Hardware logic" is chipselect is active on low.
	 * But since we have a special case where the cs_register is reset to 0
	 * We have inverted all cs inputs to the hardware from cs_register
	 * so what needs to be done is that the register written logical 1
	 * to get an electrical VLow
	 ***********************************************************************/
	if (bool(status))
	{
		//If we need to set high - we want to disable all CS_es
		this->pull_chipselet_line(HIGH);
	}
	else
	{
		/*
		 * Maybe should add check that the selected line was not already set as cs. That way we will not have to do additional data transfer.
		 */
		this->current.bits.DECODER = picked_cs;
		this->current.bits.INB_MOTOR = 0;
		HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, &(this->current.value), 1, 1);
		while (status != HAL_OK)
		{
			if (status == HAL_TIMEOUT)
			{
				HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_HIGH);
			}
			if (status == HAL_ERROR)
			{
				HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_HIGH);
				//Serial_PutString("HAL ERROR IN CS\r\n");
			}
			if (status == HAL_BUSY)
			{
				HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_HIGH);
				HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_HIGH);
			}
			status = HAL_SPI_Transmit(&hspi1, &(this->current.value), 1, 1);
		}

		HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_LOW);
		HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_LOW);
		this->pull_chipselet_line(LOW);
		this->store_register();
		static int cs_line_select = 0;

		
		cs_line_select++;
		if (cs_line_select % 31 == 0)
		{
			//this->print_current_new();
			if (false)
			{
				
			
			this->read_back_current_cs();
			this->print_current();
			while (this->readback_register.bits.INA_MOTOR != this->current.bits.INA_MOTOR || this->readback_register.bits.INB_MOTOR != this->current.bits.INB_MOTOR)
			{
				this->pull_chipselet_line(HIGH);
				HAL_SPI_Transmit(&hspi1, &(this->current.value), 1, 1);
				this->pull_chipselet_line(LOW);
				this->read_back_current_cs();
				this->print_current();
			}
			// and then set to our expected value
			this->pull_chipselet_line(HIGH);
			HAL_SPI_Transmit(&hspi1, &(this->current.value), 1, 1);
			this->pull_chipselet_line(LOW);
		}
		}
	}
}
void CS_reg::cs2(GPIO_PinState status)
{
	this->cs2_value = !(bool)status;
	enum cs_decoder picked_cs = CS_MEM2;
	cs_generic(status, picked_cs);
}

void CS_reg::cs3(GPIO_PinState status)
{
	this->cs3_value = !(bool)status;
	enum cs_decoder picked_cs = CS_MEM3;
	cs_generic(status, picked_cs);
}

void CS_reg::cs_status(GPIO_PinState status)
{
	this->cs_status_value = !(bool)status;
	enum cs_decoder picked_cs = CS_STATUS;
	cs_generic(status, picked_cs);
}

void CS_reg::cs_a2d(GPIO_PinState status)
{
	this->cs_a2d_value = !(bool)status;
	enum cs_decoder picked_cs = CS_A2D;
	uint8_t null_byte = 0;
	HAL_SPI_Transmit(&hspi1, &null_byte, 1, 1);

	cs_generic(status, picked_cs);
}

void CS_reg::cs_clear_opto_counter(GPIO_PinState status)
{
	this->cs_clear_opto_value = !(bool)status;
	enum cs_decoder picked_cs = OPTO_CNT_CLRN;
	cs_generic(status, picked_cs);
}

void CS_reg::print_current()
{
	uint8_t* status = aPacketData;
	sprintf((char*)status,
		"[%d] INB_MOTOR\r\n"\
		"[%d] INA_MOTOR\r\n"\
		"[%08x] DECODER\r\n",
		this->current.bits.INB_MOTOR,
		this->current.bits.INA_MOTOR,
		this->current.bits.DECODER);
	Serial_PutString((char*)status);
}

void CS_reg::print_current_new()
{
	uint8_t* status = aPacketData;
	sprintf((char*)status,
		"[%02X] value\r\n"\
		"[%d] CS1\r\n"\
		"[%d] CS2\r\n"\
		"[%d] CS3\r\n"\
		"[%d] CS A2D\r\n"\
		"[%d] CS opto clear\r\n"\
		"[%d] CS status\r\n"\
		"[%d] INA_MOTOR\r\n"\
		"[%d] INB_MOTOR\r\n",
		this->current.value,
		this->cs1_value,
		this->cs2_value,
		this->cs3_value,
		this->cs_a2d_value,
		this->cs_clear_opto_value,
		this->cs_status_value,
		this->cs_ENA_value, 
		this->cs_ENB_value);
	Serial_PutString((char*)status);
}


void CS_reg::pull_chipselet_line(GPIO_PinState status)
{
	/*
	 * Toggling the CS LINE LOW is what triggers the save to the cs_register, otherwise we just randomly push data on the line
	 */
	HAL_GPIO_WritePin(CHIP_SELECT_SERIAL_GPIO_Port, CHIP_SELECT_SERIAL_Pin, status);
}

void CS_reg::store_register()
{
	/*
	 * Store register is toggled on LVDS - low.
	 * When we want to read back, we toggle CS (disable to all CS except serial in-parallel out
	 * Then we toggle LVDS and latch up the value
	 * on cs_read_back we don't toggle lvds, so we store the last value
	 */
	HAL_GPIO_WritePin(CHIP_SELECT_RESERVED_GPIO_Port, CHIP_SELECT_RESERVED_Pin, GPIO_PIN_SET);
	delay(20);
	//HAL_GPIO_WritePin(CHIP_SELECT_RESERVED_GPIO_Port, CHIP_SELECT_RESERVED_Pin, GPIO_PIN_RESET);
}
void CS_reg::read_back_current_cs()
{
	uint8_t before_read_back = this->current.value;
	enum cs_decoder picked_cs = CS_READBACK_SER;
	this->current.bits.DECODER = picked_cs;
	HAL_SPI_Transmit(&hspi1, &this->current.value, 1, 10);
	this->pull_chipselet_line(LOW);
	HAL_SPI_Receive(&hspi1, &this->readback_register.value, 1, 10);
	this->pull_chipselet_line(HIGH);
	char* readback = (char*)aPacketData;

	/*
		uint8_t INB_MOTOR : 1;
		uint8_t INA_MOTOR : 1;
		uint8_t DECODER : 3;
		uint8_t CS_STATUS_INV : 1;
		uint8_t CS_MEM3 : 1;
		uint8_t CS_MEM2 : 1;
	 */
	sprintf(readback, "RDBK: %02x\r\n"\
					"[%d] INB_MOTOR\r\n"\
					"[%d] INA_MOTOR\r\n"\
					"[%08x] DECODER\r\n",
					this->readback_register.value,
					this->readback_register.bits.INB_MOTOR,
					this->readback_register.bits.INA_MOTOR,
					this->readback_register.bits.DECODER);
	Serial_PutString(readback);
	this->set_value(before_read_back);
}

void CS_reg::motor_cmd(bool state)
{
	if (state)
	{
		this->current.bits.INA_MOTOR = 1;
		this->current.bits.INB_MOTOR = 0;
		this->cs_ENA_value = true;
		this->cs_ENB_value = false;
	}
	else
	{
		this->current.bits.INA_MOTOR = 0;
		this->current.bits.INB_MOTOR = 0;
		this->cs_ENA_value = false;
		this->cs_ENB_value = false;
	}
	uint8_t value = current.value;
	this->set_value(value);
}


void CS_reg::motor_cmd_tight(volatile bool state)
{
	if (state)
	{
		this->current.bits.INA_MOTOR = 1;
		this->current.bits.INB_MOTOR = 0;
		this->cs_ENA_value = true;
		this->cs_ENB_value = false;
	}
	else
	{
		this->current.bits.INA_MOTOR = 0;
		this->current.bits.INB_MOTOR = 0;
		this->cs_ENA_value = false;
		this->cs_ENB_value = false;
	}
	this->pull_chipselet_line(HIGH);
	HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, &(this->current.value), 1, 1);
	while (status != HAL_OK)
	{
		if (status == HAL_TIMEOUT)
		{
			HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_HIGH);
		}
		if (status == HAL_ERROR)
		{
			HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_HIGH);
			//Serial_PutString("HAL ERROR IN CS\r\n");
		}
		if (status == HAL_BUSY)
		{
			HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_HIGH);
			HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_HIGH);
		}
		status = HAL_SPI_Transmit(&hspi1, &(this->current.value), 1, 1);
	}
	this->pull_chipselet_line(LOW);
	delay(10);
	this->pull_chipselet_line(HIGH);
}

void CS_reg::motor_cmd_reverse(void)
{
	this->cs_ENA_value = false;
	this->cs_ENB_value = true;
	this->current.bits.INA_MOTOR = 0;
	this->current.bits.INB_MOTOR = 1;

	this->pull_chipselet_line(HIGH);
	HAL_SPI_Transmit(&hspi1, &this->current.value, 1, 0);
	this->pull_chipselet_line(LOW);
	HAL_Delay(100);
	this->pull_chipselet_line(HIGH);
}
