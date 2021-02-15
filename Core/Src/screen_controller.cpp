#include "screen_controller.h"
#include "rtc.h"
#include <string.h>
#include  "monitor.h"
#include "common.h"
#include "usart.h"
extern uint8_t BUFFER_SCREEN[RXBUFFERSIZE];

Screen_ctrl::Screen_ctrl()
{
	this->last_remaining_message_sent = 0;
	this->last_blocking_message = 0;
	this->times_sent = 0;
	this->buffer = BUFFER_SCREEN;
	this->last_pulse_sent = 0;
	memset(this->buffer, 0, sizeof(uint8_t)*RXBUFFERSIZE);
}

Screen_ctrl::~Screen_ctrl()
{
}


void Screen_ctrl::get_time()
{
	RTC_DateTypeDef sdatestructureget;
	RTC_TimeTypeDef stimestructureget;
	HAL_RTC_GetTime(&hrtc, &stimestructureget, RTC_FORMAT_BIN);
	/* Get the RTC current Date */
	HAL_RTC_GetDate(&hrtc, &sdatestructureget, RTC_FORMAT_BIN);
	/* Display time Format : hh:mm:ss */

	
	sprintf((char*)this->buffer,
	        "Time %02d:%02d:%02d \r\n",
	        stimestructureget.Hours,
	        stimestructureget.Minutes,
	        stimestructureget.Seconds);
	Serial_PutString((char*)this->buffer);
}

void Screen_ctrl::print_logo()
{
	const static char logo[] = ""
	" NNNN                    NNNN  \r\n"
	" NNNNiiiiiiiiiiiiiiiiiiiiNNNN  \r\n"
	"  NNNNNNNNNNNNNNNNNNNNNNNNNN   \r\n"
	"             iNNi              \r\n"
	"    oooo     iNNi     oooo     \r\n"
	"    pppp     iNNi     pppp     \r\n"
	"             iNNi              \r\n"
	"             iNNi              \r\n"
	"             iNNi              \r\n"
	"             iNNi              \r\n"
	"             iNNi              \r\n"
	"             iNNi              \r\n"
	"             iNNi              \r\n"
	"             iNNi              \r\n"
	"             iNNi              \r\n";
	sprintf((char*)this->buffer, "ARMenta version: %s\r\n", VERSION);
	
	Serial_PutString(logo);
	Serial_PutString((char*)this->buffer);
}

void Screen_ctrl::update_battery(uint32_t bat_voltage, uint32_t battery)
{
	//sprintf((char*)this->buffer, "BatteryADC:%d\r\nBatteryConversion:%lu\r\n", bat_adc, battery);
	//Serial_PutString((char*)this->buffer);

	sprintf((char*)this->buffer, "$b%lu#", battery);
	Monitor_PutString((char*)this->buffer);
	if (battery < 10)
	{
		static int j= 0;
		if (j % 100 == 99)
		{
			Serial_PutString("Low Battery\r\n");
			sprintf((char*)this->buffer, "Voltage %d\r\n", bat_voltage);
			Serial_PutString((char*)this->buffer);
		}
		j++; // This will output less times
	}
}

void Screen_ctrl::update_pressure(uint32_t pressure)
{
	if(pressure >= ADC_Presure_22_bar) // pressure too low
	{
		//sprintf((char*)this->buffer, "Pr %lu [0.k]\r\n", pressure);
		//Serial_PutString((char*)this->buffer);
		sprintf((char*)this->buffer, "$pO.K#");
	}
	else
	{
		//sprintf((char*)this->buffer, "Pr %lu [low]\r\n", pressure);
		//Serial_PutString((char*)this->buffer);
		sprintf((char*)this->buffer, "$pLOW#");
	}
	Monitor_PutString((char*)this->buffer);
}

void Screen_ctrl::update_pressure_static(uint32_t pressure)
{
	if (pressure >= ADC_Presure_24_bar) // pressure too low
		{
			//sprintf((char*)this->buffer, "Pr %lu [0.k]\r\n", pressure);
			//Serial_PutString((char*)this->buffer);
			sprintf((char*)this->buffer, "$pO.K#");
		}
	else
	{
		//sprintf((char*)this->buffer, "Pr %lu [low]\r\n", pressure);
		//Serial_PutString((char*)this->buffer);
		sprintf((char*)this->buffer, "$pLOW#");
	}
	Monitor_PutString((char*)this->buffer);
}


void Screen_ctrl::update_pulse_counter(uint32_t pulses)
{
	sprintf((char*)this->buffer, "$c%lu#", pulses);
	this->last_pulse_sent = pulses;
	//Serial_PutString((char*)this->buffer);
	Monitor_PutString((char*)this->buffer);
}

void Screen_ctrl::update_pulse_counter_remaining(uint32_t pulses, uint32_t serial)
{
	sprintf((char*)this->buffer, "$h%lu#", serial);
	Monitor_PutString((char*)this->buffer);

	sprintf((char*)this->buffer, "$i%lu#", pulses);
	//Serial_PutString((char*)this->buffer);
	Monitor_PutString((char*)this->buffer);
}

void Screen_ctrl::tx_send_general_error(uint32_t reason, uint32_t debug_code)
{
	if (this->check_timer_expired())
	{
		if (reason == 404) // MCU not reachable
			{
				const char mcu_error[] = "The Mcu is Unavailable, spi mem read failed\r\n";
				Serial_PutString(mcu_error);

				// $zMCU Unavailable 4000#
				sprintf((char*)this->buffer, "$E%d#", 4000 + debug_code);
				Monitor_PutString((char*)this->buffer);
			}
		else if (reason == 405) // AP or body unreachable
			{
				// $zAPTx Disconnected 4001#
				const char apt_error[] = "The APTx is Unavailable, spi mem read failed\r\n";
				Serial_PutString(apt_error);

				sprintf((char*)this->buffer, "$E%d#", 4000 + debug_code);
				Monitor_PutString((char*)this->buffer);
			}
		else if (reason == 406) // AM
			{
				// $zAM Disconnected 4011#
				const char am_error[] = "The AM is Unavailable, spi mem read failed\r\n";
				Serial_PutString(am_error);

				sprintf((char*)this->buffer, "$E%d#", 4000 + debug_code);
				Monitor_PutString((char*)this->buffer);
			}
		else if (reason == 501)
		{
			// $zAP sn not valid#
			const char am_sn_error[] = "The AP is not initialized, please supply a valid s/n\r\n";
			Serial_PutString(am_sn_error);

			sprintf((char*)this->buffer, "$E%d#", 5000 + debug_code);
			Monitor_PutString((char*)this->buffer);

		}
		if (reason == 502)
		{
			// $zAM write error#
			const char am_write_error[] = "The AM cannot be written to\r\n";
			Serial_PutString(am_write_error);
			// TODO: add debug info when giving out the error like E6011 / E6111
			sprintf((char*)this->buffer, "$E%d#", 6000 + debug_code);
			Monitor_PutString((char*)this->buffer);
		}
	}
}

void Screen_ctrl::tx_send_general_error(uint32_t reason)
{

	if (this->check_timer_expired())
	{
		if (reason == 503)
		{
			const char apt_ammount_error[] = "Device exceeded lifetime for Maintenance, will not run anymore\r\n";
			Serial_PutString(apt_ammount_error);

			// $zContact Armenta E503#
			sprintf((char*)this->buffer, "$E503#");
			Monitor_PutString((char*)this->buffer);
		}
		else if (reason == 504)
		{
			const char apt_ammount_end_error[] = "Device near end of lifetime for Maintenance\r\n";
			Serial_PutString(apt_ammount_end_error);
			// $zContact Armenta E504#
			sprintf((char*)this->buffer, "$E504#");
			Monitor_PutString((char*)this->buffer);
		}
		else
		{
			sprintf((char*)this->buffer,"$zE%d#", reason);
			Serial_PutString((char*)this->buffer);
			Monitor_PutString((char*)this->buffer);
		}
		// Other various errors / warnings
		// $F0#
		// $F200#
		// $F1000#
	}
}

void Screen_ctrl::clean_screen(uint32_t new_pulses_to_screen)
{
	sprintf((char*)this->buffer, "$Q%lu#", new_pulses_to_screen);
	Monitor_PutString((char*)this->buffer);
}

void Screen_ctrl::blank_screen(void)
{
	sprintf((char*)this->buffer, "$R#");
	Monitor_PutString((char*)this->buffer);
}
void Screen_ctrl::test_screen_update(char* buff)
{  
	//sprintf((char*)this->buffer, "$j%s#", p);
	//sprintf((char*)this->buffer, "$j%d#", version);
	Monitor_PutString(buff);
}

void Screen_ctrl::update_version()
{  
	char version[] = VERSION;
	sprintf((char*)this->buffer, "$v%s#", version);	
	Monitor_PutString((char*)this->buffer);
}

void Screen_ctrl::update_remaining_in_percent(uint32_t max_pulses, uint32_t pulses)
{
	static uint32_t maxi = max_pulses;
	static uint32_t percent_remaining = 100;
	percent_remaining = (((maxi - pulses) * 100) / maxi);
	static uint32_t remainder_scaled = (percent_remaining * 11) / 10;
	remainder_scaled = (percent_remaining * 11) / 10;
	static uint32_t last_sent_time = HAL_GetTick();
	/*
	 *  Management wanted some buffer to have Graphics show 100% until they use up some amount of pulses
	 *  Otherwise after 1 pulse it did ceil(99.9%) and showed 99%, which was not to their liking.
	 *  So the function stretches 100% to 110% and then clips it at 100% thus giving ~9% until you
	 *  see 99%
	 */
	if (remainder_scaled > 100)
	{
		remainder_scaled = 100;
	}
	if (max_pulses > 0 && (HAL_GetTick()- last_sent_time) > 500)
	{
		sprintf((char*)this->buffer, "$t%lu#", remainder_scaled);
		Monitor_PutString((char*)this->buffer);
		last_sent_time = HAL_GetTick();
	}
	//sprintf((char*)this->buffer, "pulses=%lu, max=%lu, util=%lu\r\n", pulses, max_pulses, remainder_scaled);
	//Serial_PutString((char*)this->buffer);
}

void Screen_ctrl::update_error_remaining(uint32_t remaining)
{


	if (this->last_remaining_message_sent != remaining)
	{
		this->times_sent = 0;
		this->last_remaining_message_sent = remaining;
	}



	if (this->check_timer_expired())
	{
		// It is not another if condition, do not merge
		if((remaining > 0) && (last_remaining_message_sent == remaining))
		{
			// By saving a static variable as memory of last sent $f[count]#
			// We just send it [times_sent] times
			if(this->times_sent < AM_WARNING_TIMES)
			{
				sprintf((char*)this->buffer, "$f%lu#", remaining);
				Monitor_PutString((char*)this->buffer);
				this->times_sent++;
			}
		}
		else if(remaining == 0)
		{
			sprintf((char*)this->buffer, "$f0#");
			Monitor_PutString((char*)this->buffer);
		}
		this->last_remaining_message_sent = remaining;
	}
}

void Screen_ctrl::update_piazo(bool piazo)
{
	static uint32_t last_sent_time = HAL_GetTick();
	if ((HAL_GetTick() - last_sent_time) > 500)
		{
			
		if (piazo)
		{
			sprintf((char*)this->buffer, "$aY#");
		}
		else
		{
			sprintf((char*)this->buffer, "$aN#");
		}
		Monitor_PutString((char*)this->buffer);
		last_sent_time = HAL_GetTick();
	}
}

void Screen_ctrl::qa_mon(bool valid_mcu, bool valid_apt, bool valid_am)
{

	if (this->check_timer_expired())
	{
		sprintf((char*)this->buffer,
			"$Gcs1 %s \r\n"\
			"cs2 %s\r\n"\
			"cs3 %s\r\n",
			valid_mcu ? "true" : "false",
			valid_apt ? "true" : "false",
			valid_am ? "true" : "false");
		Monitor_PutString((char*)this->buffer);
	}
}

bool Screen_ctrl::check_timer_expired()
{
	uint32_t current_time = HAL_GetTick();
	static bool is_reset_sent = false;
	if ((current_time - this->last_blocking_message)  < (TIMEOUT_SCREEN ))
	{
		if ((current_time - this->last_blocking_message)  < (TIMEOUT_SCREEN /2))
		{
			return false;
		}
		else
		{
			// In case we are in waiting time -> we will show the count on screen - experimental
			if(!is_reset_sent)
			{
				is_reset_sent = true;
				sprintf((char*)this->buffer, "$R#");
				Monitor_PutString((char*)this->buffer);
				delay(100);
			}
			sprintf((char*)this->buffer, "$d%lu#", this->last_pulse_sent);
			//sprintf((char*)this->buffer, "$d0#");
			Monitor_PutString((char*)this->buffer);
			return false;
		}
	}
	this->last_blocking_message = HAL_GetTick();
	is_reset_sent = false;
	return true;
}

void Screen_ctrl::pass_to_ard(uint8_t* message)
{  
	
	
	sprintf((char*)this->buffer, "%s#", (char*)message);
	Serial_PutString((char*)this->buffer);
	Serial_PutString(Brk);
	Monitor_PutString((char*)this->buffer);
}
