#include "mem.h"
#include "usart.h"
#include <string.h>

#include "common.h"
#include "hardware.h"
#include "mem_controller.h"
#include "spi.h"
#include "xmodem.h"
extern uint8_t aPacketData[PACKET_1K_SIZE + PACKET_DATA_INDEX + PACKET_TRAILER_SIZE];

Memory::Memory(uint8_t cs_num)
{
	this->CS = cs_num;   // 1, 2, 3
	if(this->CS == 1)
	{
		sprintf((char*)this->name, "MCU");
	}
	else if(this->CS == 2)
	{
		sprintf((char*)this->name, "APT");
	}
	else if(this->CS == 3)
	{
		sprintf((char*)this->name, "AM");
	}
	this->id[0] = 0;   // valid id would be 0x1f-85-01 or something
	this->id[1] = 0;    // valid id would be 0x1f-85-01 or something
	this->id[2] = 0;    // valid id would be 0x1f-85-01 or something
	this->id_valid = false;  // true if okay
	this->status1.value = 0;
	this->status2.value = 0;
	this->cursor = 0;    // pointer to read memory
	this->pulses = 0;
	this->serial_number = 0;
	this->max_count = 0;
	this->date = 0;
}

Memory::~Memory()
{
	
}

void Memory::print_status(void)
{

	static char message[256];
	sprintf((char*)message,
		"cs[%d] read %08X\r\n"\
						 "SRP0 [%d]\r\n"\
						 "SEC  [%d]\r\n"\
						 "TB   [%d]\r\n"\
						 "BP2  [%d]\r\n"\
						 "BP1  [%d]\r\n"\
						 "BP0  [%d]\r\n"\
						 "WEL  [%d]\r\n"\
						 "WIP  [%d]\r\n"\
						 "\r\n"\
						 "RES  [%d]\r\n"\
						 "CMP  [%d]\r\n"\
						 "LB3  [%d]\r\n"\
						 "LB2  [%d]\r\n"\
						 "LB1  [%d]\r\n"\
						 "RES  [%d]\r\n"\
						 "QE   [%d]\r\n"\
						 "SRP1 [%d]\r\n",
		this->CS,
		(this->status1.value << 8 )+ this->status2.value,
		this->status1.bits.SRP0,
		this->status1.bits.SEC,
		this->status1.bits.TB,
		this->status1.bits.BP2,
		this->status1.bits.BP1,
		this->status1.bits.BP0,
		this->status1.bits.WEL,
		this->status1.bits.WIP,
		this->status2.bits.RES,
		this->status2.bits.CMP,
		this->status2.bits.LB3,
		this->status2.bits.LB2,
		this->status2.bits.LB1,
		this->status2.bits.RES2,
		this->status2.bits.QE,
		this->status2.bits.SRP1);
	Serial_PutString(message);
}

void Memory::uint32_to_address(uint32_t read_address, uint8_t* address_buf)
{	
	translate_int_to_address(read_address, address_buf);
}

uint32_t Memory::read_buffer_to_uint32(uint8_t* spi_read_data)
{	
	return translate_address_to_int(spi_read_data);
}

void Memory::uint32_to_array(uint32_t input, uint8_t* arr)
{
	translate_int_to_arr(input, arr);
}

HAL_StatusTypeDef Memory::erase(uint32_t erase_address)
{
	HAL_StatusTypeDef status = HAL_OK;
	uint8_t erase_command[1];
	erase_command[0] = BLOCK_ERASE_4K;     // erase opcodes: 4Kbytes: 0x20, 32Kbytes: 0x52, 64Kbytes: 0xD8
	status = HAL_SPI_Transmit(&hspi1, erase_command, 1, 5);
	if(status != HAL_OK)
	{
		return status;
	}
	// Address :
	uint8_t address[3];
	this->uint32_to_address(erase_address, address);
	return HAL_SPI_Transmit(&hspi1, address, 3, 5);
}

HAL_StatusTypeDef Memory::erase(void)
{
	uint8_t erase_command[1];
	erase_command[0] = ERASE_CHIP2;        // erase the entire chip
	return HAL_SPI_Transmit(&hspi1, erase_command, 1, 5) ;
}

void Memory::check_id(void)
{   
//	char idarray[40];
	this->get_id();
	//sprintf(idarray, "ID %x,%x,%x,", id[0], id[1], id[2]);
//	Serial_PutString(idarray);
	if ((this->id[0] == 0x1f)&&(this->id[1] == 0x85)&&(this->id[2] == 0x01))
	{
		this->id_valid = true;
	}
	else
		this->id_valid = false;
	}


void Memory::get_id(void)
{   
	//uint8_t status = 0;
	//uint8_t idAM[3];
	//char idarray[40];
	uint8_t spi_data_command[1];
	spi_data_command[0] = READ_MANUFACTURE_ID;      //get device ID 
	HAL_SPI_Transmit(&hspi1, spi_data_command, 1, 5);
	//delay(100);
	HAL_SPI_Receive(&hspi1,this->id, 3, 1);
//	if (status == HAL_OK)
//	{
//		Serial_PutString("hal ok");
//	}
//    sprintf(idarray, "ID %x,%x,%x,", idAM[0], idAM[1], idAM[2]);
//	Serial_PutString(idarray);
}

HAL_StatusTypeDef Memory::write_disable(void)
{
	uint8_t spi_write_disable[1];
	spi_write_disable[0] = WRITE_DISABLE;      // write enable
	return HAL_SPI_Transmit(&hspi1, spi_write_disable, 1, 2);
}

HAL_StatusTypeDef Memory::write_enable(void)
{
	uint8_t spi_write_enable[1];
	spi_write_enable[0] = WRITE_ENABLE;         // write enable
	return HAL_SPI_Transmit(&hspi1, spi_write_enable, 1, 2);
}

HAL_StatusTypeDef Memory::write_enable(uint32_t address)
{
	uint8_t spi_write_enable[4];
	spi_write_enable[0] = WRITE_ENABLE;          // write enable
	spi_write_enable[1] = (uint8_t)((address & 0xff0000) >> 16);            // write enable
	spi_write_enable[2] = (uint8_t)((address & 0xff00) >> 8 );            // write enable
	spi_write_enable[3] = (uint8_t)(0x0);								// write enable
	return HAL_SPI_Transmit(&hspi1, spi_write_enable, 4, 2);
}

void Memory::read_status_register1(void)
{
	/*
		7 SRP0 Status Register Protection bit-0 R/W See Table 10-3 on Status Register Protection
		6 SEC Block Protection R/W See Table 8-1 and 8-2 on Non-Volatile Protection
		5 TB Top or Bottom Protection R/W See Table 8-1 and 8-2 on Non-Volatile Protection
		4 BP2 Block Protection bit-2 R/W See Table 8-1 and 8-2 on Non-Volatile Protection
		3 BP1 Block Protection bit-1 R/W See Table 8-1 and 8-2 on Non-Volatile Protection
		2 BP0 Block Protection bit-0 R/W See Table 8-1 and 8-2 on Non-Volatile Protection
		1 WEL Write Enable Latch Status R 0 Device is not Write Enabled (default)
		1 Device is Write Enabled
		0 RDY/BSY
	*/

	uint8_t cmd[1];
	cmd[0] = READ_STATUS_REGISTER1;
	
	HAL_SPI_Transmit(&hspi1, cmd, 1, 1);
	HAL_SPI_Receive(&hspi1, &this->status1.value, 1, 1);
}

void Memory::read_status_register2(void)
{
	/*
		7 RES Reserve for future use R 0 Reserve for future use
		6 CMP Complement Block Protection R/W 0 See table on Block Protection
		5 LB3 Lock Security Register 3 R/W
			0 Security Register page-3 is not locked (default)
			1 Security Register page-3 cannot be erased/programmed
		4 LB2 Lock Security Register 2 R/W
			0 Security Register page-2 is not locked (default)
			1 Security Register page-2 cannot be erased/programmed
		3 LB1 Lock Security Register 1 R/W
			0 Security Register page-1 is not locked (default)
			1 Security Register page-1 cannot be erased/programmed
		2 RES Reserved for future use R 0 Reserved for future use
			1 QE Quad Enable R/W
			0 HOLD and WP function normally (default)
		1 HOLD and WP are I/O pins
		0 SRP1 Status Register Protect bit-1 R/W See table on Status Register Protection
	 */

	uint8_t cmd[1];
	cmd[0] = READ_STATUS_REGISTER2;
	
	HAL_SPI_Transmit(&hspi1, cmd, 1, 1);
	HAL_SPI_Receive(&hspi1, &this->status2.value, 1, 1);
}
uint8_t Memory::decorated_read_status(char chipselect, uint8_t reg_select)
{
	uint8_t reg[1];
	uint8_t cmd[1];
	if (reg_select == 1)
	{
		cmd[0] = READ_STATUS_REGISTER1;
	}
	else if (reg_select == 2)
	{
		cmd[0] = READ_STATUS_REGISTER2;
	}
	else
	{
		return 0xff;
	}
	
	HAL_SPI_Transmit(&hspi1, cmd, 1, 10);
	HAL_SPI_Receive(&hspi1, reg, 1, 10);

	static char message_register[256];
	if (reg_select == 1)
	{
		sprintf(message_register,
			"cs[%d] read %02x\r\n"\
						 "SRP0 [%d]\r\n"\
						 "SEC  [%d]\r\n"\
						 "TB   [%d]\r\n"\
						 "BP2  [%d]\r\n"\
						 "BP1  [%d]\r\n"\
						 "BP0  [%d]\r\n"\
						 "WEL  [%d]\r\n"\
						 "BSY  [%d]\r\n",
			chipselect,
			reg[0],
			(reg[0] >> 7) & 0x1,
			(reg[0] >> 6) & 0x1,
			(reg[0] >> 5) & 0x1,
			(reg[0] >> 4) & 0x1,
			(reg[0] >> 3) & 0x1,
			(reg[0] >> 2) & 0x1,
			(reg[0] >> 1) & 0x1,
			(reg[0] >> 0) & 0x1);
	}
	else if (reg_select == 2)
	{
		sprintf(message_register,
			"cs[%d] read %02x\r\n"\
						 "RES  [%d]\r\n"\
						 "CMP  [%d]\r\n"\
						 "LB3  [%d]\r\n"\
						 "LB2  [%d]\r\n"\
						 "LB1  [%d]\r\n"\
						 "RES  [%d]\r\n"\
						 "QE   [%d]\r\n"\
						 "SRP1 [%d]\r\n",
			chipselect,
			reg[0],
			(reg[0] >> 7) & 0x1,
			(reg[0] >> 6) & 0x1,
			(reg[0] >> 5) & 0x1,
			(reg[0] >> 4) & 0x1,
			(reg[0] >> 3) & 0x1,
			(reg[0] >> 2) & 0x1,
			(reg[0] >> 1) & 0x1,
			(reg[0] >> 0) & 0x1);
	}
	else
	{
		return 0xff;
	}
	Serial_PutString(message_register);
	return reg[0];
}
void Memory::write_status_register(uint16_t write_now)
{
	uint8_t cmd[3];
	cmd[0] = WRITE_STATUS_REGISTER;
	cmd[1] = (uint8_t)((write_now >> 8) & 0xff);
	cmd[2] = (uint8_t)((write_now >> 0) & 0xff);
	HAL_SPI_Transmit(&hspi1, cmd, 3, 10);
}

uint32_t Memory::read(uint32_t read_address)
{

	uint8_t spi_read_command[1];
	spi_read_command[0] = READ_ARRAY1;
	/*Read an array
	 *0x0B needs an additional dummy byte to transmit data
	 *difference is 0x03 can be used only at lower speeds
	 *0x0B can be used at any speed
	 *ONLY 0x0B needs a dummy byte
	*/
	
	HAL_SPI_Transmit(&hspi1, spi_read_command, 1, 50);

	uint8_t address[4];
	uint8_t spi_read_data[4];
	this->uint32_to_address(read_address, address);
	address[3] = 0xFF;    //doesn't matter

	HAL_SPI_Transmit(&hspi1, address, 4, 50);
	HAL_SPI_Receive(&hspi1, spi_read_data, 4, 10);

	return this->read_buffer_to_uint32(spi_read_data);
}

void Memory::read16(uint32_t read_address)
{
	uint8_t address[4];

	this->uint32_to_array(read_address << 8, address);

	char address_str[20];
	sprintf(address_str, "[%02X%02X%02X] ", (uint32_t)address[0], (uint32_t)address[1], (uint32_t)address[2]);
	Serial_PutString(address_str);

	uint8_t spi_read_command[1];
	spi_read_command[0] = READ_ARRAY1;
	/*Read an array
	 *0x0B needs an additional dummy byte to transmit data
	 *difference is 0x03 can be used only at lower speeds
	 *0x0B can be used at any speed
	 *ONLY 0x0B needs a dummy byte
	*/
	if (HAL_SPI_Transmit(&hspi1, spi_read_command, 1, 5) != HAL_OK)
	{
		Serial_PutString("Wrong command sent!!!\r\n");
	}

	static uint8_t spi_read_data[16];


	if (HAL_SPI_Transmit(&hspi1, address, 4, 5) != HAL_OK)
	{
		Serial_PutString("Wrong address sent!!!\r\n");
	}
	delay(20);
	for (int i = 0; i < 16; i++)
	{
		HAL_StatusTypeDef status = HAL_SPI_Receive(&hspi1, &spi_read_data[i], 1, 2);
		if (status  == HAL_ERROR)
		{
			Serial_PutString("Wrong Rx!!!\r\n");
		}
		else if (status == HAL_BUSY)
		{
			Serial_PutString("Rx Busy!!!\r\n");
		}
		else if (status == HAL_TIMEOUT)
		{
			Serial_PutString("Rx TIMEOUT!!!\r\n");
		}
	}



	static uint8_t value[20];
	for (int i = 0; i < 16; i++)
	{

		sprintf((char*)value, "0x%02X ",(uint8_t)( spi_read_data[i] & 0xff));
		Serial_PutString((char*)value);
	}
	Serial_PutString("\r\n");
}

HAL_StatusTypeDef Memory::write(volatile uint32_t write_address, volatile uint32_t write_value)
{
	uint8_t spi_read_command[1];
	spi_read_command[0] = BYTE_PROGRAM;
	// Address start : where to write :
	uint8_t address[4];
	uint8_t counter_in_bytes[4];
	volatile uint8_t complete_write[8];

	address[3] = 0xFF;      //doesn't matter - is not sent
	uint32_to_address(write_address, address);
	uint32_to_array(write_value, counter_in_bytes);
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
	return HAL_SPI_Transmit(&hspi1, (uint8_t *)complete_write, 8, 5);
}
