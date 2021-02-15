#pragma once
#include "main.h"
#include "cs_register.h"
#include "hardware.h"

//!  ADC controller interface class. 
/*!
  The adc is MCP3008-I/SL.
  Datasheet included in ./Doxygen/misc
  
								VIN �  1024 
  Digital Output Code	= --------------------------
									VREF

	VIN = analog input voltage
	VREF = reference voltage
*/
class adc_controller
{
public:
	uint32_t dma_ring_buffer[DMA_LENGTH];
	channel channels_buffer[8];
	uint32_t current_sense;
	uint32_t temperature_mcu;
	uint32_t vref;
	uint32_t vbat;
	CS_reg* cs_register;
	uint32_t battery_volt;
	uint32_t bat_percentage;
	bool stall_pressure;
	bool pressure_ok;
	uint16_t V12_;
	uint16_t V_Motor_;
	uint16_t Motor_current_sense_;
	float Pressure_;
	uint16_t Undefined_;
	uint16_t Temp_;
	uint16_t V5_;
	uint16_t VDD_sense_;
	int temperature_celsius;
	int max_current;
	adc_controller(CS_reg*);
	~adc_controller();
	void read(enum select_channel);
	void read();
	uint16_t serial_comm_to_adc(select_channel channel_);
	uint32_t convert_to_volt();
	void pressure_check();
	uint32_t convert_to_psi();
	int32_t convert_to_celsius();
};
