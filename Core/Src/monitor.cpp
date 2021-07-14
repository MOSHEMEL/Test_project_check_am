#include "monitor.h"
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "menu.h"

extern uint8_t BUFFER_RX[RXBUFFERSIZE];
extern volatile bool RESET_PRESSED;
extern volatile bool WATCHDOG_IS_ENABLED;
extern volatile uint8_t COUNTER_V;
monitor::monitor(main_controller* apt)
{
	this->apt = apt;
	this->buffer = BUFFER_RX;
	memset(this->buffer, 0, sizeof(uint8_t) * RXBUFFERSIZE);
	memset(this->tokens, 0, sizeof(char) * 10);
	this->address = 0;
	this->value = 0;
	this->chip_select = 0;
	this->index = 0;
	this->rx_char = 0;
	this->tkn_i = 0;
}

monitor::~monitor()
{
}


void monitor::test_engine()
{
	HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, (GPIO_PinState)true);
	this->apt->get_device()->start_motor_pwm();
	for (int i = 0; i < 100; i++)
	{
		this->apt->get_device()->start_motor();
		delay(10000);
	}
	while (1)
	{
		Serial_PutString("Run\n\r");
		HAL_Delay(10000);
	}
}
void monitor::parse_input_uart()
{
	// ReSharper disable once CppLocalVariableMayBeConst
	volatile HAL_StatusTypeDef status = HAL_UART_Receive(SERIAL_PC, (uint8_t*)&this->rx_char, 1, 1);
	if (status == HAL_OK)
	{
		if (this->rx_char == '%' || this->rx_char == '#')
		{
			this->buffer[this->index] = space;
			this->index = 0;
			
			Serial_PutString(Brk);
			Serial_PutString((char*)this->buffer);
			//Serial_PutString(brk);

			char* rest = (char*)this->buffer;
			// Returns first token
			this->tkn_i = 0;
			this->tokens[this->tkn_i] = strtok_r(rest, ",", &rest);

			// Keep printing tokens while one of the 
			// delimiters present in str[]. 
			while(this->tokens[tkn_i] != NULL)
			{
				this->tkn_i++;
				this->tokens[tkn_i] = strtok_r(NULL, ",", &rest);
			}
			if (this->tokens[0] != NULL)
			{
				// PARSE THE STRING
				if(strncmp(this->tokens[0], "wrt", 3) == 0)
				{
					this->rx_write();
				}
				if (strncmp(this->tokens[0], "testeng", 7) == 0)
				{
					this->test_engine();
				}
				else if(strncmp(this->tokens[0], "status1", 7) == 0)
				{
					this->rx_status_read();
				}
				else if(strncmp(this->tokens[0], "statusw", 7) == 0)
				{
					this->rx_status_write();
				}
				else if(strncmp(this->tokens[0], "rd", 2) == 0)
				{
					this->rx_read();
				}
				else if(strncmp(this->tokens[0], "erase", 5) == 0)
				{
					this->rx_erase();
				}
				else if(strncmp(this->tokens[0], "testmem", 6) == 0)
				{
					this->test_memo();
				}
				else if(strncmp(this->tokens[0], "nuke", 4) == 0)
				{
					this->rx_nuke();
				}
				else if(strncmp(this->tokens[0], "motoron", 7) == 0)
				{
					this->rx_start_motor();
				}
				else if(strncmp(this->tokens[0], "motoroff", 8) == 0)
				{
					this->rx_stop_motor();
				}
				else if(strncmp(this->tokens[0], "run", 3) == 0)
				{
					this->rx_go_active();
				}
				else if(strncmp(this->tokens[0], "testread", 8) == 0)
				{
					this->rx_test_read();
				}
				else if(strncmp(this->tokens[0], "snum", 4) == 0)
				{
					this->rx_set_serial();
				}
				else if(strncmp(this->tokens[0], "maxi", 4) == 0)
				{
					this->rx_set_maxi();
				}
				else if(strncmp(this->tokens[0], "date", 4) == 0)
				{
					this->rx_set_date();
				}
				else if(strncmp(this->tokens[0], "pwr_on", 6) == 0)
				{
					this->pwr_on();
				}
				else if(strncmp(this->tokens[0], "pwr_off", 7) == 0)
				{
					this->pwr_off();
				}
				else if(strncmp(this->tokens[0], "scan", 4) == 0)
				{
					this->rx_scan();
				}
				else if(strncmp(this->tokens[0], "pattern", 7) == 0)
				{
					this->rx_pattern();
				}
				else if(strncmp(this->tokens[0], "dump", 4) == 0)
				{
					uint8_t cs_num = *(this->tokens[1]) - '0';  // try to parse into cs number
					for(uint32_t i = 0 ; i < 0x100000 / 0x100 ; i++)
					{
						this->rx_scan(cs_num, i * 0x100);
					}
				}
				else if(strncmp(this->tokens[0], "getid", 5) == 0)
				{
					this->rx_get_man_id();
				}
				else if(strncmp(this->tokens[0], "validate", 8) == 0)
				{
					this->rx_get_man_id_check();
				}
				else if(strncmp(this->tokens[0], "find", 4) == 0)
				{
					this->rx_find_pulses();
				}
				else if(strncmp(this->tokens[0], "upload", 5) == 0)
				{
					this->rx_upload();
				}
				else if(strncmp(this->tokens[0], "debug", 5) == 0)
				{
					this->rx_debug();
				}
				else if(strncmp(this->tokens[0], "exit", 4) == 0)
				{
					this->rx_exit();
				}
				else if(strncmp(this->tokens[0], "adc_read", 8) == 0 || strncmp(this->tokens[0], "read_adc", 8) == 0)
				{
					this->rx_adc();
					for (int i = 0; i < 8; i++)
					{
						sprintf((char*)this->buffer, "ADC %d, %d\r\n", i, this->apt->get_adc()->channels_buffer[i].value);
						Serial_PutString((char*)this->buffer);
					}
				}
				else if(strncmp(this->tokens[0], "stat_read", 9) == 0)
				{
					this->rx_status_reg_read();
				}
				else if(strncmp(this->tokens[0], "readback", 8) == 0)
				{
					this->rx_read_back();
				}
				else if(strncmp(this->tokens[0], "help", 4) == 0)
				{
					this->rx_help();
				}
				else if(strncmp(this->tokens[0], "pass", 4) == 0)
				{
					this->rx_pass();
				}
				else if(strncmp(this->tokens[0], "blink", 5) == 0)
				{
					this->rx_blink();
				}
				else if(strncmp(this->tokens[0], "reverse", 7) == 0)
				{
					this->rx_reverse_motor();	
				}
				else if(strncmp(this->tokens[0], "log", 3) == 0)
				{
					uint8_t cs_err = *(this->tokens[1]) - '0';   // try to parse into cs number

					stall_log_out LOG;
					LOG.fields.error = 100 + cs_err;
					LOG.fields.am_sn = 0x123456;
					LOG.fields.apt_sn = 0xa5a5;
					LOG.fields.am_count = 0xff00ff00;
					this->apt->get_mem()->LOG_WRITE(LOG);
				}
				else if(strncmp(this->tokens[0], "starwars", 7) == 0)
				{
					this->apt->get_device()->star_wars();
				}
				else if(strncmp(this->tokens[0], "watch_off", 8) == 0)
				{
					static char watchmen[] = "Watchdog Disabled\r\n";
					Serial_PutString(watchmen);
					WATCHDOG_IS_ENABLED = false;
				}
				else if(strncmp(this->tokens[0], "c_method", 8) == 0)
				{
					static char counter_method_str[] = "counting_method\r\n";
					Serial_PutString(counter_method_str);
					COUNTER_V = (uint8_t)(*(this->tokens[1]) - '0');
				}
				else if(strncmp(this->tokens[0], "reset", 5) == 0)
				{
					HAL_NVIC_SystemReset();
				}
				else if(strncmp(this->tokens[0], "puapt", 5) == 0)
				{
					Config_Apt_Mem_Var();
				}
				else if(strncmp(this->tokens[0], "readlog", 7) == 0)
				{
					Reading_Log_File();
				}
				memset(this->buffer, 0, sizeof(this->buffer));

				/*				char TEMP_BUFFER[50];
				sprintf((char*)TEMP_BUFFER, "Recieved %d args\r\n", this->tkn_i);
				Serial_PutString((char*)TEMP_BUFFER);
				for (int i = 0; i < tkn_i; i++)
				{
					sprintf((char*)TEMP_BUFFER, "%d -> %s \r\n", i, this->tokens[i]);
					Serial_PutString((char*)TEMP_BUFFER);
				}
				*/

			}
		}
		else
		{
			if (this->rx_char >= ' ' && this->rx_char <= '~')
			{
				if (this->rx_char >= 'A' && this->rx_char <= 'Z')
				{
					this->rx_char += uint8_t('a' - 'A');    // toLowerCase
				}
				this->buffer[this->index] = this->rx_char;
				this->index++;
				this->index = this->index % RXBUFFERSIZE;
			}
			else if (this->rx_char == Backspace)
			{
				if (this->index > 0)
				{
					this->buffer[this->index] =space;
					this->index--;
					Serial_PutString(" \b");
					//getting backspace writing space and getting back
				}
			}
		}
	}
	else if(status == HAL_ERROR || status == HAL_BUSY)
	{
		static char error_uart[] = "UART rcv dead\r\n";
		Serial_PutString(error_uart);
	}
}
	
void monitor::serial_consume()
{
	parse_input_uart();

	// DEBUG FUNCTION: when we are in uart-debug, the reset button is used as a FIRE BUTTON
	RESET_PRESSED = test_for_reset_press_aux();
	if (RESET_PRESSED)
	{
		static char rst_press_str[] = "Reset Pressed!\r\n";
		Serial_PutString(rst_press_str);
		if (this->apt->is_b_motor_running())
		{
			this->apt->set_b_motor_running(false);
			this->apt->get_device()->stop_motor_after_run();
		}
		else
		{
			this->apt->get_debug()->fast_beeps_2();
			this->apt->get_device()->clear_counter();
			this->apt->added_counter();
			this->apt->set_b_motor_running(true);
			this->apt->get_device()->start_motor_pwm();  // This takes around 500ms
			//this->apt->get_debug()->fast_beeps_3();
		}
		RESET_PRESSED  = false;
	}
	//this->apt->is_b_is_interlocked();
	if (this->apt->is_b_motor_running())
	{
			uint8_t additional_counts = this->apt->added_counter();
			static uint32_t temp_pulses = 0;
			temp_pulses += additional_counts;
			this->apt->set_un32_pulses(temp_pulses);
			this->apt->get_screen()->update_pulse_counter(temp_pulses);
		if (!this->apt->is_b_is_interlocked())
		{
			static char interlock[] = "Interlock!\r\n";
			Serial_PutString(interlock);
			this->apt->set_b_motor_running(false);
			this->apt->get_device()->stop_motor_init();
			this->apt->get_debug()->long_beep();
			uint32_t updates_last = HAL_GetTick();
			while (HAL_GetTick() - updates_last < 100)
			{
				HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
				HAL_Delay(10);
			}
			this->apt->get_debug()->long_beep();
			updates_last = HAL_GetTick();
			while (HAL_GetTick() - updates_last < 100)
			{
				HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
				HAL_Delay(10);
			}
		}
	}
}

void monitor::rx_status_read()
{
	if (this->tkn_i > 0)
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		uint8_t cs_num = *(this->tokens[1]) - '0'; // try to parse into cs number
		this->apt->get_mem()->print_register(cs_num);
	}
}

void monitor::rx_blink()
{
	if (this->tkn_i > 0)
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		uint32_t value = strtoul(this->tokens[1], NULL, 16);
		/*
		HAL_GPIO_WritePin(DEBUG_AM_OK_GPIO_Port, DEBUG_AM_OK_Pin, (GPIO_PinState)( value & (0x1 << 0)));
		HAL_GPIO_WritePin(DEBUG_APT_OK_GPIO_Port, DEBUG_APT_OK_Pin, (GPIO_PinState)(value & (0x1 << 1)));
		HAL_GPIO_WritePin(DEBUG_MCU_OK_GPIO_Port, DEBUG_MCU_OK_Pin, (GPIO_PinState)(value & (0x1 << 2)));
		HAL_GPIO_WritePin(DEBUG_PRESSURE_OK_GPIO_Port, DEBUG_PRESSURE_OK_Pin, (GPIO_PinState)(value & (0x1 << 3)));
		HAL_GPIO_WritePin(DEBUG_MOTOR_RUNNING_GPIO_Port, DEBUG_MOTOR_RUNNING_Pin, (GPIO_PinState)(value & (0x1 << 4)));
		HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_1_Pin, (GPIO_PinState)(value & (0x1 << 5)));
		HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_2_Pin, (GPIO_PinState)(value & (0x1 << 6)));
		HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_3_Pin, (GPIO_PinState)(value & (0x1 << 7)));
		 */
		sprintf((char*)BUFFER_RX,
								"AM %d\r\n"\
								"APT %d\r\n"\
								"MCU %d\r\n"\
								"P %d\r\n"\
								"M %d\r\n"\
								"S1 %d\r\n"\
								"S2 %d\r\n"\
								"S3 %d\r\n",
			(value & (0x1 << 0)) >> 0,
			(value & (0x1 << 1)) >> 1,
			(value & (0x1 << 2)) >> 2,
			(value & (0x1 << 3)) >> 3,
			(value & (0x1 << 4)) >> 4,
			(value & (0x1 << 5)) >> 5,
			(value & (0x1 << 6)) >> 6,
			(value & (0x1 << 7)) >> 7);
		Serial_PutString((char*)BUFFER_RX);
		this->apt->get_debug()->update(value); // We flip all the bits
	}
}

void monitor::rx_status_write()
{
	if (this->tkn_i > 1)
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		uint8_t cs_num = *(this->tokens[1]) - '0'; // try to parse into cs number
		// ReSharper disable once CppLocalVariableMayBeConst
		uint16_t value = strtoul(this->tokens[2], NULL, 16);
		this->apt->get_mem()->write_register(cs_num, value);
	}
}
void monitor::rx_pattern()
{
	if (this->tkn_i > 1)
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		uint8_t cs_num = *(this->tokens[1]) - '0';  // try to parse into cs number
		// ReSharper disable once CppLocalVariableMayBeConst
		this->apt->get_mem()->write_register(cs_num, value);

		
		uint32_t value = 0x055AA55F;
		for (uint32_t address = 0; address < 0x10000; address += 4)
		{
			this->apt->get_mem()->write(cs_num, address, value);
			if (address % 0x1000 == 0)
			{
				Serial_PutString("Populating\r\n");
			}
		}
		Serial_PutString("Finished Populating!\r\n");
	}
}

void monitor::rx_nuke()
{
	if (this->tkn_i > 0)
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		uint8_t cs_num = *(this->tokens[1]) - '0'; // try to parse into cs number
		this->apt->get_mem()->erase(cs_num);
	}
}

void monitor::rx_erase()
{
	char TEMP_BUFFER[50];
	if (this->tkn_i > 2)
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		uint8_t cs_num = *(this->tokens[1]) - '0'; // try to parse into cs number
		uint32_t value = strtoul(this->tokens[2], NULL, 16);
		sprintf((char*)TEMP_BUFFER, "CS[%d] Erase 4K @ %02X\r\n", cs_num, value);
		Serial_PutString((char*)TEMP_BUFFER);
		this->apt->get_mem()->erase(cs_num, value);
	}
}

void monitor::rx_write()
{
	if (this->tkn_i > 3)
	{
	    uint8_t cs_num = *(this->tokens[1]) - '0'; // try to parse into cs number
		uint32_t address = strtoul(this->tokens[2], NULL, 16);
		uint32_t value = strtoul(this->tokens[3], NULL, 16);
		this->apt->get_mem()->write(cs_num, address, value);

		char* write_str = (char*)BUFFER_RX;
		sprintf(write_str, "\r\nWrite: %08lX at address %08lX\r\n", value, address);
		Serial_PutString(write_str);
		delay(100);
		uint32_t read_back = this->apt->get_mem()->read(cs_num, address);

		char* read_str = (char*)BUFFER_RX;
		sprintf(read_str, "\r\nRead:  %08lX at address %08lX\r\n", read_back, address);
		Serial_PutString(read_str);
	}
}

void monitor::rx_read()
{
	if (this->tkn_i > 1)
	{
		uint8_t cs_num = *(this->tokens[1]) - '0'; // try to parse into cs number
		uint32_t address = strtoul(this->tokens[2], NULL, 16);
		uint32_t read_back = this->apt->get_mem()->read(cs_num, address);

		char* read_str = (char*)BUFFER_RX;
		sprintf(read_str, "\r\nRead:  %08lX at address %08lX\r\n", read_back, address);
		Serial_PutString(read_str);
	}
}

void monitor::rx_start_motor()
{
	if (this->apt->is_b_motor_running())
		{
			return;
		}
		else
		{
			this->apt->set_b_motor_running(true);
			this->apt->get_device()->start_motor_pwm();
			const static char motor_stat_stop_c[] = "Motor Running\r\n";
			Serial_PutString(motor_stat_stop_c);
		}
}

void monitor::rx_stop_motor()
{
	if (this->apt->is_b_motor_running())
		{
			this->apt->set_b_motor_running(false);
			this->apt->get_device()->stop_motor_init();
			const static char motor_stat_start_c[] = "Motor Stopping\r\n";
			Serial_PutString(motor_stat_start_c);
		}
		else
		{
			return;
		}
}

void monitor::rx_reverse_motor()
{
	this->apt->get_device()->reverse_motor();
}

void monitor::rx_go_active()
{
	this->apt->get_device()->start_motor();
	this->apt->set_un32_pulses_on_screen(0);
	this->apt->get_state()->next = Active;
	this->apt->get_state()->current = Active;
}

void monitor::rx_test_read()
{
	if (this->tkn_i > 1)
	{
		uint8_t cs_num = *(this->tokens[1]) - '0';

		uint32_t t_snum = this->apt->get_mem()->read(cs_num, 0x00FFFFA);
		uint32_t t_snum_v = this->apt->get_mem()->read(cs_num, 0x00FFFF6);
		uint32_t t_date = this->apt->get_mem()->read(cs_num, 0x00FFFF2);
		uint32_t t_date_v = this->apt->get_mem()->read(cs_num, 0x00FFFEE);
		uint32_t t_max = this->apt->get_mem()->read(cs_num, 0x00FFFEA);
		uint32_t t_max_v = this->apt->get_mem()->read(cs_num, 0x00FFFE6);

		static char ap_status[100];

		static char snum[5];
		translate_int_to_arr(t_snum, (uint8_t*)snum);
		static char date_t[5];
		translate_int_to_arr(t_date, (uint8_t*)date_t);
		static char maxi[5];
		translate_int_to_arr(t_max, (uint8_t*)maxi);

		sprintf(ap_status, "\r\n%s %08x\r\n%s %d\r\n%s %d\r\n", snum, t_snum_v, date_t, t_date_v, maxi, t_max_v);
		Serial_PutString(ap_status);
	}
}
void monitor::rx_set_serial()
{
	if (this->tkn_i > 2)
	{
		uint8_t cs_num = *(this->tokens[1]) - '0'; // try to parse into cs number
		uint32_t token_numeric = strtoul(this->tokens[2], NULL, 16);

		this->apt->get_mem()->write(cs_num, 0x00FFFFA, 0x534E554D);
		this->apt->get_mem()->write(cs_num, 0x00FFFF6, token_numeric);
		this->apt->get_mem()->write(cs_num, 0x00FFF10, 0x534E554D);
		this->apt->get_mem()->write(cs_num, 0x00FFF14, token_numeric);
	}
}
void monitor::rx_set_maxi()
{
	if (this->tkn_i > 2)
	{
		uint8_t cs_num = *(this->tokens[1]) - '0'; // try to parse into cs number
		uint32_t token_numeric = strtoul(this->tokens[2], NULL, 16);

		this->apt->get_mem()->write(cs_num, 0x00FFFEA, 0x4D415849);
		this->apt->get_mem()->write(cs_num, 0x00FFFE6, token_numeric);
	}
}

void monitor::rx_set_date()
{
	if (this->tkn_i > 2)
	{
		uint8_t cs_num = *(this->tokens[1]) - '0'; // try to parse into cs number
		uint32_t token_numeric = strtoul(this->tokens[2], NULL, 16);

		this->apt->get_mem()->write(cs_num, 0x00FFFF2, 0x44415445);
		this->apt->get_mem()->write(cs_num, 0x00FFFEE, token_numeric);
	}
}
void monitor::rx_scan()
{
	if (this->tkn_i == 3)
	{
		uint8_t cs_num = *(this->tokens[1]) - '0';
		uint32_t token_numeric = strtoul(this->tokens[2], NULL, 16);
		char cs_select[20];
		sprintf(cs_select, "CS [%d]\r\n", cs_num);
		Serial_PutString(cs_select);
		this->apt->get_mem()->read256(cs_num, token_numeric);
	}
}

void monitor::rx_scan(uint8_t cs_num, uint32_t address)
{
	this->apt->get_mem()->read256(cs_num, address);
}

void monitor::rx_get_man_id()
{
	if (this->tkn_i > 1)
	{
		uint8_t cs_num = *(this->tokens[1]) - '0'; // try to parse into cs number
		this->apt->get_mem()->check_connections_with_printback(cs_num);
	}
}

void monitor::rx_get_man_id_check()
{
	if (this->tkn_i > 1)
	{
		int valids = 0;
		uint8_t cs_num = *(this->tokens[1]) - '0';  // try to parse into cs number
		for(int i = 0 ; i < 10000 ; i++)
		{
			this->apt->get_mem()->check_connections(cs_num);
			if (cs_num == 3)
			{
				if (this->apt->is_b_am_valid())
				{
					valids++;
				}
			}
			if (cs_num == 2)
			{
				if (this->apt->is_b_apt_valid())
				{
					valids++;
				}
			}
			if (cs_num == 1)
			{
				if (this->apt->is_b_mcu_valid())
				{
					valids++;
				}
			}
		}
		sprintf((char*)BUFFER_RX, "CS [%d] valid [%d] out of 10000\r\n", cs_num, valids);
		Serial_PutString((char*)BUFFER_RX);
	}
	
}
void monitor::rx_find_pulses()
{
	if (this->tkn_i > 0)
	{
		uint32_t time_delay = HAL_GetTick();
		this->apt->get_mem()->Max_search_Am(End_4k_BORDER,START_BLOCK_ADD_AM);
		uint32_t pulse_count = this->apt->get_un32_pulses();
		char* pulse_str = (char*)BUFFER_RX;
		sprintf(pulse_str, "\r\nFound: %lu pulses in %lu time", pulse_count, HAL_GetTick() - time_delay);
		Serial_PutString(pulse_str);
	}
}

void monitor::rx_upload()
{
	Main_Menu();
}
void monitor::rx_debug()
{
	this->apt->get_state()->next = UartDebug;
	this->apt->get_state()->current = UartDebug;
}

void monitor::rx_exit()
{
	NVIC_SystemReset();
}

void monitor::rx_help()
{
	const static char help_str[] =
			"\r\n======================================================================"
			"\r\n= Registered functions::"
			"\r\n= help, adc_read, getid, scan, rd, wrt, debug, pwr_on, pwr_off, erase="
			"\r\n= nuke, motoron, motoron, reverse, pass, swtarwars, dump,puapt       ="
			"\r\n= snum,date,maxi, testread                      - header settters    ="
			"\r\n======================================================================"
			"\r\n\r\n";
	Serial_PutString(help_str);
}
void monitor::rx_pass()
{
	if (this->tkn_i > 0)
	{
		this->apt->get_screen()->pass_to_ard((uint8_t*)this->tokens[1]);
	}
}

void monitor::rx_adc()
{
	this->apt->get_adc()->read();
	this->apt->get_adc()->convert_to_volt();
	this->apt->get_screen()->update_battery(this->apt->get_adc()->battery_volt, this->apt->get_un32_battery_percent());
	this->apt->get_screen()->update_pressure(this->apt->get_n16_pressure());
	this->apt->get_screen()->update_piazo(this->apt->is_b_motor_running());
}

void monitor::rx_status_reg_read()
{
	this->apt->get_device()->read();
	this->apt->get_device()->StatusReg->print_current();

}
void monitor::rx_read_back()
{
	this->apt->get_mem()->read_back_current_cs();
}


void monitor::Config_Apt_Mem_Var()
{   
	uint32_t i = 0;
	uint32_t Value = 0;
	uint32_t Address = 0;
	if (this->tkn_i > 1)
	{
		Value = strtoul(this->tokens[1], NULL, 10);
		
	    Address = Value/100 * 4;
		for (i = 0, Value = 0; i < Address; i = i + 4, Value = Value + 100)
		{
			this->apt->get_mem()->write(2, i, Value);
		
		}
		this->apt->set_apt_pulses(Value);
	}
}

void monitor::Reading_Log_File()
{   
	static uint32_t current = -1;
	uint8_t command = 0;
	this->apt->get_mem()->LOG_FIND_LAST();
	if (current == -1)
	{
		current = this->apt->get_mem()->Last_addr;
	}
		if (this->tkn_i > 1) 
	{
		command = *(this->tokens[1]);	
		
		switch (command)
		{
		case 'p':
			{
				if (current != 0)
				{
					current = current + 16;
				}
				this->apt->get_mem()->LOG_READ_PARSE(current);
				break;	
			}
		case 'n':
			{  if (current < 1023985)
				{
					//the last address is 1024k
					current = current + 16;
				}
					this->apt->get_mem()->LOG_READ_PARSE(current);
					break;
			}
		case 'c':
			{
				this->apt->get_mem()->LOG_READ_PARSE(current);
				break;
			}
		case 'l':
			{
				current = this->apt->get_mem()->Last_addr;
				this->apt->get_mem()->LOG_READ_PARSE(current);
				break;
			}
		default:
			{
				break;
			}
		}
	}
}

void monitor::test_memo()
{
	uint8_t status = 0;
	uint16_t Am_Valid = 0, valid_read=0;
	for (int i = 0; i < 100; i++)
	{
		status = HAL_GPIO_ReadPin(SIMPLE_INTERLOCK_GPIO_Port, SIMPLE_INTERLOCK_Pin);
		if (status == 0)
		{
			HAL_GPIO_WritePin(ENABLE_POWER_12V_GPIO_Port, ENABLE_POWER_12V_Pin, GPIO_PIN_RESET);
			delay(100);
			HAL_GPIO_WritePin(ENABLE_POWER_12V_GPIO_Port, ENABLE_POWER_12V_Pin, GPIO_PIN_SET);
			delay_with_watchdog(30);
		}
				 
		this->apt->get_mem()->check_connections(AM_MEMO);
		{
			if (this->apt->is_b_am_valid())
			{
				Am_Valid++;
			}
		}
		
		}
	
	
	this->apt->get_mem()->erase(AM_MEMO);
	delay_with_watchdog(10000);
	//this->apt->get_mem()->write_register(AM_MEMO, value);

		
	uint32_t value = 0x055AA55F;
	for (uint32_t address = 0; address < 0x1000; address += 4)
	{
		this->apt->get_mem()->write(AM_MEMO, address, value);
		delay(10000);
		uint32_t read_back = this->apt->get_mem()->read(AM_MEMO, address);
		if (read_back == value)
		{
			valid_read++;
		}
	}
	sprintf((char*)BUFFER_RX, "CS [%d] valid [%d] out of 100\r\n", AM_MEMO, Am_Valid);
	Serial_PutString((char*)BUFFER_RX);
	sprintf((char*)BUFFER_RX, "read [%d] valid [%d] out of 1024\r\n", AM_MEMO, valid_read);
	Serial_PutString((char*)BUFFER_RX);
	
}
	
	
