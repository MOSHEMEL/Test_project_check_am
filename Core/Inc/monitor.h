#pragma once
#include <stdint.h>
#include "hardware.h"
#include "main_controller.h"
#include "usart.h"

/**
 * \brief This is the technician enabled monitor
 */

#define  space     ' '
#define  Backspace 0x08
#define  Brk     "\r\n"
 

class monitor
{
public:
	monitor(main_controller*);
	~monitor();
	void test_engine();
	void parse_input_uart();
	uint8_t* buffer;
	uint32_t address;
	uint32_t value;
	uint8_t chip_select;
	main_controller* apt;
	void rx_write();
	void rx_status_read();
	void rx_status_write();
	void rx_pattern();
	void rx_read();
	void rx_erase();
	void rx_nuke();
	void rx_start_motor();
	void rx_stop_motor();
	void rx_reverse_motor();
	void rx_go_active();
	void rx_test_read();
	void rx_set_serial();
	void rx_set_maxi();
	void rx_set_date();
	void rx_scan();
	void rx_scan(uint8_t cs_num, uint32_t address);
	void rx_get_man_id();
	void rx_get_man_id_check();
	void rx_find_pulses();
	void rx_upload();
	void rx_debug();
	void rx_exit();
	void rx_help();
	void rx_pass();
	void Config_Apt_Mem_Var();
	void Reading_Log_File();
	
	void pwr_on()
	{
		HAL_GPIO_WritePin(ENABLE_POWER_12V_GPIO_Port, ENABLE_POWER_12V_Pin, GPIO_PIN_SET);
		HAL_Delay(60000);
	}
	void pwr_off()
	{
		HAL_GPIO_WritePin(ENABLE_POWER_12V_GPIO_Port, ENABLE_POWER_12V_Pin, GPIO_PIN_RESET);
		HAL_Delay(60000);
	}
	void rx_blink();
	void rx_adc();
	void rx_status_reg_read();
	void rx_read_back();
	void serial_consume();
private:
	uint32_t index;
	volatile char rx_char;
	uint8_t tkn_i;
	char* tokens[10];
};

