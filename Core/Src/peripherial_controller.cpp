#include "peripherial_controller.h"

#include "common.h"
#include "spi.h"
#include "hardware.h"
#include "tim.h"

PeripherialDevices::PeripherialDevices(CS_reg* cs, STATUS_reg* status)
{
	this->CsRegister = cs;
	this->StatusReg = status;
	this->motor_running = false;
}

PeripherialDevices::~PeripherialDevices()
{
}

void PeripherialDevices::read()
{
	this->CsRegister->cs_status(LOW);
	this->StatusReg->get_value();
	this->CsRegister->cs_status(HIGH);
}

void PeripherialDevices::clear_counter()
{
	this->CsRegister->cs_clear_opto_counter(LOW);
}
uint8_t PeripherialDevices::read_counter()
{
	this->read();
	return (uint8_t)this->StatusReg->current.bits.OPTO_STATUS;
}

void PeripherialDevices::start_motor()
{
	this->clear_counter();
	this->CsRegister->motor_cmd_tight(MOTOR_RUN);
	this->motor_running = true;
	const static char motor_stat_start_c[] = "Motor Starting\r\n";
	Serial_PutString(motor_stat_start_c);
}

void PeripherialDevices::reverse_motor()
{
	this->CsRegister->motor_cmd_reverse();
}
void PeripherialDevices::stop_motor_after_run()
{
	for (int pwm_steps = 0; pwm_steps < 250; pwm_steps++)
	{
		this->CsRegister->motor_cmd_tight(MOTOR_STOP);
		delay(pwm_steps_cnt[pwm_steps]);
		this->CsRegister->motor_cmd_tight(MOTOR_RUN);
		delay(2000 - pwm_steps_cnt[pwm_steps]);
	}
	this->CsRegister->motor_cmd_tight(MOTOR_STOP);
	this->motor_running = false;
	const static char motor_stat_stop_c[] = "Motor Stopping after run\r\n";
	Serial_PutString(motor_stat_stop_c);
}

void PeripherialDevices::stop_motor_init()
{
	
	this->CsRegister->motor_cmd_tight(MOTOR_STOP);
	this->motor_running = false;
	const static char motor_stat_stop_c[] = "Motor Stopping init\r\n";
	Serial_PutString(motor_stat_stop_c);
}

void PeripherialDevices::start_motor_pwm()
{
	// SPI freq = 1.5 MBits/s
	// We need to send one Packet (8Bits) for each INA or INB (But we actually can just send packet = On and packet = Off.
	// 1.5 MHz / 8 = 187.5 KHz
	// 100 ms = 18.75 cycles
	// 2 ms  = 375 cycles
	
	
	this->CsRegister->current.value = 0;
	for(int pwm_steps=0; pwm_steps<250; pwm_steps++)
	{
		this->CsRegister->motor_cmd_tight(MOTOR_RUN);
		delay(pwm_steps_cnt[pwm_steps]);
		this->CsRegister->motor_cmd_tight(MOTOR_STOP);
		delay(2000 - pwm_steps_cnt[pwm_steps]);
	}

	this->CsRegister->motor_cmd_tight(MOTOR_RUN);
	this->motor_running = true;
	const static char motor_stat_start_pwm_c[] = "Motor Starting PWM\r\n";
	Serial_PutString(motor_stat_start_pwm_c);
}

bool PeripherialDevices::toggle_motor()
{
	if (this->motor_running)
	{
		this->stop_motor_after_run();
	}
	else
	{
		this->start_motor();
	}
	return this->motor_running;
}
bool PeripherialDevices::read_fire_button_press()
{
	this->read();
	return this->StatusReg->current.bits.FIRE_STATUS;
}
//const int c = 261;
//const int d = 294;
//const int e = 329;
const int f = 349;
//const int g = 391;
const int gS = 415;
const int a = 440;
const int aS = 455;
const int b = 466;
const int cH = 523;
const int cSH = 554;
const int dH = 587;
const int dSH = 622;
const int eH = 659;
const int fH = 698;
const int fSH = 740;
const int gH = 784;
const int gSH = 830;
const int aH = 880;

void tone(int note, int duration)
{
	uint32_t _beep_duration = (500000) / note  + 1; // 50% duty cycle time of up and down
	__HAL_TIM_SET_COUNTER(&htim3, 0);
	{
		while (__HAL_TIM_GET_COUNTER(&htim3) < duration)
		{
			HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
			delay(_beep_duration);
			HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
			delay(_beep_duration);
		}
	}
}

void noTone()
{
	HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
}

void beep(int note, int duration)
{
	//Play tone on buzzerPin
	tone(note, duration);
	//Stop tone on buzzerPin
	noTone();
	delay(50);
}
 
void firstSection()
{
	beep(a, 500);
	beep(a, 500);    
	beep(a, 500);
	beep(f, 350);
	beep(cH, 150);  
	beep(a, 500);
	beep(f, 350);
	beep(cH, 150);
	beep(a, 650);
 
	HAL_Delay(500);
 
	beep(eH, 500);
	beep(eH, 500);
	beep(eH, 500);  
	beep(fH, 350);
	beep(cH, 150);
	beep(gS, 500);
	beep(f, 350);
	beep(cH, 150);
	beep(a, 650);
 
	HAL_Delay(500);
}
 
void secondSection()
{
	beep(aH, 500);
	beep(a, 300);
	beep(a, 150);
	beep(aH, 500);
	beep(gSH, 325);
	beep(gH, 175);
	beep(fSH, 125);
	beep(fH, 125);    
	beep(fSH, 250);
 
	HAL_Delay(325);
 
	beep(aS, 250);
	beep(dSH, 500);
	beep(dH, 325);  
	beep(cSH, 175);  
	beep(cH, 125);  
	beep(b, 125);  
	beep(cH, 250);  
 
	HAL_Delay(350);
}
void PeripherialDevices::star_wars()
{
	 
	//Play first section
	firstSection();
 
	//Play second section
	secondSection();
 
	beep(f, 250);  
	beep(gS, 500);  
	beep(f, 350);  
	beep(a, 125);
	beep(cH, 500);
	beep(a, 375);  
	beep(cH, 125);
	beep(eH, 650);
 
	HAL_Delay(500);
 
	//Repeat second section
	secondSection();
 

	beep(f, 250);  
	beep(gS, 500);  
	beep(f, 375);  
	beep(cH, 125);
	beep(a, 500);  
	beep(f, 375);  
	beep(cH, 125);
	beep(a, 650);  
 
	HAL_Delay(650);
}

