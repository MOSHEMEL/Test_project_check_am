#include "status_reg.h"
#include "main.h"
#include "usart.h"
#include <string.h>

#include "common.h"
#include "hardware.h"
#include "spi.h"


STATUS_reg::STATUS_reg()
{
	
}

STATUS_reg::~STATUS_reg()
{
	
}

uint8_t STATUS_reg::get_value(void)
{
	uint8_t temp[1];
	HAL_SPI_Receive(&hspi1, temp, 1, 10);
	this->current.value = temp[0];
	//print_current();
	//this->current.nibbles.nibble_OPTO = (this->nibble_reverse(this->current.nibbles.nibble_OPTO));
	//print_current();
	return this->current.value;
}
uint8_t STATUS_reg::nibble_reverse(uint8_t val)
{
	return ((val & 0x08) >>	3) +
		   ((val & 0x04) >> 1) +
		   ((val & 0x02) << 1) +
		   ((val & 0x01) << 3);
}

void STATUS_reg::print_current(void)
{
	static char status[100];
	sprintf(status,
		"[%d] nibble_OPTO \r\n"\
		"[%d] SWITCHES_STATUS \r\n"\
		"[%d] OPTO_STATUS \r\n"\
		"[%d] FIRE_STATUS \r\n"\
		"%02X VALUE \r\n",
		this->current.nibbles.nibble_OPTO,
		this->current.bits.SWITCHES_STATUS,
		this->current.bits.OPTO_STATUS,
		this->current.bits.FIRE_STATUS,
		this->current.value);
	Serial_PutString(status);
}
