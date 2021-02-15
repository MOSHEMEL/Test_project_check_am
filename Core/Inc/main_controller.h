#pragma once
#include "debug_facilities.h"
#include "peripherial_controller.h"
#include "adc_controller.h"
#include "common.h"
#include "mem_controller.h"
#include "state.h"
#include "screen_controller.h"


enum StateTestMachine
{
	Bring_Up = 0,
	TEST_DC,
	TEST_MCU,
};
class main_controller
{
public:
	bool Read_no_pulses_remaining();
	void write_no_pulses_remaining();
	void Write_Pulses_To_Am_Every_50();
	void init_before_while();
	void show_version();
	
	int generate_error_code();
	void get_apt_pulses();
	void set_apt_pulses(uint32_t Value);
	void init_state();
	void idle_state();
	void write_out_pulses_on_am();
	void active_state();
	uint32_t update_opto_counter();
	uint8_t added_counter();
	void qa_state();
	void Test_Status_Machine();
	void update_state();
	void update_debug();
	bool fire_is_pressed();
	void init_PF_struct(struct PF_memory* pf_memory);
	main_controller(DebugHelper*, Mem_ctrl*, PeripherialDevices*, State*, Screen_ctrl*, adc_controller*);
	~main_controller();
	bool need_to_stall_static();
	uint8_t need_to_stall_dynamic();
	void stall_error_write_back();
	void Test_DC_MACHINE();
	void Test_Mcu_Machine();
	void Test_Short_Cut();
	void Test_UART();
	uint16_t SUM_ADDED_FROM_TEST;
	
	void Send_Uart_Test_Screen();
	// GETTERS AND SETTERS for personal parameters. We should replace all (locals with getters)
	// There is also the  added benefit of visibility with the member variables (for debug)
	// although variables that can be changed elsewhere are good to keep as references. (Maybe?)
	// if done with references - they don't expire, if done with getters and setters - we encapsulate and eliminate mistakes of referencing pointers
	bool is_b_mcu_valid()
	{
		return this->b_mcu_valid_ = this->mem->MCU_MEM->id_valid;
	}
	bool get_mcu_valid()
	{
		this->b_mcu_valid_ = this->mem->MCU_MEM->id_valid;
		return this->b_mcu_valid_;
	}
	void set_b_mcu_valid(bool mcu_valid)
	{
		this->mem->MCU_MEM->id_valid = mcu_valid;
		this->b_mcu_valid_ = this->mem->MCU_MEM->id_valid;
		this->debug->Debug_fields.bits.DEBUG_MCU_OK = mcu_valid;
	}

	bool is_b_apt_valid()
	{
		return this->b_apt_valid_ = this->mem->APT_MEM->id_valid;
	}
	bool get_apt_valid()
	{
		this->b_apt_valid_ = this->mem->APT_MEM->id_valid;
		return this->b_apt_valid_;
	}
	void set_b_apt_valid(bool apt_valid)
	{
		this->mem->APT_MEM->id_valid = apt_valid;
		this->b_apt_valid_ = this->mem->APT_MEM->id_valid;
		this->debug->Debug_fields.bits.DEBUG_APT_OK = apt_valid;
	}
	bool is_b_am_valid()
	{
		return this->b_am_valid_ = this->mem->AM_MEM->id_valid;
	}
	bool get_am_valid()
	{
		this->b_am_valid_ = this->mem->MCU_MEM->id_valid;
		return this->b_am_valid_;
	}
	void set_b_am_valid(bool am_valid)
	{
		this->mem->MCU_MEM->id_valid = am_valid;
		this->b_am_valid_ = this->mem->MCU_MEM->id_valid;
		this->debug->Debug_fields.bits.DEBUG_AM_OK = am_valid;
	}
	DebugHelper* get_debug() const
	{
		return this->debug;
	}
	Mem_ctrl* get_mem() const
	{
		return this->mem;
	}
	PeripherialDevices* get_device() const
	{
		return this->device;
	}
	State* get_state() const
	{
		return this->state;
	}
	Screen_ctrl* get_screen() const
	{
		return this->screen;
	}
	adc_controller* get_adc() const
	{
		return this->adc;
	}
	uint32_t get_un32_pulses()
	{
		return this->un32_pulses_ = this->mem->AM_MEM->pulses;
	}
	void set_un32_pulses(uint32_t un32_pulses)
	{
		this->un32_pulses_ = un32_pulses;
		this->mem->AM_MEM->pulses = un32_pulses;

	}
	uint32_t get_un32_max_count()
	{
		return this->un32_max_count_ = this->mem->AM_MEM->max_count;
	}
	void set_un32_max_count(uint32_t un32_max_count)
	{
		this->un32_max_count_ = un32_max_count;
		this->mem->AM_MEM->max_count = un32_max_count;
	}
	uint32_t get_un32_pulses_on_screen() const
	{
		return this->un32_pulses_on_screen_;
	}
	void set_un32_pulses_on_screen(uint32_t un32_pulses_on_screen)
	{
		this->un32_pulses_on_screen_ = un32_pulses_on_screen;
	}

	int16_t get_n16_battery()
	{
		return this->n16_battery_ = this->adc->channels_buffer[V12].value;
	}
	void set_n16_battery(int16_t n16_battery)
	{
		this->n16_battery_ = n16_battery;
	}
	uint32_t get_un32_battery_percent()
	{
		return this->un32_battery_percent_ = this->adc->bat_percentage;
	}
	void set_un32_battery_percent(uint32_t un32_battery_percent)
	{
		this->un32_battery_percent_ = un32_battery_percent;
	}
	int16_t get_n16_pressure()
	{
		return this->n16_pressure_ = this->adc->Pressure_;
	}
	void set_n16_pressure(int16_t n16_pressure)
	{
		this->n16_pressure_ = n16_pressure;
	}
	bool is_b_motor_running() const
	{
		return this->b_motor_running_;
	}
	void set_b_motor_running(bool motor_running)
	{
		this->b_motor_running_ = motor_running;
		this->debug->Debug_fields.bits.DEBUG_MOTOR_RUNNING = motor_running;
	}
	bool is_b_pressure_ok() 
	{
		this->adc->pressure_check();
		this->debug->Debug_fields.bits.DEBUG_PRESSURE_OK = this->adc->pressure_ok;
		this->b_pressure_ok_ = this->adc->pressure_ok;
		return this->b_pressure_ok_;
	}
	void set_b_pressure_ok(bool pressure_ok)
	{
		this->b_pressure_ok_ = pressure_ok;
		this->adc->pressure_ok = pressure_ok;
		this->debug->Debug_fields.bits.DEBUG_PRESSURE_OK = pressure_ok;
	}
	bool is_b_is_counting_up() const
	{
		return this->b_is_counting_up_;
	}
	void set_b_is_counting_up(bool is_counting_up)
	{
		this->b_is_counting_up_ = is_counting_up;
	}
	uint16_t read_interlock(void)
	{
		static uint16_t history = 0xff;
		GPIO_PinState status = HAL_GPIO_ReadPin(SIMPLE_INTERLOCK_GPIO_Port, SIMPLE_INTERLOCK_Pin);
		history = history << 1;
		history |= (uint16_t)status;
		return history;
	}
	
	bool is_b_is_interlocked()
	{
		uint16_t history = read_interlock();
		if (history == 0x00)
		{
			if (!this->b_is_interlocked_)
			{
				HAL_GPIO_WritePin(ENABLE_POWER_12V_GPIO_Port, ENABLE_POWER_12V_Pin, GPIO_PIN_SET);
				this->b_is_interlocked_ = true;
			}
		}
		else if(history == 0xffff)
		{
			this->get_device()->stop_motor_after_run();
			if (this->b_is_interlocked_)
			{
				HAL_GPIO_WritePin(ENABLE_POWER_12V_GPIO_Port, ENABLE_POWER_12V_Pin, GPIO_PIN_RESET);
				this->b_is_interlocked_ = false;
				this->get_debug()->long_beep();
			}
		}
		return this->b_is_interlocked_;
	}
	void set_b_is_interlocked(bool is_interlocked)
	{
		this->b_is_interlocked_ = is_interlocked;
	}
	uint32_t get_un32_mcu_sn()
	{
		return this->un32_mcu_sn_ = this->mem->MCU_MEM->serial_number;
	}
	void set_un32_mcu_sn(uint32_t un32_mcu_sn)
	{
		this->un32_mcu_sn_ = un32_mcu_sn;
		this->mem->MCU_MEM->serial_number = un32_mcu_sn;
	}
	uint32_t get_un32_apt_sn()
	{
		return this->un32_apt_sn_ = this->mem->APT_MEM->serial_number;
	}
	void set_un32_apt_sn(uint32_t un32_apt_sn)
	{
		this->un32_apt_sn_ = un32_apt_sn;
		this->mem->APT_MEM->serial_number = un32_apt_sn;
	}
	uint32_t get_un32_am_sn()
	{
		return this->un32_am_sn_ = this->mem->AM_MEM->serial_number;
	}
	void set_un32_am_sn(uint32_t un32_am_sn)
	{
		this->un32_am_sn_ = un32_am_sn;
		this->mem->AM_MEM->serial_number = un32_am_sn;
	}
	uint32_t get_un32_cursor()
	{
		return this->un32_cursor_ = this->mem->AM_MEM->cursor;
	}
	void set_un32_cursor(uint32_t un32_cursor)
	{
		this->un32_cursor_ = un32_cursor;
		this->mem->AM_MEM->cursor = un32_cursor;
	}
	statesMachine get_current_state()
	{
		this->current_state_ = this->state->current;
		this->debug->Debug_fields.bits.DEBUG_STATE_1 = (this->state->current >> 0) & 0x1;
		this->debug->Debug_fields.bits.DEBUG_STATE_2 = (this->state->current >> 1) & 0x1;
		this->debug->Debug_fields.bits.DEBUG_STATE_3 = (this->state->current >> 2) & 0x1;
		return this->state->current;
	}
	void get_all();
	uint32_t last_saved_50;		
	uint32_t last_written_pulse_count_;
	uint32_t virtual_counter;
	uint32_t last_time_fire;
private:

	// NOTE TO SELF:
	// For the n-th time, we need the pointers so we can have visibility to the variables as they change.
	// The private variables are referenced by value. But the changes will be seen by reference.
	// Since it is private, we don't mind it.
	char rx_char;
	char UartBuffer[4];
	uint32_t un32_pulses_on_screen_;
	bool* mcu_valid;
	bool  b_mcu_valid_;
	bool* apt_valid;
	bool b_apt_valid_;
	bool* am_valid;
	bool b_am_valid_;
	int16_t* battery;
	int16_t n16_battery_;
	uint32_t* battery_percent;
	uint32_t un32_battery_percent_;
	int16_t* pressure;
	int16_t n16_pressure_;
	bool* motor_running;
	bool b_motor_running_;
	bool* pressure_ok;
	bool b_pressure_ok_;
	bool b_is_counting_up_;
	bool b_is_interlocked_;
	byte8_t_reg_status* status;
	uint32_t* mcu_sn;
	uint32_t un32_mcu_sn_;
	uint32_t* apt_sn;
	uint32_t un32_apt_sn_;
	uint32_t* am_sn;
	uint32_t un32_am_sn_;
	uint32_t* cursor;
	uint32_t un32_cursor_;
	DebugHelper* debug;
	Mem_ctrl* mem;
	PeripherialDevices* device;
	enum StateTestMachine STMachine  ;
	State* state;
	Screen_ctrl* screen;
	adc_controller* adc;
	uint32_t* pulses;
	uint32_t un32_pulses_;
	uint32_t un32_apt_pulses_;
	uint32_t* max_count;
	uint32_t un32_max_count_;
	statesMachine current_state_;
	volatile uint8_t last_opto_ammount_;
};
typedef enum
{
	MAX_PULSE_TEST=0,
	PRESSURE_TEST,
	TEMPERATURE_TEST,
	INTERLOCK_TEST,
	AM_FAIL_TEST,
	V12_TEST
}STALL_TEST;

typedef enum
{
	TEST_INTELOCK = 0,
	START_TEST_MOTOR,
	TEST_PRESSURE,
	TEST_COUNTER,
	TEST_WRITE_AM_MEMORY,
	TEST_FIRE_BUTTON,
	IDLE
}DC_TEST;


typedef enum
{
	VOLT_2 = 0,
	VOLT_4,
	VOLT_6,
    NO_ENABLE_PRESS,
	TEST_PASS,
	TEST_FAIL,
}BRING_BOARD;

typedef enum
{
	TEST_INTELOCK_MCU = 0,
	AM_TEST_MCU,
	APT_MEM,
	TEST_RESET_BUTTON,
	TEST_SELF_UART,
	NO_ACTIVITY,
}MCU_TEST;
