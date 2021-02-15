#pragma once
#include <stdint.h>
#include <stm32f0xx_hal.h>
#include "hardware.h"


class Memory
{
public:
	uint8_t CS; // 1, 2, 3
	char name[5]; // either "AM" , "APT", "MCU"
	uint8_t id[3];  // valid id would be 0x1f-85-01 or something
	bool id_valid; // true if okay
	byte8_t_mem_status1 status1;
	byte8_t_mem_status2 status2;
	uint32_t cursor; // pointer to read memory
	uint32_t pulses;
	uint32_t serial_number;
	uint32_t max_count;
	uint32_t date;
	Memory(uint8_t);
	~Memory();
	void print_status(void);
	HAL_StatusTypeDef erase(uint32_t erase_address);
	HAL_StatusTypeDef erase(void);
	HAL_StatusTypeDef write_disable(void);
	HAL_StatusTypeDef write_enable(void);
	HAL_StatusTypeDef write_enable(uint32_t address);
	void read_status_register1(void);
	void read_status_register2(void);
	uint8_t decorated_read_status(char chipselect, uint8_t reg_select);
	void write_status_register(uint16_t write_now);
	uint32_t read(uint32_t read_address);
	void read16(uint32_t read_address);
	void check_id(void);
	HAL_StatusTypeDef write(volatile uint32_t write_address, volatile uint32_t write_value);

private:
	void uint32_to_address(uint32_t read_address, uint8_t* address_buf);
	uint32_t read_buffer_to_uint32(uint8_t* spi_read_data);
	void uint32_to_array(uint32_t input, uint8_t* arr);
	void get_id(void);
};
