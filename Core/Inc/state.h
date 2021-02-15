#pragma once
#include <stdint.h>
#include "hardware.h"

class State
{
public:
	enum statesMachine current; /**< Currently in this state. */
	enum statesMachine next; /**< Switch to this state next iteration. */
	bypass_state bypass;
	uint32_t battery;
	uint32_t pressure;
	uint32_t pulses_to_screen;
	State();
	~State();
	void init_first_step(void);

private:
	void read_interlock();
	volatile bool LOGIC_BYPASS_1;
	volatile bool LOGIC_BYPASS_2;
	volatile bool LOGIC_BYPASS_3;
	volatile bool LOGIC_BYPASS_4;

};
