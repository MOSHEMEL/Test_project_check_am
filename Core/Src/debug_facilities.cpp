#include "debug_facilities.h"
#include "main.h"
#include "common.h"

extern TIM_HandleTypeDef htim14;
extern uint16_t num_beeps_need_play;
extern uint16_t buzz_time1;
extern uint16_t buzz_time2;
DebugHelper::DebugHelper()
{
	this->am_ok = false;
	this->apt_ok = false;
	this->mcu_ok = false;
	this->pressure_ok = false;
	this->motor_run = false;
}

DebugHelper::~DebugHelper()
{
}

void DebugHelper::fast_beeps()
{
	//HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, (GPIO_PinState)true);
	num_beeps_need_play = 4;
	buzz_time1 = 25;
	buzz_time2 = 25;
	TIM14->ARR = buzz_time1;////till enterning first time
	HAL_TIM_Base_Start_IT(&htim14);
}
void DebugHelper::fast_beeps_2()
{  // HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, (GPIO_PinState)true);
	num_beeps_need_play = 4;
	buzz_time1 = 125;
	buzz_time2 = 25;
	TIM14->ARR = buzz_time1; ////till enterning first time
	HAL_TIM_Base_Start_IT(&htim14);
}
void DebugHelper::fast_beeps_3()
{
	//HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, (GPIO_PinState)true);
	num_beeps_need_play = 4;
	buzz_time1 = 25;
	buzz_time2 = 125;
	TIM14->ARR = buzz_time1;  ////till enterning first time
	HAL_TIM_Base_Start_IT(&htim14);
}
void DebugHelper::long_beep()
{
	//HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, (GPIO_PinState)true);
	num_beeps_need_play = 1;
	buzz_time1 = 250;//
	buzz_time2 = 25;
	TIM14->ARR = buzz_time1; //till enterning first time
	HAL_TIM_Base_Start_IT(&htim14);
//	HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, (GPIO_PinState)true);
//	delay_with_watchdog(250);
//	HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, (GPIO_PinState)false);
//	delay_with_watchdog(25);
}
void DebugHelper::long_beep_2()
{
	num_beeps_need_play = 1;
	buzz_time1 = 2000; //
	buzz_time2 = 1;
	TIM14->ARR = buzz_time1;  //till enterning first time
	HAL_TIM_Base_Start_IT(&htim14);
}
void DebugHelper::update(bool motor, bool pressure, bool mcu, bool apt, bool am)
{
	this->Debug_fields.bits.DEBUG_AM_OK = am;
	this->Debug_fields.bits.DEBUG_APT_OK = apt;
	this->Debug_fields.bits.DEBUG_MCU_OK = mcu;
	this->Debug_fields.bits.DEBUG_PRESSURE_OK = pressure;
	this->Debug_fields.bits.DEBUG_MOTOR_RUNNING = motor;

	this->show((uint32_t)this->Debug_fields.value);
}

void DebugHelper::update_state(uint8_t value)
{
	HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_1_Pin, (GPIO_PinState)(~value & (0x1 << 0)));
	HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_2_Pin, (GPIO_PinState)(~value & (0x1 << 1)));
	HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_3_Pin, (GPIO_PinState)(~value & (0x1 << 2)));
}
void DebugHelper::update()
{
	HAL_GPIO_WritePin(DEBUG_AM_OK_GPIO_Port, DEBUG_AM_OK_Pin, (GPIO_PinState)~this->Debug_fields.bits.DEBUG_AM_OK);
	HAL_GPIO_WritePin(DEBUG_APT_OK_GPIO_Port, DEBUG_APT_OK_Pin, (GPIO_PinState)~this->Debug_fields.bits.DEBUG_APT_OK);
	HAL_GPIO_WritePin(DEBUG_MCU_OK_GPIO_Port, DEBUG_MCU_OK_Pin, (GPIO_PinState)~this->Debug_fields.bits.DEBUG_MCU_OK);
	HAL_GPIO_WritePin(DEBUG_PRESSURE_OK_GPIO_Port, DEBUG_PRESSURE_OK_Pin, (GPIO_PinState)~this->Debug_fields.bits.DEBUG_PRESSURE_OK);
	HAL_GPIO_WritePin(DEBUG_MOTOR_RUNNING_GPIO_Port, DEBUG_MOTOR_RUNNING_Pin, (GPIO_PinState)~this->Debug_fields.bits.DEBUG_MOTOR_RUNNING);
}

void DebugHelper::update(uint32_t value)
{
	this->Debug_fields.value = (uint8_t)(value & 0xff);
	this->show(value);
}

void DebugHelper::show(uint32_t value)
{
	value = ~value;
	HAL_GPIO_WritePin(DEBUG_AM_OK_GPIO_Port, DEBUG_AM_OK_Pin, (GPIO_PinState)(value & (0x1 << 0)));
	HAL_GPIO_WritePin(DEBUG_APT_OK_GPIO_Port, DEBUG_APT_OK_Pin, (GPIO_PinState)(value & (0x1 << 1)));
	HAL_GPIO_WritePin(DEBUG_MCU_OK_GPIO_Port, DEBUG_MCU_OK_Pin, (GPIO_PinState)(value & (0x1 << 2)));
	HAL_GPIO_WritePin(DEBUG_PRESSURE_OK_GPIO_Port, DEBUG_PRESSURE_OK_Pin, (GPIO_PinState)(value & (0x1 << 3)));
	HAL_GPIO_WritePin(DEBUG_MOTOR_RUNNING_GPIO_Port, DEBUG_MOTOR_RUNNING_Pin, (GPIO_PinState)(value & (0x1 << 4)));
	
}
