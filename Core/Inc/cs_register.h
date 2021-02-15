#pragma once
#include "stm32f0xx_hal.h"
#include "hardware.h"

class CS_reg
{
	// TODO: convert to singleton?!
public:
	byte8_t_reg_cs current;
	byte8_t_reg_readback readback_register;
	bool cs1_value;
	bool cs2_value;
	bool cs3_value;
	bool cs_a2d_value;
	bool cs_clear_opto_value;
	bool cs_status_value;
	bool cs_ENA_value;
	bool cs_ENB_value;
	CS_reg();
	~CS_reg();
	void print_current(void);
	void print_current_new();
	void cs1(GPIO_PinState);
	void cs2(GPIO_PinState);
	void cs3(GPIO_PinState);
	void cs_status(GPIO_PinState);
	void cs_a2d(GPIO_PinState);
	void cs_clear_opto_counter(GPIO_PinState status); // no tested yet
	void motor_cmd(bool state);
	void motor_cmd_tight(bool state);
	void motor_cmd_reverse();
	void read_back_current_cs(void);
	void cs_generic(GPIO_PinState status, cs_decoder picked_cs);
private:
	void pull_chipselet_line(GPIO_PinState status);
	void set_value(uint8_t);
	void store_register(void);
};

