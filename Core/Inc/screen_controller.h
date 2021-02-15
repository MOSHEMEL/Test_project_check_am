#pragma once
#include <stdint.h>
#include "hardware.h"
class Screen_ctrl
{
public:

	Screen_ctrl();
	~Screen_ctrl();
	/**
	 * @brief time output
	 *
	 * Print time from cpu start to serial
	 */
	void get_time(void);

	/**
	 * @brief print logo of armenta
	 *
	 * Its a cow
	 */
	void print_logo(void);

	/**
	 * @brief update the battery 
	 *
	 * update the battery on screen
	 */
	void update_battery(uint32_t bat_voltage, uint32_t battery);

	/**
	 * @brief update the pressure
	 *
	 * update the pressure, if the pressure is red we cannot start the motor.
	 */
	void update_pressure(uint32_t pressure);
	void update_pressure_static(uint32_t pressure);

	/**
	 * @brief update the pulses
	 *
	 * send the pulses on screen to the screen.
	 * This function has to be the quickest to respond.
	 */
	void update_pulse_counter(uint32_t pulses);
	void update_pulse_counter_remaining(uint32_t pulses, uint32_t serial);

	/**
	 * @brief send error
	 *
	 * when there is an error to show to the screen, we show a red screen and some text.
	 * added some text after the fact for me to analyze
	 */
	void tx_send_general_error(uint32_t reason, uint32_t debug_code);

	/**
	 * @brief send error
	 *
	 * when there is an error to show to the screen, we show a red screen and some text.
	 */
	void tx_send_general_error(uint32_t reason);

	/**
	 * @brief reset screen state
	 *
	 * sometimes we need to be certain the screen is cleared
	 */
	void clean_screen(uint32_t new_pulses_to_screen);
	void blank_screen();

	/**
	 * @brief show version on screen
	 *
	 * sometimes we need to be certain the screen is cleared
	 */
	void update_version(void);
	
	void test_screen_update(char* buff);

	/**
	 * @brief show percentage of AM usage on screen
	 *
	 * we take the maximum - used / maximum *100 
	 */
	void update_remaining_in_percent(uint32_t max_pulses, uint32_t pulses);

	/**
	 * @brief show warning of remaining less than 1000
	 *
	 * we show warnings at:
	 *	+ 1000
	 *	+ 800
	 *	+ 600
	 *	+ 400
	 *	+ 200
	 *	+ 0 - we stop if running and show if try to start
	 */
	void update_error_remaining(uint32_t remaining);

	/**
	 * @brief update pulse symbol on screen
	 *
	 * Currently we just show the green icon on motor running and red one when not.
	 * Should be determined by the piazo signal.
	 */
	void update_piazo(bool piazo);
	
	/**
	 * @brief update qa status to screen
	 *
	 */
	void qa_mon(bool valid_mcu, bool valid_apt, bool valid_am);

	void pass_to_ard(uint8_t* message);

private:
	uint8_t* buffer;
	uint32_t last_remaining_message_sent;
	uint8_t times_sent;
	uint32_t last_blocking_message;
	uint32_t last_pulse_sent;
	bool check_timer_expired();
};
