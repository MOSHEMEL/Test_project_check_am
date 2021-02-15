#pragma once
#include "mem.h"
#include "cs_register.h"

class Mem_ctrl
{
public:
	uint32_t Last_addr;
	Memory* MCU_MEM;
	Memory* APT_MEM;
	Memory* AM_MEM;
	CS_reg* cs_register;
	Mem_ctrl(Memory* , Memory* , Memory*, CS_reg*);
	~Mem_ctrl();
	void check_connections(void);
	HAL_StatusTypeDef poll_complete(uint8_t cs_num, uint32_t timeout);
	HAL_StatusTypeDef write(uint8_t cs_num,uint32_t write_address, uint32_t write_value);
	HAL_StatusTypeDef write_once(uint8_t cs_num,uint32_t write_address, uint32_t write_value);
	uint32_t read(uint8_t, uint32_t);
	uint32_t read_skip_problems(uint8_t cs_num, uint32_t read_address);
	void read256(uint8_t, uint32_t);
	void print_register(uint8_t);
	void write_register(uint8_t, uint16_t);
	void erase(uint8_t cs_num, uint32_t address);
	void erase(uint8_t cs_num);
	void read_back_current_cs();
	void LOG_WRITE(stall_log_out LOG);
	void LOG_DEBUG(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3);
	void LOG_READ_PARSE(uint32_t Readaddr);
	uint32_t get_serial(void);
	void get_max(void);
	void get_date(void);
	uint32_t Max_search_Am(uint32_t Address, uint32_t start_address);
	uint32_t binary_and_patch(void);
	bool check_connections(uint8_t);
	bool check_connections_with_printback(uint8_t);
	bool Read_no_pulses_remaining(void);
	void LOG_FIND_LAST();
	void erase_all_am();

private:
	uint32_t get_serial(uint8_t);
	uint32_t get_max(uint8_t);
	uint32_t get_date(uint8_t);
	Memory* chip_select_pull(uint8_t, GPIO_PinState);
	uint32_t read_once(uint8_t cs_num, uint32_t read_address);
	uint32_t get_ap_offset(void);
};
