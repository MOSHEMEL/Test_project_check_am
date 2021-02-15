#pragma once
#include "hardware.h"

class STATUS_reg
{
	// TODO: convert to singleton?!
public :
	byte8_t_reg_status current;
	STATUS_reg();
	~STATUS_reg();
	uint8_t get_value(void);
	uint8_t nibble_reverse(uint8_t val);
	void print_current(void);
};

