#include "error_controller.h"

#include "common.h"
#include "usart.h"
#include "hardware.h"

error_controller::error_controller(): error_level_output(DEBUG_LEVEL_OFF)
{
}
error_controller::~error_controller()
{
}

void error_controller::log(DEBUG_LEVEL_ debug_level, const char* string_to_print, uint16_t size)
{
	if (debug_level > this->error_level_output)
	{
		Serial_PutString(string_to_print);
		HAL_Delay(10);
		NVIC_SystemReset();
	}
}

void error_controller::on_fatal()
{
	// write to memory mcu

}
