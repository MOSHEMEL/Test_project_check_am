#include "state.h"
#include <stdbool.h>
#include "main.h"
extern uint8_t COUNTER_V;
extern bool reset_button_instead_of_fire;

State::State()
{
	this->current = Init_before_main;
	this->next = Init;
}

State::~State()
{
	
}

void State::read_interlock()
{

	if (HAL_GPIO_ReadPin(SIMPLE_INTERLOCK_GPIO_Port, SIMPLE_INTERLOCK_Pin))
	{
		uint32_t updates_last = HAL_GetTick();
		while (HAL_GetTick() - updates_last < 100)
		{
			HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
			HAL_Delay(10);
		}
		if (!(HAL_GPIO_ReadPin(SIMPLE_INTERLOCK_GPIO_Port, SIMPLE_INTERLOCK_Pin)))
		{
			HAL_GPIO_WritePin(ENABLE_POWER_12V_GPIO_Port, ENABLE_POWER_12V_Pin, GPIO_PIN_SET);
			this->current = this->next;
		}
	}
	else
	{
		HAL_GPIO_WritePin(ENABLE_POWER_12V_GPIO_Port, ENABLE_POWER_12V_Pin, GPIO_PIN_SET);
		this->current = this->next;
	}
}

void State::init_first_step(void)
{
	while (this->current == Init_before_main)
	{

		this->LOGIC_BYPASS_1 = ((bool)HAL_GPIO_ReadPin(LOGIC_BYPASS_1_GPIO_Port, LOGIC_BYPASS_1_Pin));
		this->LOGIC_BYPASS_2 = ((bool)HAL_GPIO_ReadPin(LOGIC_BYPASS_2_GPIO_Port, LOGIC_BYPASS_2_Pin));
//		this->LOGIC_BYPASS_3 = ((bool)HAL_GPIO_ReadPin(LOGIC_BYPASS_3_GPIO_Port, LOGIC_BYPASS_3_Pin));
//		this->LOGIC_BYPASS_4 = ((bool)HAL_GPIO_ReadPin(LOGIC_BYPASS_4_GPIO_Port, LOGIC_BYPASS_4_Pin));
		this->bypass.bits.bypass1 = this->LOGIC_BYPASS_1;
		this->bypass.bits.bypass2 = this->LOGIC_BYPASS_2;
		this->bypass.bits.bypass3 = true;
		this->bypass.bits.bypass4 = true;
		this->bypass.bits.reserved = 0xf;
		this->bypass.value = ~this->bypass.value;
		
		reset_button_instead_of_fire = false;
		
		
		COUNTER_V = 2;
		this->current = this->next;
		// Debug since bypass killed the voltage reg.
		// this->bypass.value = 2; WE ARE NOW WORKING LIVE

		if ( this->bypass.value == 3)
		{
			this->read_interlock();
		}
		else if (this->bypass.value == 1)
		{
			// don't connect the apt
			this->current = Idle;
		}
		else if (this->bypass.value == 2)
		{
			// don't connect apt and start uart
			this->read_interlock();
			this->current = UartDebug;
		}

		HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);

	}
}
