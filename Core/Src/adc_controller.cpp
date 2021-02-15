#include "hardware.h"
#include "adc_controller.h"

#include <string.h>

#include "common.h"
#include "spi.h"

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

adc_controller::adc_controller(CS_reg* cs_reg)
{
	this->cs_register = cs_reg;
	memset(this->dma_ring_buffer, 0, sizeof(uint8_t)*DMA_LENGTH*4);
	memset(this->channels_buffer, 0, sizeof(int16_t)* 8);
	this->current_sense = 0;
	this->temperature_mcu = 0;
	this->vref = 0;
	this->vbat = 0;
	this->battery_volt = 0;
	this->bat_percentage = 0;
	this->stall_pressure = false;
	this->pressure_ok = false;
}

adc_controller::~adc_controller()
{
}

void adc_controller::read()
{
	static uint32_t AVG_V12 = 0;
	//char avg_output[12];
	static float AVG_Pressure = 0;
	static uint32_t AVG_TEMP = 0;
	static uint32_t AVG_CURRENT = 0;
	bool is_value_read_ok = true;
	//HAL_GPIO_TogglePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin);
	
	for (uint8_t i = 0; i < 8; i++)
	{
		HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin); // Add watchdog toggling to make sure we don't reset
		enum select_channel channel_ = (select_channel)(i);  					// NOLINT(modernize-use-auto)
		this->read(channel_);
		HAL_Delay(3);
		if (this->channels_buffer[i].value == 2047)
		{
			if (i != Undefined)
			{
				is_value_read_ok = false;	
			}
			
		}
	}
	if (!is_value_read_ok)
	{
		Serial_PutString("Wrong values adc \r\n");
	}
	
#ifdef DEBUG1
	sprintf(avg_output, "RAW_VALUE: %d\r\n", this->channels_buffer[Temp].value);
	Serial_PutString(avg_output);
#endif // DEBUG
	if ((this->channels_buffer[V12].value<2047))
	{
		this->V12_ = this->channels_buffer[V12].value;
		AVG_V12 = AVG_V12 * 3 / 4 + this->V12_/4;
		this->V12_ = AVG_V12;

	}
	if (this->channels_buffer[V_Motor].value < 2047)
	{
		this->V_Motor_ = this->channels_buffer[V_Motor].value;
	}
	if (this->channels_buffer[Motor_current_sense].value < 600)
	{
		this->Motor_current_sense_ = this->channels_buffer[Motor_current_sense].value;
		AVG_CURRENT = 3*AVG_CURRENT/4 + this->Motor_current_sense_/4;
		this->Motor_current_sense_ = AVG_CURRENT;	
#ifdef DEBUG2
		sprintf(avg_output, "RAW_CUR: %d\r\n", this->channels_buffer[Motor_current_sense].value);
		Serial_PutString(avg_output);
		sprintf(avg_output, "AVG_CURR: %d\r\n", AVG_CURRENT);
		Serial_PutString(avg_output);
		
#endif // DEBUG
	}
	
	if(this->channels_buffer[Pressure].value < 1500)
	{
		this->Pressure_ = this->channels_buffer[Pressure].value;
		AVG_Pressure = 3*(float)(AVG_Pressure)/4  + (float)this->Pressure_/4 ;
		this->Pressure_ = AVG_Pressure;
		
	}
	this->Undefined_ = this->channels_buffer[Undefined].value;

	if (this->channels_buffer[Temp].value < 2000)
	{
		
		this->Temp_ = this->channels_buffer[Temp].value;
		AVG_TEMP = AVG_TEMP * 15/ 16+ this->Temp_ /16;
		this->Temp_ = AVG_TEMP;
#ifdef DEBUG1
		sprintf(avg_output, "AVG: %d\r\n", AVG_TEMP);
		Serial_PutString(avg_output);
#endif // DEBUG
	}
	if (this->channels_buffer[V5].value < 2047)
	{
		this->V5_ = this->channels_buffer[V5].value;
	}
	if (this->channels_buffer[VDD_sense].value < 2047)
	{
		this->VDD_sense_ = this->channels_buffer[VDD_sense].value;
	}
}

uint16_t adc_controller::serial_comm_to_adc(select_channel channel_)
{
	uint8_t txBuffer[3];
	uint8_t rxBuffer[3];
	txBuffer[0] = 0x01;                                                  // Start Bit
	txBuffer[1] = (((uint8_t)(channel_) << 4) & 0xf0)| 0x80; 						 // First bit is 1 since we do Single-ended conversion. And then 3 bits of channel
	txBuffer[2] = this->cs_register->current.value;					 						 // Last bit is Don't Care
	HAL_SPI_TransmitReceive(&hspi1, txBuffer, rxBuffer, 3,10);
	return (((uint16_t)rxBuffer[1]  & 0x07)<<8) + ((uint16_t)rxBuffer[2]  & 0xff);
}

void adc_controller::read(enum select_channel channel_)
{
	uint16_t adc_value = 0;
	for (int i = 0; i < MAX_READ_ADC_CYCLES; i++)
	{
		/*
		 * In order to avoid analog noise, we just average MAX_READ_ADC_CYCLES of the data (currently 5)
		 * In the future, we can just toggle cs line to low. Because we push into cs register back the data that we had.
		 */
		this->cs_register->cs_a2d(LOW);   // We have a problem here selecting the CS
		delay(10);
		adc_value += serial_comm_to_adc(channel_);
		this->cs_register->cs_a2d(HIGH);	
		delay(10);
	}
	this->channels_buffer[channel_].value = (adc_value / MAX_READ_ADC_CYCLES);
}

uint32_t adc_controller::convert_to_volt()
{
	// This function gives volt in millie volts
	this->battery_volt =  (this->V12_ * 20 - 100);
	this->bat_percentage = (this->battery_volt - 9000) / 30;
	if (this->battery_volt < 9000)
	{
		this->bat_percentage = 0;
	}
	else if (this->battery_volt > 12000)
	{
		this->bat_percentage =  100;
	}
	return this->bat_percentage;
}

void adc_controller::pressure_check()
{
	if (this->Pressure_ > ADC_Presure_24_bar)
	{
		this->pressure_ok = true;
		this->stall_pressure = false;
	}
	else if (this->Pressure_ < ADC_Presure_20_bar)
	{
		this->pressure_ok = false;
		this->stall_pressure = true;
	}
	else
	{
		this->pressure_ok = false;
		this->stall_pressure = false;	
	}

}

uint32_t adc_controller::convert_to_psi()
{
	return ((this->Pressure_ - ADC_OFFSET) * 69 / 2048);
}

int32_t adc_controller::convert_to_celsius()
{
	
	int volt_Temp  = this->Temp_;
	this->temperature_celsius = (volt_Temp * 13 - 1000);
	return (this->temperature_celsius);
}