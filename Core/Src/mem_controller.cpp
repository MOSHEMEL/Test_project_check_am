#include "mem_controller.h"
#include "hardware.h"
#include "main.h"
#include <string.h>

#include "common.h"
#include "spi.h"
#include "xmodem.h"
extern uint8_t aPacketData[PACKET_1K_SIZE + PACKET_DATA_INDEX + PACKET_TRAILER_SIZE];

Mem_ctrl::Mem_ctrl(Memory* AM_MEM, Memory* APT_MEM, Memory* MCU_MEM, CS_reg* cs_register)
{
	this->AM_MEM = AM_MEM;
	this->APT_MEM = APT_MEM;
	this->MCU_MEM = MCU_MEM;
	this->cs_register = cs_register;
}
Mem_ctrl::~Mem_ctrl()
{

}

Memory* Mem_ctrl::chip_select_pull(uint8_t cs_num, GPIO_PinState status)
{
	if (cs_num == 1)
	{
		this->cs_register->cs1(status);
		return this->MCU_MEM;
	}
	else if (cs_num == 2)
	{
		this->cs_register->cs2(status);
		return this->APT_MEM;
	}
	else if (cs_num == 3)
	{
		this->cs_register->cs3(status);
		return this->AM_MEM;
	}
	return NULL;
}

uint32_t Mem_ctrl::read_once(uint8_t cs_num, uint32_t read_address)
{
	Memory* current_MEM = this->chip_select_pull(cs_num, LOW);  // PULL CS LOW so we start to send data
	uint32_t read_value = current_MEM->read(read_address);
	this->chip_select_pull(cs_num, HIGH);
	return read_value;
}

uint32_t Mem_ctrl::read(uint8_t cs_num, uint32_t read_address)
{
	return read_once(cs_num, read_address);
}

uint32_t Mem_ctrl::read_skip_problems(uint8_t cs_num, uint32_t read_address)
{
	uint32_t read_buff[3];
	for (int i = 0; i < 3; i++)
	{
		read_buff[i] = read_once(cs_num, read_address);
	}
	if (read_buff[0] == read_buff[1])
	{
		return read_buff[0];
	}
	else if (read_buff[0] == read_buff[2])
	{
		return read_buff[0];
	}
	else if (read_buff[2] == read_buff[1])
	{
		return read_buff[2];
	}
	return 0xffffffff;
}

void Mem_ctrl::read256(uint8_t cs_num, uint32_t read_address)
{
	for (uint32_t j = 0; j < 16; j++)
	{
		Memory* current_MEM = this->chip_select_pull(cs_num, LOW);    // PULL CS LOW so we start to send data
		current_MEM->read16(read_address +j*16);
		this->chip_select_pull(cs_num, HIGH);
		delay(10);
	}
	return;
}


bool Mem_ctrl::check_connections(uint8_t cs_num)
{
	Memory* current_MEM = this->chip_select_pull(cs_num, LOW);    // PULL CS LOW so we start to send data
	current_MEM->check_id();
	this->chip_select_pull(cs_num, HIGH);
	return current_MEM->id_valid;
}

bool Mem_ctrl::check_connections_with_printback(uint8_t cs_num)
{
	Memory* current_MEM = this->chip_select_pull(cs_num, LOW);     // PULL CS LOW so we start to send data
	current_MEM->check_id();
	this->chip_select_pull(cs_num, HIGH);
	char* id_str = (char*)aPacketData;
	sprintf(id_str, "\r\nCS[%d]: get id: %02X-%02X-%02X\r\n", cs_num,\
														current_MEM->id[0],\
														current_MEM->id[1],\
														current_MEM->id[2]);
	Serial_PutString(id_str);
	return current_MEM->id_valid;
}

void Mem_ctrl::check_connections(void)
{
	for (uint8_t cs = 1; cs < 4; cs++)
	{
		HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
		this->check_connections(cs);
		HAL_Delay(1);
	}

}

uint32_t Mem_ctrl::get_serial(uint8_t cs_num)
{
	static char str_serial_num[45];
	Memory* current_MEM = this->chip_select_pull(cs_num, LOW);    // PULL CS LOW so we start to send data
	current_MEM->serial_number = current_MEM->read(0x000FFFF6);
	this->chip_select_pull(cs_num, HIGH);
	sprintf(str_serial_num, "S\\N AM: %d \r\n", current_MEM->serial_number);
	Serial_PutString(str_serial_num);
	return current_MEM->serial_number;
}

uint32_t Mem_ctrl::get_max(uint8_t cs_num)
{
	uint32_t max_temp = 0xffffffff;
	uint32_t start_time = HAL_GetTick();
	while (max_temp == 0xffffffff && ((HAL_GetTick() - start_time) < 1000))
	{
		Memory* current_MEM = this->chip_select_pull(cs_num, LOW);     // PULL CS LOW so we start to send data
		max_temp = current_MEM->read(0x000FFFE6); 
		current_MEM->max_count = max_temp; 
		this->chip_select_pull(cs_num, HIGH);
		HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
		delay(100);
	}
	return max_temp;
}

uint32_t Mem_ctrl::get_date(uint8_t cs_num)
{
	Memory* current_MEM = this->chip_select_pull(cs_num, LOW);      // PULL CS LOW so we start to send data
	current_MEM->date = current_MEM->read(0x000FFFEE); 
	this->chip_select_pull(cs_num, HIGH);
	return current_MEM->date;
}

HAL_StatusTypeDef Mem_ctrl::poll_complete(uint8_t cs_num, uint32_t timeout)
{
	Memory* current_MEM = this->chip_select_pull(cs_num, LOW);      // PULL CS LOW so we start to send data
	volatile uint32_t time_start = HAL_GetTick();

	if(current_MEM->id_valid)
	{
		current_MEM->read_status_register1();
		this->chip_select_pull(cs_num, HIGH);
		HAL_Delay(1);
		
		while ((current_MEM->status1.bits.WIP) == 1) // while busy 
		{
			this->chip_select_pull(cs_num, LOW);       // PULL CS LOW so we start to send data
			current_MEM->read_status_register1();
			this->chip_select_pull(cs_num, HIGH);
			HAL_Delay(1);
			if (HAL_GetTick() - time_start > timeout)
			{
				return HAL_TIMEOUT;
			}
		}
	}
	else
	{
		char* temp = (char*)aPacketData;
		sprintf(temp,"SPI action could not complete, %s mem not connected \r\n",current_MEM->name);
		Serial_PutString(temp);
		return HAL_ERROR;
	}
	return HAL_OK;
}

HAL_StatusTypeDef Mem_ctrl::write_once(uint8_t cs_num, uint32_t write_address, uint32_t write_value)
{
	HAL_StatusTypeDef status = HAL_OK;
	// --------------------------------------------------------------
	// write  start  : Counter
	// counter value
	//spi_erase_4_kb_from_ff(ChipSelectNumber, write_adress);
	volatile uint32_t addr_temp = write_address;
	volatile uint32_t value_temp = write_value;
	Memory* current_MEM = this->chip_select_pull(cs_num, LOW);

	if (current_MEM->write_enable() != HAL_OK)
	{
		this->chip_select_pull(cs_num, HIGH);
		Serial_PutString("Could not enable write!\r\n");
		return HAL_ERROR;
	}
	this->chip_select_pull(cs_num, HIGH);
	HAL_Delay(1);
	
	this->chip_select_pull(cs_num, LOW);
	current_MEM->write(addr_temp, value_temp);
	this->chip_select_pull(cs_num, HIGH);
	HAL_Delay(1);

	status = this->poll_complete(cs_num, 30);
	if (status != HAL_OK)
	{
		Serial_PutString("Polling ended in fail!\r\n");
		//current_MEM->print_status();
		return HAL_ERROR;
	}
	HAL_Delay(1);

	this->chip_select_pull(cs_num, LOW);
	if (current_MEM->write_disable() != HAL_OK)
	{
		this->chip_select_pull(cs_num, HIGH);
		Serial_PutString("Could not disable write!\r\n");
		return HAL_ERROR;
	}
	this->chip_select_pull(cs_num, HIGH);
	HAL_Delay(1);

	volatile uint32_t read_back = this->read(cs_num, write_address);
	if (read_back != write_value)
	{
		static char str_value_mismatch[50];
		Serial_PutString("Write Error\r\n");
		sprintf(str_value_mismatch,"CS %d ADR: %02x W: %08X R: %08X\r\n",cs_num, write_address, write_value, read_back);
		Serial_PutString(str_value_mismatch);
		return HAL_ERROR;
	}
	static char str_value_mismatch[50];
	sprintf(str_value_mismatch, "CS %d ADR: %02x W: %08X\r\n", cs_num, write_address, write_value);
	Serial_PutString("Succesful write!\r\n");
	Serial_PutString(str_value_mismatch);

	return HAL_OK;
}
HAL_StatusTypeDef Mem_ctrl::write(uint8_t cs_num, uint32_t write_address, uint32_t write_value)
{
	HAL_StatusTypeDef status = HAL_OK;

	for (int i = 0; i < RETRY_TIMES; i++)
	{
		status = this->write_once(cs_num, write_address, write_value);
		if (status == HAL_OK)
		{
			return status;
		}
	}
	return status;
}

bool Mem_ctrl::Read_no_pulses_remaining(void)
{
	uint32_t No_More_Pulse_Adrress = 0xffff40;
	const uint32_t magic_num = 0x444f4e45;
	uint32_t Read_back = 0;
	
	Read_back = this->read(3, No_More_Pulse_Adrress);
	if (magic_num == Read_back)
		return true;
	return false;
}
void Mem_ctrl :: erase_all_am()
{
	this->erase(3);
	HAL_Delay(20);
}
	uint32_t Mem_ctrl::Max_search_Am(uint32_t Address, uint32_t start_address)

{
	uint32_t temp_max = 0, Max_search = 0, Max_add = 0;
	uint32_t i;

	if (this->Read_no_pulses_remaining())
	{
		//reading flag in order to test if over the max
		this->AM_MEM->pulses = this->get_max(3);
		return 0xff000;
	}
	for (i = start_address; i < Address; i = i + 4)
	{
		if ((temp_max = this->read(3, i)) != 0xffffffff && (temp_max < this->get_max(3) +1))
		{
			if (Max_search <= temp_max) 
			{
				Max_search = temp_max;
				Max_add = i;
			}
		}
	}
	this->AM_MEM->cursor = Max_add;
	this->AM_MEM->pulses = Max_search;
	return Max_add;
	
}	


uint32_t Mem_ctrl::binary_and_patch(void)
{
	if (this->Read_no_pulses_remaining())
	{
		//reading flag in order to test if over the max
		this->AM_MEM->pulses = this->get_max(3);
		return 0xff000;
	}
	uint32_t address_pulse = 0;
	char debug_str[20];
	address_pulse = this->get_ap_offset();
	sprintf(debug_str, "binfind %02X\r\n", address_pulse);
	Serial_PutString(debug_str);
	this->AM_MEM->cursor = address_pulse;
	if (this->AM_MEM->cursor == 0)
	{
		Serial_PutString("Empty Memory\r\n");
		if (this->read(3, 0) == 0xffffffff)
		{
			// we should probably just blank out 0-th cell, cause every error in code points to it
			this->write(3, 0, 0);
			this->AM_MEM->pulses = 0;
			sprintf(debug_str, "pulses %d\r\n", this->AM_MEM->pulses);
			Serial_PutString(debug_str);
			return address_pulse;
		}
		else
		{
			uint32_t temp_cursor = 0;
			uint32_t temp_pulse = 0;
			while (temp_pulse != 0xffffffff && temp_cursor < 0xff000)
			{
				if (this->read(3, temp_cursor + 4) == 0xffffffff)
				{
					sprintf(debug_str, "cursor %02x \r\n", temp_cursor);
					Serial_PutString(debug_str);
					break;
				}
				else if(temp_pulse > this->AM_MEM->pulses)
				{
					this->AM_MEM->pulses = temp_pulse;
				}
				temp_pulse = this->read(3, temp_cursor + 4);
				temp_cursor += 4;
			}
			this->AM_MEM->cursor = temp_cursor;
			sprintf(debug_str, "pulses %d\r\n", this->AM_MEM->pulses);
			Serial_PutString(debug_str);
			return temp_cursor;
		}
	}
	else if (this->AM_MEM->cursor == 0xffffff00)
	{
		// If we have a full AM - we want to erase it and then write back the data
		sprintf(debug_str, "Full memory - cleanup\r\n");
		Serial_PutString(debug_str);
		// remember last pulse count
		uint32_t temp_pulses = this->read(3, address_pulse);

		//ERASE
		this->erase(3);
		HAL_Delay(20);
		
		while (this->read(3, address_pulse) != 0xffffffff)
		{
			HAL_Delay(1);
		}
		
		//WRITE SERIAL BACK
		this->write(3, 0x00FFFFA, 0x534E554D);
		this->write(3, 0x00FFFF6, this->AM_MEM->serial_number);
		this->write(3, 0x00FFF10, 0x534E554D);
		this->write(3, 0x00FFF14, this->AM_MEM->serial_number);

		//WRITE DATE BACK
		this->write(3, 0x00FFFF2, 0x44415445);
		this->write(3, 0x00FFFEE, this->AM_MEM->date);
		this->write(3, 0x00FFF20, 0x44415445);
		this->write(3, 0x00FFF24, this->AM_MEM->date);

		//WRITE MAXI BACK
		this->write(3, 0x00FFFEA, 0x4D415849);
		this->write(3, 0x00FFFE6, this->AM_MEM->max_count);
		this->write(3, 0x00FFF30, 0x4D415849);
		this->write(3, 0x00FFF34, this->AM_MEM->max_count);

		//write
		this->write(3, 0x0, temp_pulses);
		this->AM_MEM->pulses = temp_pulses;
		sprintf(debug_str, "pulses %d\r\n", this->AM_MEM->pulses);
		Serial_PutString(debug_str);
		address_pulse = 0;
		return address_pulse;
	}
	
	// Otherwise Lets try to find the maximum
	uint32_t max_so_far = 0;
	uint32_t last_address_to_scan = 0;
	sprintf(debug_str, "MAXI FIND\r\n");
	Serial_PutString(debug_str);
	if (this->AM_MEM->cursor > (25 * 4))
	{
		Serial_PutString("MAXI FIND 25 values before");
		last_address_to_scan = this->AM_MEM->cursor - 25 * 4;
	}
	for (uint32_t temp_cursor = this->AM_MEM->cursor + 100; temp_cursor >= last_address_to_scan; temp_cursor -= 4)
	{
		if (temp_cursor > 0xfff00)
		{
			break;
		}
		uint32_t pulses_temp = this->read(3, temp_cursor);
		sprintf(debug_str, "%d @ %02X\r\n", pulses_temp, temp_cursor);
		Serial_PutString(debug_str);
		if (pulses_temp != 0 && pulses_temp != 0xffffffff && pulses_temp <= this->AM_MEM->max_count + 1)
		{
			if (max_so_far < pulses_temp)
			{
				max_so_far = pulses_temp;
			}
		}
	}
	this->AM_MEM->pulses = max_so_far;
	sprintf(debug_str, "maxfind %d\r\n", max_so_far);
	Serial_PutString(debug_str);
	return address_pulse;

	/*
	// sometimes, we may 
	if(this->AM_MEM->pulses == 0 || this->AM_MEM->pulses > this->AM_MEM->max_count + 10000)
	{
		static char read_zeros[] = "Read zeroes Or Invalids in mem\r\n";
		uint32_t address_valid = address_pulse;
		do
		{
			Serial_PutString(read_zeros)
			address_valid -= 4;    // We backtrack to find last valid number
			this->AM_MEM->pulses = this->read(3, address_valid);      // We backtrack to find last valid number
		} while ((this->AM_MEM->pulses == 0 || this->AM_MEM->pulses > this->AM_MEM->max_count + 10000) && address_valid !=0);
	}
	return address_pulse;
	*/
}

uint32_t Mem_ctrl::get_ap_offset(void)
{
	/*
	 *def find_log(ram):
		if len(ram)>1:
			steps =  0
			l = 0
			r = len(ram) - 1
			mid = (l + (r - l))//2
			while(l < r):
				mid = l + (r - l)//2;
				if ((ram[mid] != 0 )& (ram[mid+1] == 0)):
					steps=steps+1
					return steps
				elif (ram[mid] == 0):
					r = mid - 1
					steps=steps+1
				else:
					l = mid + 1
					steps=steps+1
			return steps
	
	NOTE: either we make this do the right thing efficiently
	or we doing stupid things faster.
	
	 **/
	char debug_str[20];

	// simple case 1 : the array is empty
	if(this->read_skip_problems(3, 0) == 0xffffffff)
	{
		sprintf(debug_str, "Empty mem\r\n");
		Serial_PutString(debug_str);
		return 0;
	}
	uint32_t rx_address =  0x3fc00;     // The maximum address that the mem can reach (div 4)
	// simple case 2 : the array is full
	if(this->read_skip_problems(3, 0xff000) != 0xffffffff)
	{
		sprintf(debug_str, "Full mem\r\n");
		Serial_PutString(debug_str);
		return 0xff000;
	}

	uint32_t low = 0;
	uint32_t high = 0x3fc00;       // Different Method, we just read everything  (div 4)
	uint32_t mid = (low + high) / 2;
	int c = 0;
	Serial_PutString("Finding Binary\r\n");
	while (low < high)
	{
		if (c % 10 == 0)
		{
			HAL_GPIO_TogglePin(BLINKING_LED_GPIO_Port, BLINKING_LED_Pin);
			HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
		}
		c++;

		if (c > 100)
		{
			while (this->read_skip_problems(3, rx_address) != 0xffffffff)
			{
				rx_address += 4;
			}
			return rx_address - 8;
		}
		
		rx_address = mid * 4;
		if (this->read_skip_problems(3, rx_address) == 0xffffffff || this->read_skip_problems(3, rx_address) == 0) 
		{
			high = mid;
			mid = (low + high) / 2; // If we haven't done the units in div 4, here we would have be dividing by 8
		}
		else
		{
			// means that read(3,rx_adress)!= 0xffffff
			if(this->read_skip_problems(3, rx_address + 4) == 0xffffffff )
			{
				return rx_address;
			}
			else
			{
				low = mid;
				mid = (low + high) / 2;
			}
		}
	}
	return rx_address;
}


uint32_t Mem_ctrl::get_serial(void)
{
	HAL_Delay(1);
	return this->get_serial(3);
}
void Mem_ctrl::get_max(void)
{
	this->get_max(3);
	HAL_Delay(1);
}

void Mem_ctrl::get_date(void)
{
	this->get_date(3);
	HAL_Delay(1);
}

void Mem_ctrl::print_register(uint8_t cs_num)
{
	Memory* current_mem = this->chip_select_pull(cs_num, LOW);
	current_mem->read_status_register1();
	this->chip_select_pull(cs_num, HIGH);
	HAL_Delay(1);
	this->chip_select_pull(cs_num, LOW);
	current_mem->read_status_register2();
	this->chip_select_pull(cs_num, HIGH);
	current_mem->print_status();	
}

void Mem_ctrl::write_register(uint8_t cs_num, uint16_t value)
{
	Memory* current_mem = this->chip_select_pull(cs_num, LOW);
	current_mem->write_enable();
	this->chip_select_pull(cs_num, HIGH);
	HAL_Delay(1);
	this->chip_select_pull(cs_num, LOW);
	current_mem->write_status_register(value);
	this->chip_select_pull(cs_num, HIGH);
}

void Mem_ctrl::erase(uint8_t cs_num, uint32_t address)
{
	Memory* current_mem = this->chip_select_pull(cs_num, LOW);
	current_mem->write_enable();
	this->chip_select_pull(cs_num, HIGH);
	HAL_Delay(1);
	this->chip_select_pull(cs_num, LOW);
	current_mem->erase(address);
	this->chip_select_pull(cs_num, HIGH);
}

void Mem_ctrl::erase(uint8_t cs_num)
{
	Memory* current_mem = this->chip_select_pull(cs_num, LOW);
	current_mem->write_enable();
	this->chip_select_pull(cs_num, HIGH);
	HAL_Delay(1);
	this->chip_select_pull(cs_num, LOW);
	current_mem->erase();
	this->chip_select_pull(cs_num, HIGH);
}

void Mem_ctrl::read_back_current_cs(void)
{
	this->cs_register->read_back_current_cs();
}

void Mem_ctrl::LOG_WRITE(stall_log_out LOG)
{
	if (this->MCU_MEM->id_valid)
	{
		uint32_t addr = 0;
		uint32_t val = this->read(1, addr); 
		while (val != 0xffffffff)
		{
			addr += 16;   // Each log is 16
			val = this->read(1, addr);
		}
		Memory* current_mem = this->chip_select_pull(1, LOW);
		current_mem->write_enable();
		this->chip_select_pull(1, HIGH);
		delay(100);
		this->chip_select_pull(1, LOW);
		// SEND WRT FUNCTION
		uint8_t spi_read_command[1];
		spi_read_command[0] = BYTE_PROGRAM;
		HAL_SPI_Transmit(&hspi1, spi_read_command, 1, 1);
		// SEND ADDRESS
		// Address start : where to write :
		uint8_t address[4];

		address[3] = 0xFF;       //doesn't matter - is not sent
		translate_int_to_address(addr, address);
		HAL_SPI_Transmit(&hspi1, address, 3, 1);

		HAL_SPI_Transmit(&hspi1, LOG.serial, 16, 2);
		this->chip_select_pull(1, HIGH);
	}
}

void Mem_ctrl::LOG_DEBUG(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	/**
	 * This function is writing out debug info on stall each time there is a problem
	 * It writes to the mcu memory and can be read from debug terminal.
	 * In the future it may be extended to suit our needs or blackholed to not be a problem
	 */
	stall_log_out LOG;
	LOG.fields.error = arg0;
	LOG.fields.am_sn = arg1;
	LOG.fields.apt_sn = arg2;
	LOG.fields.am_count = arg3;
	this->LOG_WRITE(LOG);
}

void  Mem_ctrl::LOG_FIND_LAST()
{
	if (this->MCU_MEM->id_valid)
	{
		uint32_t addr = 0;
		uint32_t val = this->read(1, addr); 
		while (val != 0xffffffff)
		{
			addr += 16;     // Each log is 16
			val = this->read(1, addr);
		}
		//need to decrease -16 and that the address is not zero or over the limit
		this->Last_addr = addr;
		if (this->Last_addr!=0)
		{
			this->Last_addr = this->Last_addr - 16;
		}
	}
}

void  Mem_ctrl::LOG_READ_PARSE(uint32_t Readaddr)
{
	char str_out[30];
	uint32_t val = 0;
	stall_log_out LOG;
	uint32_t temp_error = 0;
	
	LOG.fields.error = this->read(1, Readaddr); 
	LOG.fields.error = translate_address_to_int(LOG.serial);
	LOG.fields.am_sn = this->read(1, Readaddr+4); 
	LOG.fields.am_sn = translate_address_to_int(LOG.serial + 4);
	LOG.fields.apt_sn = this->read(1, Readaddr + 8); 
	LOG.fields.apt_sn = translate_address_to_int(LOG.serial + 8);
	LOG.fields.am_count = this->read(1, Readaddr + 12);
	LOG.fields.am_count = translate_address_to_int(LOG.serial + 12);
	sprintf(str_out, "e[%08x], a:[%08x],  c[%08x]\r\n", LOG.fields.error, LOG.fields.am_sn, LOG.fields.am_count);
	Serial_PutString(str_out);
 }