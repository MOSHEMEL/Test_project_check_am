#pragma once
#include <stdint.h>

#include "main.h"
#include "hardware.h"

class DebugHelper
{
public:
	bool pressure_ok;
	bool mcu_ok;
	bool apt_ok;
	bool am_ok;
	bool motor_run;
	byte8_t_debug Debug_fields;
	DebugHelper();
	~DebugHelper();
	void fast_beeps();
	void fast_beeps_2();
	void fast_beeps_3();
	void long_beep_2();
	void long_beep();
	void update(void);
	void update(uint32_t value);
	void update(bool motor, bool pressure, bool mcu, bool apt, bool am);
	void update_state(uint8_t value);
	void show(uint32_t value);
};
