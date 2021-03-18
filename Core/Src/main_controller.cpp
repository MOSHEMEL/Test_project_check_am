#include "main_controller.h"

#include <string.h>


#include "common.h"
#include "monitor.h"

volatile static  bool flag_uart_test = 0;
static bool flag_test = 0;
extern volatile bool RESET_PRESSED;
extern volatile uint8_t COUNTER_V;
extern PF_memory pf_memory;
extern bool reset_button_instead_of_fire;
char writeout_str_data[20];
extern char aRxBuffer[5];	
char   aTxBuffer[] = {"test"};
main_controller::main_controller(DebugHelper* debug_, Mem_ctrl* mem_, PeripherialDevices* device_, State* state_, Screen_ctrl* screen_, adc_controller* adc_)
{
    this->debug = debug_;
    this->mem = mem_;
    this->device = device_;
    this->state = state_;
    this->screen = screen_;
    this->adc = adc_;

    // pointers in controller
    //memories:
    this->mcu_valid = &this->mem->MCU_MEM->id_valid;
    this->apt_valid = &this->mem->APT_MEM->id_valid;
    this->am_valid  = &this->mem->AM_MEM->id_valid;
    this->mcu_sn = &this->mem->MCU_MEM->serial_number;
    this->apt_sn = &this->mem->APT_MEM->serial_number;
    this->am_sn  = &this->mem->AM_MEM->serial_number;
    this->max_count = &this->mem->AM_MEM->max_count;
    this->pulses = &this->mem->AM_MEM->pulses;
    this->cursor = &this->mem->AM_MEM->cursor;

    //status:
    this->motor_running = &this->device->motor_running;
    this->status = &this->device->StatusReg->current;
    this->un32_pulses_on_screen_ = 0;
    this->b_is_counting_up_ = true;

    //adc
    this->battery = &this->adc->channels_buffer[V12].value;
    this->battery_percent = &this->adc->bat_percentage;
    this->pressure = &this->adc->channels_buffer[Pressure].value;
    this->pressure_ok = &this->adc->pressure_ok;
}

main_controller::~main_controller()
{
}

void main_controller::init_before_while()
{
    this->get_debug()->show(0);
    this->screen->print_logo();
    this->screen->update_version();	
    this->state->init_first_step();
    this->get_debug()->show(32);
}

void main_controller::show_version()
{
	//this->get_debug()->show(0);
	//this->screen->print_logo();
	this->screen->update_version();	
	//this->state->init_first_step();
	//this->get_debug()->show(32);
}


int main_controller::generate_error_code()
{
    // We add to error code a supllement, we add the statuses
    return ((int)this->is_b_am_valid() * 100 + (int)this->is_b_apt_valid() * 10 + (int)this->is_b_mcu_valid());  // We have a problem with mcu, so we will just pass mcu_b_valid
}
void main_controller::get_apt_pulses()
{
    this->un32_apt_pulses_ = 0;
    for (uint32_t i = 0; i < 50000; i += 4)
    {
        uint32_t temp_read_value = this->get_mem()->read(2, i);
        if (temp_read_value != 0xffffffff)
        {
            this->un32_apt_pulses_ = temp_read_value;
        }
        else
        {
            break;
        }
    }
}

void main_controller::set_apt_pulses(uint32_t Value)
{
	this->un32_apt_pulses_ = Value;
		
}
void main_controller::init_state()
{  
	    
	this->state->next = Init;
	HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
	HAL_Delay(25);
	static const char init_stat[] = "Start Init\r\n";
	static const char init_stat_2[] = "Start Init checked connections\r\n";
    
	char temp_pulses_str[40];

	Serial_PutString(init_stat);
	this->get_device()->stop_motor_init();
	this->mem->check_connections();

	this->is_b_am_valid();
	this->is_b_apt_valid();
	this->is_b_mcu_valid();

	HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
	HAL_Delay(25);
	Serial_PutString(init_stat_2);
	
	this->update_debug();
	
	
    
	if (this->state->bypass.value == 4)
	{
		this->state->next = QA;
	}
	else if (this->state->bypass.value == 8)
	{
		// In this state we check connections and connect apt and then pass uart state
		this->state->next = UartDebug;
	}
	else
	{
		static const char mem_stat[] = "Reading memory contents\r\n";
		Serial_PutString(mem_stat);

		// EVERYTHING IS O.K
		if(this->is_b_apt_valid() && this->is_b_am_valid()) // currently we should bypass MCU check when apt and am connected
		{
			HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);  // This will make the tone stop
			this->mem->get_serial();
			this->mem->get_max();
			this->mem->get_date();
			//finding_max in the flash 
			this->mem->Max_search_Am(End_4k_BORDER,START_BLOCK_ADD_AM);
			
			this->get_apt_pulses();
            
			// Init step 2.
			uint32_t sn_am = this->get_un32_am_sn();
			uint32_t maximum_am = this->get_un32_max_count();
			if (sn_am == 0xffffffff || sn_am == 0x00000000 || maximum_am == 0xffffffff || maximum_am == 0)
			{
				this->screen->tx_send_general_error(501, this->generate_error_code());
				this->screen->update_version();
				return;
			}
			if (this->get_un32_pulses() >= maximum_am)
			{
				this->screen->update_error_remaining(0);
				sprintf(temp_pulses_str,
					"Line 145 p %d\r\nc %d\r\nm %d\r\n",
					(int)(this->un32_pulses_),
					(int)(this->un32_cursor_),
					(int)(this->un32_max_count_));
				Serial_PutString(temp_pulses_str);
				this->screen->update_version();
				return;
			}
			this->last_written_pulse_count_ = this->get_un32_pulses();
			this->get_un32_cursor();
			// Now lets try to write the same data again
		while(this->get_mem()->read(3, this->get_un32_cursor()) != 0xffffffff)
			{
				this->set_un32_cursor(this->get_un32_cursor() + 4);
				if (this->get_un32_cursor() >= End_4k_BORDER)
				{
					Serial_PutString("reached end of cursor space looking for free place\r\n");
					this->get_screen()->tx_send_general_error(666);   // couldn't write
					this->set_un32_cursor(0);
					break;
					
				}
			}
			if (this->get_un32_cursor() != 0)  //in case of getting the last address of 4k  the cursor become zero
			{
				if (this->get_mem()->write(3, this->un32_cursor_, this->un32_pulses_) != HAL_OK) 
				{
					Serial_PutString("encountered problem writing to memory on startup\r\n");
					this->get_screen()->tx_send_general_error(666);   // couldn't write
				}
			}
			this->get_adc()->read();
			if (this->get_adc()->V12_ == 0)
			{
				Serial_PutString("Problem reading ADC 12V channel\r\n");
				this->get_screen()->tx_send_general_error(667);   // the adc is bad
			}
			this->state->next = Idle;

			// UnLock Registers to memory
			this->get_mem()->print_register(3);
			this->get_mem()->write_register(3, 0x8001);
			this->get_mem()->print_register(3);
			// wait with watchdog
			delay_with_watchdog(2000);
            
            
			// Otherwise - we just show the remaining
			//this->screen->update_pulse_counter(this->un32_max_count_  - this->un32_pulses_);
			uint32_t serial_number = this->get_mem()->get_serial();
			this->screen->update_pulse_counter_remaining(this->un32_max_count_  - this->un32_pulses_, serial_number);

			delay_with_watchdog(5000);

			for (int j = 0; j < 2; j++)
			{
				this->get_screen()->blank_screen();
				delay(100);
			}
			this->init_PF_struct(&pf_memory);
			Serial_PutString("Done Init\r\n");

			return;
		}
        
		if (!this->is_b_mcu_valid())
		{
			// MCU IS INVALID - THROW ERROR ONCE
			this->screen->tx_send_general_error(404, this->generate_error_code());
			this->screen->update_version();
		}

        
		if (!this->is_b_apt_valid())
		{
			// APTX DISCONNECTED INIT NOT DONE
			this->screen->tx_send_general_error(405, this->generate_error_code());
			HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);  // If there is no aptx - we make a tone until its connected to annoy the shit out of the shirut guys
			this->screen->update_version();
			return;
		}
        
		if (!this->is_b_am_valid())
		{
			// AM DISCONNECTED INIT NOT DONE
			this->screen->tx_send_general_error(406, this->generate_error_code());
			this->screen->update_version();
			return;
		}
	}
}

void main_controller::get_all()
{
    this->get_n16_battery();
    this->get_un32_battery_percent();
    this->get_n16_pressure();
    this->get_un32_max_count();
    this->get_un32_pulses();
    this->is_b_motor_running();
    this->is_b_pressure_ok();
    this->is_b_am_valid();
    this->is_b_apt_valid();
    this->is_b_mcu_valid();
    this->get_current_state();
	
}

void main_controller::idle_state()
{
	static bool Flag_press = false;
	
       RESET_PRESSED = test_for_reset_press_aux();
	if (RESET_PRESSED && (this->need_to_stall_static()))
	{
		Flag_press = true;
	}
	
		if (RESET_PRESSED && !reset_button_instead_of_fire && (!this->need_to_stall_static())) // If we want reset to be the fire button we have to disable it
    {
        this->debug->fast_beeps();	
        if (this->is_b_is_counting_up())
        {
           this->set_b_is_counting_up(false);
            this->set_un32_pulses_on_screen(400);
            this->screen->clean_screen(this->get_un32_pulses_on_screen());
        }
       else
       {
           this->set_b_is_counting_up(true);
            this->set_un32_pulses_on_screen(0);
            this->screen->clean_screen(this->get_un32_pulses_on_screen());
        }
       RESET_PRESSED = false;
    }
    this->adc->read();
    this->device->read();
	
	
	
///activate the motor with fire pressed and reset button to release the valve
    if (this->fire_is_pressed() )
    {
	    //RESET_PRESSED = test_for_reset_press_aux();
	    
	    if(Flag_press)
	    {
		    this->device->start_motor_pwm();
		    this->set_b_motor_running(true);
		    delay_with_watchdog(1000);
		    this->device->stop_motor_after_run();
		    this->set_b_motor_running(false);
		    Flag_press = false;
		    
	    }
	    else if (!this->need_to_stall_static())
        {
            this->last_time_fire = HAL_GetTick();
            if (COUNTER_V == 1)
            {
                this->get_device()->clear_counter();       // We clear before each run to not skip counts
            }
            else if (COUNTER_V == 2)
            {
                // this->virtual_counter = 0;
                // probably not needed
            }
            this->added_counter();
            this->device->start_motor_pwm();
            this->set_b_motor_running(true);
            this->screen->update_piazo(this->is_b_motor_running());
            this->state->next = Active;
        }
    }
    this->get_all();
    this->adc->convert_to_volt();
    this->screen->update_battery(this->get_adc()->battery_volt, this->un32_battery_percent_);
    this->screen->update_pressure_static(this->n16_pressure_);
    this->screen->update_remaining_in_percent(this->un32_max_count_, this->un32_pulses_);
    this->screen->update_piazo(this->b_motor_running_);
    this->update_debug();
}

void main_controller::Write_Pulses_To_Am_Every_50()
{ 
	char writeout_str[] = "Write pulses on Am \r\n";
	uint32_t error_count = 0;   //no error count because this save is in the middle of work
	uint32_t read_back = 0;
	HAL_StatusTypeDef status;
	if (this->last_saved_50 < this->get_un32_pulses() && this->b_is_interlocked_) 
	{
		this->get_un32_cursor();
		read_back = this->get_mem()->read(3, this->get_un32_cursor());
		if ((read_back != 0xffffffff) || (read_back != 0))
		{
			this->set_un32_cursor(this->get_un32_cursor() + 4);
			//erase the first 4k after getting to the end of 4k block
			if((this->get_un32_cursor() % End_4k_BORDER) == 0)
			{   this->get_mem()->erase(3,START_BLOCK_ADD_AM);
				this->set_un32_cursor(START_BLOCK_ADD_AM) ;
			}
		}
		
		
		status = this->get_mem()->write(3, this->un32_cursor_, this->un32_pulses_);
		read_back = this->get_mem()->read(3, this->get_un32_cursor());
		if (read_back != this->un32_pulses_)
		{
			sprintf(writeout_str, "Saving Am Failed\r\n");	
		}
		//only saving once	
	 sprintf(writeout_str_data,
			"p: %lu c: %d \r\n",
			(unsigned long)(this->get_un32_pulses()),
			(int)(this->get_un32_cursor()));
		
		
		Serial_PutString(writeout_str);
		Serial_PutString(writeout_str_data);
		
		while ((status != HAL_OK) && (error_count-- != 0))
		{
			this->get_mem()->check_connections();
			if (this->get_am_valid())
			{
				delay(1);
				status = this->get_mem()->write(3, this->get_un32_cursor(), this->un32_pulses_);	
				sprintf(writeout_str_data,
					"p: %lu c: %lu \r\n",
					this->get_un32_pulses(),
					this->get_un32_cursor());
				Serial_PutString("Write Out on Am\r\n");
				Serial_PutString(writeout_str_data);
			}
			
			if (error_count == 0)
			{
				// After this error we will have to reseat the AM
				this->get_screen()->tx_send_general_error(502, this->generate_error_code());
			}
		}
	}
}

void main_controller::write_out_pulses_on_am()
{
	HAL_StatusTypeDef status;
	uint32_t error_count = 30;     //no error count because this save is in the middle of work
	uint32_t read_back = 0;
	// Added in case the interlock fails - we want to restore the data)
	   if(this->last_written_pulse_count_ < this->get_un32_pulses() && this->b_is_interlocked_)  
	{
		this->get_un32_cursor();
		read_back = this->get_mem()->read(3, this->get_un32_cursor());
		if ((read_back != 0xffffffff) || (read_back != 0))
		{
			//erase the first 4k after getting to the end of 4k block
			this->set_un32_cursor(this->get_un32_cursor() + 4);
			if ((this->get_un32_cursor() % End_4k_BORDER) == 0)
			{
				this->get_mem()->erase(3, START_BLOCK_ADD_AM);
				this->set_un32_cursor(START_BLOCK_ADD_AM);
			}
			
		}         
		status = this->get_mem()->write(3, this->un32_cursor_, this->un32_pulses_);
	        
		while ((status != HAL_OK) && (error_count-- != 0))
		{
			this->get_mem()->check_connections();
			if (this->get_am_valid())
			{
				delay(1);
				status = this->get_mem()->write(3, this->get_un32_cursor(), this->un32_pulses_);	
			}
		}
		if (error_count == 0)
		{
			// After this error we will have to reseat the AM
			this->get_screen()->tx_send_general_error(502, this->generate_error_code());
			Serial_PutString("NO Write Out on Stop \r\n");
			sprintf(writeout_str_data,
				"p: %lu c: %d \r\n",
				(unsigned long)(this->get_un32_pulses()),
				(int)(this->get_un32_cursor()));
			Serial_PutString(writeout_str_data);
		}
		else 
		{
			read_back = this->get_mem()->read(3, this->get_un32_cursor());
			if (read_back == this->un32_pulses_)
			{
				this->last_written_pulse_count_ = this->get_un32_pulses();
				pf_memory.pulses_last_time_written_v = this->last_written_pulse_count_;
				pf_memory.AM_address_to_write_on_PF_v = this->get_un32_cursor();
				Serial_PutString("Write Succeed\r\n");
				sprintf(writeout_str_data,
					"p: %lu c: %d \r\n",
					(unsigned long)(this->get_un32_pulses()),
					(int)(this->get_un32_cursor()));
				Serial_PutString(writeout_str_data);
			}
		}
	}
	else if(!this->b_is_interlocked_)
	{
		Serial_PutString("Interlock Not Letting write out on stall\r\n");
	}
	HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
}

bool main_controller::Read_no_pulses_remaining()
{  
	return this->get_mem()->Read_no_pulses_remaining();
}

void main_controller::write_no_pulses_remaining()
{  
	uint32_t No_More_Pulse_Adrress = 0xffff40;
	const uint32_t magic_num = 0x444f4e45;
	HAL_StatusTypeDef status;
	uint32_t error_count = 5; 

	status = this->get_mem()->write(3, No_More_Pulse_Adrress, magic_num);
	while ((status != HAL_OK) && (error_count-- != 0))
	{
		this->get_mem()->check_connections();
		if (this->get_am_valid())
		{
			delay(1);
			status = this->get_mem()->write(3, No_More_Pulse_Adrress, magic_num);	
		}
		if (error_count == 0)
		{
			// After this error we will have to reseat the AM
			//this->get_screen()->tx_send_general_error(502, this->generate_error_code());
			Serial_PutString("Problem Writing magic number \r\n");
		}
	}

}
void main_controller::active_state()
{
	this->device->read();
	//this->get_adc()->read(); // This can be passed and not done as often because it is very time consuming
	uint8_t stall_reason = this->need_to_stall_dynamic();

	if (this->fire_is_pressed() || stall_reason != 0 )
	{
		this->set_b_motor_running(false);
		this->device->stop_motor_after_run();
		this->write_out_pulses_on_am();
		
		  if(stall_reason == 3) // Pressure stall
		{
			for (int k = 0; k < 10; k++)
			{
				this->get_screen()->update_pressure(0);
				delay_with_watchdog(200);
			}
		}

		if (stall_reason == 1)
		{
			// If we stall on max count
			write_no_pulses_remaining();
			for(int k = 0 ; k < 10 ; k++)
			{
				this->screen->update_error_remaining(0);
				delay_with_watchdog(200);
			}
		}

		this->state->next = UpdateData;

		if (stall_reason == 5 && this->is_b_is_interlocked())
		{
			this->device->start_motor_pwm();
			this->set_b_motor_running(true);
			this->screen->update_piazo(this->is_b_motor_running());
			this->state->next = Active;   // On interlock, if we manage to reread and write out the interlock. just keep running
		}
		if (stall_reason == 6)
		{
			Serial_PutString("no connection to am\r\n");
			screen->tx_send_general_error(600);
		}
	}
	else
	{
		this->state->next = Active;
	}

	// UPDATE THE RUNNING DATA if we don't stall
	uint8_t added_pulses = this->added_counter();
	if (added_pulses > 0)
	{
		// UPDATE AM PULSES
		this->set_un32_pulses(this->un32_pulses_ + added_pulses);
		// UPDATE APT PULSES
		this->un32_apt_pulses_ = this->un32_apt_pulses_ + added_pulses;
		pf_memory.pulses_need_to_write_on_PF_v = this->un32_pulses_;
		// BEEP EVERY 200th time;
		static uint32_t beep_once = 0;
		if (beep_once != this->un32_pulses_on_screen_  && this->un32_pulses_on_screen_  % 200 == 0)
		{
			beep_once = this->un32_pulses_on_screen_;
			if (this->is_b_is_counting_up() && this->un32_pulses_on_screen_ != 0)
			{
				this->debug->long_beep_2();
			}
			else if (!this->is_b_is_counting_up() && this->un32_pulses_on_screen_ != 400)
			{
				this->debug->long_beep_2();
			}
		}

		// Show count up or down
		if(this->b_is_counting_up_)
		{
			this->un32_pulses_on_screen_ += added_pulses;
		}
		else
		{
			if (this->un32_pulses_on_screen_  >= added_pulses)
			{
				this->un32_pulses_on_screen_ = this->un32_pulses_on_screen_ - added_pulses;
			}
			else
			{
				this->un32_pulses_on_screen_ = 0;
				this->set_b_motor_running(false);
				this->set_b_is_counting_up(true);
				this->device->stop_motor_after_run();
				///    this->write_out_pulses_on_stop_motor();
				    this->state->next = UpdateData;
			}
		}
	}
	this->screen->update_pulse_counter(this->un32_pulses_on_screen_);
	/* writing to AM memory every 50 pulses*/
	if ((this->un32_pulses_ % 50 == 0) && (this->last_saved_50 != this->un32_pulses_))
	{
		Write_Pulses_To_Am_Every_50();
		this->last_saved_50 = this->un32_pulses_;
	}
	if (
		(this->un32_max_count_  - this->un32_pulses_ <= 5000) && 
		((this->un32_max_count_  - this->un32_pulses_) % 200 == 0))
	{
		// When we have less than 5000 counts, we will show on start the attention of number of remaining counts
		 this->screen->update_error_remaining(this->un32_max_count_  - this->un32_pulses_);
	}
	
	// Update APT memory with pulses so far:
	uint32_t temp_pulses = this->un32_apt_pulses_;
	if (temp_pulses % 100 == 0)
	{
		bool has_written_out_console = true;
		uint32_t address = temp_pulses / 25;  // Every 100-th pulses while we need to step 4 bytes so we div 100 and mul by 4
		uint32_t read_back = this->get_mem()->read(2, address);
		if (read_back != temp_pulses)
		{
			this->get_mem()->write(2, address, temp_pulses);
			has_written_out_console = false;
		}
		else
		{
			if (!has_written_out_console)
			{
				Serial_PutString("APT write pulses succesfully\r\n");
				has_written_out_console = true;
			}
		}
	}
	if (this->un32_apt_pulses_ >= MAX_APT_PULSE)
	{  
		
		this->set_b_motor_running(false);
		this->device->stop_motor_after_run();
		Serial_PutString("cannot run APT above 204k\r\n");
	    screen->tx_send_general_error(503);	
		
	}
	else if ((this->un32_apt_pulses_>= WARN_APT_PULSES)&& (this->un32_apt_pulses_% 1200==0))
	{
		Serial_PutString("pulses above 180000\r\n");
		screen->tx_send_general_error(504);
	}
}

uint32_t main_controller::update_opto_counter(void)
{
    static uint8_t button_history = 0;
    button_history =  button_history << 1;
    button_history |= (uint8_t)this->get_device()->StatusReg->current.bits.OPTO_STATUS ;
    if ((button_history & 0xcf) == 0x0f) // We can make the mask bigger / or look for shorter pattern if we find problems
    { 
        this->virtual_counter++;
        if (this->virtual_counter > 15)
        {
            this->virtual_counter = 0;
        }
        button_history = 0xff;
    }
    return (this->virtual_counter);
}
uint8_t main_controller::added_counter()
{
    this->get_device()->read();
    static volatile uint8_t new_opto_amount = this->last_opto_ammount_;  // INITIALIZE VALUE;
    static volatile uint8_t once_before_last_amount = this->last_opto_ammount_;  // INITIALIZE VALUE;
    static volatile uint8_t additional_counts = 0;
    static bool wait_for_the_flip = false;

    if (COUNTER_V == 1)
    {
        
        // V1
        new_opto_amount = this->get_device()->StatusReg->current.nibbles.nibble_OPTO & 0xf;
    }
    else if (COUNTER_V == 2)
    {
        // V2 
        new_opto_amount = this->update_opto_counter();
    }
    else
    {
        // DEFAULT
        new_opto_amount = this->get_device()->StatusReg->current.nibbles.nibble_OPTO & 0xf;
    }
    
    if(new_opto_amount != this->last_opto_ammount_)
    {
        if (new_opto_amount > this->last_opto_ammount_)
        {
            additional_counts = new_opto_amount - this->last_opto_ammount_;
        }
        else
        {
            additional_counts = (16 - this->last_opto_ammount_) + new_opto_amount;
        }
        if (additional_counts > 1)
        {
            if (new_opto_amount == once_before_last_amount)
            {
                additional_counts = 0;
            }
            else
            {
                wait_for_the_flip = true;
                additional_counts = 0;
            }
        }
        if ((new_opto_amount == 1) && wait_for_the_flip)
        {
            wait_for_the_flip = false;
            additional_counts = 2;
        }
        once_before_last_amount = this->last_opto_ammount_;
        this->last_opto_ammount_ = new_opto_amount;

        return additional_counts;
    }
    else
    {
        return 0;
    }
}

void main_controller::qa_state()
{
    this->adc->read();
    this->adc->convert_to_volt();
    this->device->read();
    if (this->status->bits.FIRE_STATUS)
    {
        this->device->toggle_motor();
    }
    
    volatile uint32_t added_pulses = this->device->read_counter();
    
    added_counter();
    
    if (added_pulses > 0)
    {
        this->mem->AM_MEM->pulses += added_pulses;      // add a function to spi write and increment
        this->mem->AM_MEM->cursor += added_pulses * 8;
        if (this->b_is_counting_up_)
        {
            this->un32_pulses_on_screen_ += added_pulses;
        }
        else
        {
            if (this->un32_pulses_on_screen_ > added_pulses)
            {
                this->un32_pulses_on_screen_ = this->un32_pulses_on_screen_ - added_pulses;
            }
            else
            {
                this->un32_pulses_on_screen_ = 0;
                this->device->stop_motor_after_run();
            }
        }
    }
    this->screen->update_pulse_counter(this->get_un32_pulses());
    this->mem->check_connections();
    this->get_screen()->update_battery(this->get_adc()->battery_volt, this->get_un32_battery_percent());
    this->screen->update_pressure(*this->pressure);
    this->screen->update_remaining_in_percent(10000, *this->pulses); //default is 10000 for initial run
    this->screen->update_piazo(this->is_b_motor_running());
    // special to QA:
    this->screen->qa_mon(this->is_b_mcu_valid(), this->is_b_apt_valid(), this->is_b_am_valid());
    this->update_debug();
}

void main_controller::Send_Uart_Test_Screen()
{
	//this->screen->test_screen_update("Uart Recieve");
}
void main_controller::Test_UART()
{ 
	if (HAL_UART_Receive_DMA(& huart1, (uint8_t *)aRxBuffer, 5) != HAL_OK)
	{
		Error_Handler();
	}
	
	if (HAL_UART_Transmit_DMA(&huart1, (uint8_t*)aTxBuffer, 5) != HAL_OK)
	{
		Error_Handler();
	}		
}


	


void main_controller::Test_Short_Cut()

{  
	char buffer[30];
	delay_with_watchdog(3000);
	static BRING_BOARD BRING_STATUS = NO_ENABLE_PRESS;
	switch (BRING_STATUS)
	{
	case VOLT_2:
		if (HAL_GPIO_ReadPin(DEBUG_STATE_2_GPIO_Port, DEBUG_STATE_2_Pin) == 1)
		{
			HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_1_Pin, PIN_LOW);       //A
			HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_HIGH);         //B	
			HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_LOW);  //C
			sprintf(buffer, "$w%s#", "Testing");
			this->screen->test_screen_update(buffer);
			delay_with_watchdog(5000);
			if ((HAL_GPIO_ReadPin(DEBUG_STATE_3_GPIO_Port, DEBUG_STATE_3_Pin) == 1))
			{
				
				//pass
				BRING_STATUS = VOLT_4;
				
			}
			else
			{
				BRING_STATUS = TEST_FAIL;    //fail
				sprintf(buffer, "$z%s#", "Fail-1");
				this->screen->test_screen_update(buffer);
				
			}
			HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_1_Pin, PIN_LOW);        //A
			HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_LOW);          //B	
			HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_LOW);       //C
			
		}
		
		else
			BRING_STATUS = NO_ENABLE_PRESS; //not activate
			
		break;
		
	case VOLT_4:
		
		if (HAL_GPIO_ReadPin(DEBUG_STATE_2_GPIO_Port, DEBUG_STATE_2_Pin) == 1)
		{
			HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_1_Pin, PIN_HIGH);     //A
			HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_HIGH);        //B	
			HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_LOW); //C
			sprintf(buffer, "$w%s#", "Testing");
			this->screen->test_screen_update(buffer);
			delay_with_watchdog(5000);
			if ((HAL_GPIO_ReadPin(DEBUG_STATE_3_GPIO_Port, DEBUG_STATE_3_Pin) == 1))
			{
				//succeed
			 //this->screen->test_screen_update("Motor Test");
			 //Serial_PutString("motor test\r\n");
			 BRING_STATUS = VOLT_6;
			}
			else
			{
				BRING_STATUS = TEST_FAIL;     //fail
				sprintf(buffer, "$z%s#", "Fail-2");
				this->screen->test_screen_update(buffer);
			}
			HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_1_Pin, PIN_LOW);         //A
			HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_LOW);           //B	
			HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_LOW);        //C
			
		}
		else
		{
			BRING_STATUS = NO_ENABLE_PRESS;    //No press
		}
		break;
	case VOLT_6:
		
		if (HAL_GPIO_ReadPin(DEBUG_STATE_2_GPIO_Port, DEBUG_STATE_2_Pin) == 1)
		{
			HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_1_Pin, PIN_LOW);      //A
			HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_LOW);         //B	
			HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_HIGH);      //C
			sprintf(buffer, "$w%s#", "Testing");
			this->screen->test_screen_update(buffer);
			delay_with_watchdog(5000);
			sprintf(buffer, "$w%s#", "Testing");
			this->screen->test_screen_update(buffer);
			delay_with_watchdog(5000);
			if ((HAL_GPIO_ReadPin(DEBUG_STATE_3_GPIO_Port, DEBUG_STATE_3_Pin) == 1))
			{
				//failure
			 //this->screen->test_screen_update("Motor Test");
				//Serial_PutString("motor test\r\n");
				BRING_STATUS = TEST_PASS;
			}
			else
			{
				BRING_STATUS = TEST_FAIL;      //fail
				sprintf(buffer, "$z%s#", "Fail-3");
				this->screen->test_screen_update(buffer);
			}
			HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_1_Pin, PIN_LOW);         //A
			HAL_GPIO_WritePin(DEBUG_BRK1_GPIO_Port, DEBUG_BRK1_Pin, PIN_LOW);           //B	
			HAL_GPIO_WritePin(DEBUG_BRK2_GPIO_Port, DEBUG_BRK2_Pin, PIN_LOW);        //C
			
		}
		else
		{
			BRING_STATUS = NO_ENABLE_PRESS;      //fail	
		}
		break;
	case NO_ENABLE_PRESS:
		if (HAL_GPIO_ReadPin(DEBUG_STATE_2_GPIO_Port, DEBUG_STATE_2_Pin) == 0)
		{
			sprintf(buffer, "$w%s#", "Push Enable");
			this->screen->test_screen_update(buffer);
		}
		
		Serial_PutString("start test\r\n");
		delay_with_watchdog(1000);
		BRING_STATUS = VOLT_2;
		break;
	case TEST_PASS:
		sprintf(buffer, "$j%s#", "Pass");
		this->screen->test_screen_update(buffer);
		delay_with_watchdog(1000);
		//BRING_STATUS = VOLT_2;
		break;
	case TEST_FAIL:
		sprintf(buffer, "$z%s#", "FAIL");
		this->screen->test_screen_update(buffer);
		delay_with_watchdog(1000);
		//BRING_STATUS = VOLT_2;
		break;
		
	}
	
}

void main_controller::Test_DC_MACHINE()
{   delay_with_watchdog(3000);
	char  fire_test = 0;
	char inter_test = 0;
	char string[30];
	
	uint16_t i=0;
	uint8_t read_value = 0;
	uint8_t ADD_PULSE = 0;
	static DC_TEST DC_STATUS = TEST_INTELOCK;
	switch (DC_STATUS)
	{
	case TEST_INTELOCK:
		inter_test = false;
		Serial_PutString("Press Interlock button");
		sprintf(string, "$w%s#", "Interlock Test");
		this->screen->test_screen_update(string);
		delay_with_watchdog(1000);
		for (i = 0; ((i < 500) && (inter_test == false)); i++)
		{
			delay_with_watchdog(10);
			if (!(HAL_GPIO_ReadPin(SIMPLE_INTERLOCK_GPIO_Port, SIMPLE_INTERLOCK_Pin)))
			{
				inter_test = true;
				HAL_GPIO_WritePin(ENABLE_POWER_12V_GPIO_Port, ENABLE_POWER_12V_Pin, GPIO_PIN_SET);
			}
		}
		if (inter_test == true)
		{
			sprintf(string, "$j%s#", "Pass");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
			Serial_PutString("Interlock was pressed");
			DC_STATUS = TEST_WRITE_AM_MEMORY;
		}
		else
		{
			flag_test = 1;
			sprintf(string, "$z%s#", "Fail");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
			DC_STATUS = IDLE;
		}
		
		
			//DC_STATUS = IDLE;
		
		//if yes continu ,no stop
		break;
	case START_TEST_MOTOR :
		
		sprintf(string, "$w%s#", "Motor Test");
		this->screen->test_screen_update(string);
		Serial_PutString("motor test\r\n");
		this->device->start_motor_pwm();
		this->set_b_motor_running(true);
		for (i = 0; i < 5; i++)
		{
			delay_with_watchdog(100);
			this->adc->read();
		}
		this->device->stop_motor_after_run();
		this->set_b_motor_running(false);
		if (this->get_adc()->channels_buffer[Motor_current_sense].value > ADC_TEST_CURRENT)
		{
			Serial_PutString("Pass\r\n");
			sprintf(string, "$j%s#", "Pass");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
		}
		else
		{
			flag_test = 1;
			sprintf(string, "$z%s#", "FAIL");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);	
			
		}
	   ///start motor 
		///DELAY FOR
		//check current
		//signal sucess or failure
		//stop motor 
		DC_STATUS =IDLE;
		break;
	case TEST_PRESSURE:	
		sprintf(string, "$w%s#", "Push Pressure");
		this->screen->test_screen_update(string);
		Serial_PutString("pressure test\r\n");
		delay_with_watchdog(5000);
		this->adc->read();
		if (this->get_adc()->channels_buffer[Pressure].value > ADC_TEST_DC_PRESSURE_TH)///12 or 79
		{
			Serial_PutString("Pass\r\n");
			sprintf(string, "$j%s#", "Pass");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
		}
		else
		{
			flag_test = 1;
			sprintf(string, "$z%s#", "FAIL");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);	
			
		}
		//signal pressure test
		///DELAY FOR RESULT
		//CHECK RESULT
		DC_STATUS = START_TEST_MOTOR;
		break;
	case TEST_COUNTER:
		//signal COUNTER test
		///DELAY FOR RESULT
		sprintf(string, "$w%s#", "Counter Test");
		this->screen->test_screen_update(string);
		Serial_PutString("Test Counter\r\n");
		//delay_with_watchdog(5000);
		for(i = 0 ; i < 500 ; i++)
		{
			//ADD_PULSE = this->added_counter();
			delay_with_watchdog(10);
			ADD_PULSE = this->added_counter();
			
			if (ADD_PULSE > 0)
				//CHECK RESULT
				{
				this->SUM_ADDED_FROM_TEST = this->SUM_ADDED_FROM_TEST + ADD_PULSE;	//DC_STATUS = TEST_WRITE_AM_MEMORY;
				}
		}
		if (this->SUM_ADDED_FROM_TEST > ADC_TEST_COUNTER_TH)
			{
				Serial_PutString("Pass\r\n");
				sprintf(string, "$j%s#", "Pass");
				this->screen->test_screen_update(string);
				delay_with_watchdog(1000);
			}
			else
			{
				flag_test = 1;
				sprintf(string, "$z%s#", "Fail");
				this->screen->test_screen_update(string);
				delay_with_watchdog(1000);
			}
		
		sprintf(string, "NUM_OF PULSE:%d\r\n", this->SUM_ADDED_FROM_TEST);
		Serial_PutString(string);
		DC_STATUS = TEST_PRESSURE;
		break;
	case TEST_WRITE_AM_MEMORY:
		sprintf(string, "$w%s#", "Memory Test");
		this->screen->test_screen_update(string);
		this->mem->check_connections();
		delay_with_watchdog(2000);
		this->get_mem()->erase(3);
		delay_with_watchdog(8000);
		this->get_mem()->read256(3, 0);
		//this->screen->test_screen_update("MEMORY TEST");
		sprintf(string, "$w%s#", "Memory Test");
		this->screen->test_screen_update(string);
		delay_with_watchdog(1000);
		this->get_mem()->read256(3, 256);
		delay_with_watchdog(7000);
		sprintf(string, "$w%s#", "Memory Test");
		this->screen->test_screen_update(string);
		this->get_mem()->write(3, 0, 31);
		delay_with_watchdog(1000);
		read_value = this->get_mem()->read(3, 0);
		if (read_value == 31)
		{
			Serial_PutString("Write success\r\n");
			sprintf(string, "$j%s#", "Pass");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
		}
		else
		{
			flag_test = 1;
			Serial_PutString("Write fail");
			sprintf(string, "$z%s#", "Fail");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
		}
		DC_STATUS =  TEST_FIRE_BUTTON;
			break;
	case TEST_FIRE_BUTTON:
		fire_test = false;
		Serial_PutString("Press fire button");
		sprintf(string, "$w%s#", "Button Test");
		this->screen->test_screen_update(string);
		delay_with_watchdog(1000);
		for (i = 0; ((i < 500) && (fire_test==false)); i++)
		{
			delay_with_watchdog(10);
			if (this->fire_is_pressed())
				fire_test = true;
		}
		if (fire_test == true)
		{
			sprintf(string, "$j%s#", "Pass");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
			Serial_PutString("Fire was pressed");	
		}
		else
		{
			flag_test = 1;
			sprintf(string, "$z%s#", "Fail");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
		}
		DC_STATUS =   TEST_COUNTER;
		break;
		
	case IDLE:
		//Serial_PutString("IDLE\r\n");
	  if(flag_test != 1)
		{
			sprintf(string, "$j%s#", "Success Test");
			this->screen->test_screen_update(string);
		}
		else
		{
			sprintf(string, "$z%s#", "Fail Test");
			this->screen->test_screen_update(string);	
		}
			break;
		default:
			break;
		}	
	}
void main_controller::Test_Mcu_Machine()
{ 
	delay_with_watchdog(3000);
	uint8_t Button_Reset = 0;
	uint8_t read_value = 0;
	char inter_test = 0;
	char  button_test = 0;
	//static bool flag_test = 0;
	uint16_t i = 0;
	char string[30];
	static MCU_TEST  TMcuMachine = TEST_INTELOCK_MCU;

	switch (TMcuMachine)
	{
	case TEST_INTELOCK_MCU :
		inter_test = false;
		delay_with_watchdog(4000);
		Serial_PutString("Press Interlock button");
		sprintf(string, "$w%s#", "Interlock Test");
		this->screen->test_screen_update(string);
		delay_with_watchdog(1000);
		for (i = 0; ((i < 500) && (inter_test == false)); i++)
		{
			delay_with_watchdog(10);
			if (!(HAL_GPIO_ReadPin(SIMPLE_INTERLOCK_GPIO_Port, SIMPLE_INTERLOCK_Pin)))
			{
				inter_test = true;
				HAL_GPIO_WritePin(ENABLE_POWER_12V_GPIO_Port, ENABLE_POWER_12V_Pin, GPIO_PIN_SET);
			}
		}
		if (inter_test == true)
		{
			sprintf(string, "$j%s#", "Pass");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
			Serial_PutString("Interlock was pressed");
			TMcuMachine = AM_TEST_MCU;
		}
		else
		{
			flag_test = 1;
			sprintf(string, "$z%s#", "Fail");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
			TMcuMachine = NO_ACTIVITY;
		}
		break;	
	case AM_TEST_MCU:
		sprintf(string, "$w%s#", "AM Test");
		this->screen->test_screen_update(string);
		this->mem->check_connections();
		delay_with_watchdog(2000);
		sprintf(string, "$w%s#", "AM Test");
		this->screen->test_screen_update(string);
		this->get_mem()->erase(3);
		delay_with_watchdog(8000);
		sprintf(string, "$w%s#", "AM Test");
		this->screen->test_screen_update(string);
		this->get_mem()->read256(3, 0);
		delay_with_watchdog(3000);
		this->get_mem()->read256(3, 0);
		sprintf(string, "$w%s#", "AM Test");
		this->screen->test_screen_update(string);
		delay_with_watchdog(7000);
		sprintf(string, "$w%s#", "AM Test");
		this->screen->test_screen_update(string);
		this->get_mem()->write(3, 0, 31);
		delay_with_watchdog(1000);
		read_value = this->get_mem()->read(3, 0);
		if (read_value == 31)
		{
			sprintf(string, "$j%s#", "Pass");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
			
			Serial_PutString("Write success");
		}
		else
		{
			//Serial_PutString("Write fail");
			flag_test = 1;
			sprintf(string, "$z%s#", "Fail");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
		}
		TMcuMachine = APT_MEM;
		break;
	case APT_MEM:
		sprintf(string, "$w%s#", "APT Test");
		this->screen->test_screen_update(string);
		this->mem->check_connections();
		delay_with_watchdog(2000);
		sprintf(string, "$w%s#", "APT Test");
		this->screen->test_screen_update(string);
		this->get_mem()->erase(2);
		delay_with_watchdog(8000);
		sprintf(string, "$w%s#", "APT Test");
		this->screen->test_screen_update(string);
		this->get_mem()->read256(2, 0);
		delay_with_watchdog(1000);
		this->get_mem()->read256(2, 0);
		sprintf(string, "$w%s#", "APT Test");
		this->screen->test_screen_update(string);
		delay_with_watchdog(7000);
		sprintf(string, "$w%s#", "APT Test");
		this->screen->test_screen_update(string);
		this->get_mem()->write(2, 0, 34);
		delay_with_watchdog(1000);
		read_value = this->get_mem()->read(2, 0);
		if (read_value == 34)
		{
			Serial_PutString("Write success apt");
			sprintf(string, "$j%s#", "Pass");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
			
		}
		else
		{
			flag_test = 1;
			sprintf(string, "$z%s#", "Fail");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
			this->get_mem()->read256(3, 0);
			delay_with_watchdog(1000);
			Serial_PutString("Write fail apt");
		}
		TMcuMachine = TEST_RESET_BUTTON;
		break;
	case TEST_RESET_BUTTON:
		button_test = false;
		Serial_PutString("Press fire button");
		sprintf(string, "$w%s#", "Button Test");
		this->screen->test_screen_update(string);
		delay_with_watchdog(1000);
		for (i = 0; ((i < 500) && (button_test == false)); i++)
		{
			delay_with_watchdog(10);
			Button_Reset = test_for_reset_press_aux();
			if (Button_Reset==1)
				button_test = true;
		}
		if (button_test == true)
		{
			sprintf(string, "$j%s#", "Pass");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
			Serial_PutString("Fire was pressed");	
		}
		else
		{
			flag_test = 1;
			sprintf(string, "$z%s#", "Fail");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
		}
		TMcuMachine =   TEST_SELF_UART;
		break;
	case TEST_SELF_UART:
		sprintf(string, "$w%s#", "Uart Test");
		this->screen->test_screen_update(string);
		delay_with_watchdog(4000);
		Test_UART();
		delay_with_watchdog(2000);
		if (aRxBuffer[1] == 't' || aRxBuffer[2] == 'e' || aRxBuffer[3] == 's' || aRxBuffer[4] == 't')
		{
			flag_uart_test = 1;
		}
	
		if (flag_uart_test ==1)
		{
			sprintf(string, "$j%s#", "Pass");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
				
		}
		else
		{
			flag_test = 1;
			sprintf(string, "$z%s#", "Fail");
			this->screen->test_screen_update(string);
			delay_with_watchdog(1000);
		}
		
		TMcuMachine = NO_ACTIVITY;
		break;
	case NO_ACTIVITY:
		if (flag_test != 1)
		{
			sprintf(string, "$j%s#", "Success Test");
			this->screen->test_screen_update(string);
		}
		else
		{
			sprintf(string, "$z%s#", "Fail Test");
			this->screen->test_screen_update(string);	
		}
		break;
	}
}
	void main_controller::Test_Status_Machine()
{ 
	bool state1 = 0;
	bool state2 = 0;
	
	state1 = ((bool)HAL_GPIO_ReadPin(LOGIC_BYPASS_3_GPIO_Port, LOGIC_BYPASS_3_Pin));
	state2 = ((bool)HAL_GPIO_ReadPin(LOGIC_BYPASS_4_GPIO_Port, LOGIC_BYPASS_4_Pin));
	if ((state1 == 0)&&(state2 == 0))
	{
		STMachine = TEST_DC;
	}
	else if ((state1 == 1)&&(state2 == 0))
	{
		STMachine = TEST_MCU;
	}
	else if ((state1 == 0)&&(state2 == 1))
	{
		STMachine = Bring_Up; 
		
	}
	switch (STMachine) 
	{
	case Bring_Up:
	{
		Test_Short_Cut();	
		break;
	}
	case TEST_DC:
		Test_DC_MACHINE();
		break;
	case TEST_MCU:
		Test_Mcu_Machine();
		break;
		
	}
	
}

void main_controller::update_state()

{   
    this->mem->check_connections();

    this->get_all();
    
    this->adc->read();
    this->adc->convert_to_volt();
    this->adc->convert_to_celsius();
    if (this->get_adc()->temperature_celsius > ADC_FIRE_TEMP)
    {
        Serial_PutString("DANGEROUSLY HIGH TEMP \r\n");
        HAL_GPIO_WritePin(ENABLE_POWER_12V_GPIO_Port, ENABLE_POWER_12V_Pin,GPIO_PIN_RESET);
        HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);  // This will
        while (1)
        {
            HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
            HAL_Delay(10);
        }
    }
    this->get_screen()->update_battery(this->get_adc()->battery_volt, this->get_un32_battery_percent());
    this->get_screen()->update_pressure(this->get_n16_pressure());
    this->get_screen()->update_piazo(this->is_b_motor_running());
    this->screen->update_remaining_in_percent(this->un32_max_count_, this->un32_pulses_);
	
	this->screen->update_pulse_counter(this->get_un32_pulses_on_screen());
    this->screen->update_piazo(this->is_b_motor_running());
    this->screen->update_version();
    this->update_debug();
    // After Update - go to the needed state
    if (this->is_b_motor_running())
    {
        if (this->get_state()->current == UartDebug)
        {
            this->get_state()->next = UartDebug;
        }
        else
        {
            this->get_state()->next = Active;
        }
    }
    else
    {
        this->get_state()->next = Idle;
    }
}

void main_controller::update_debug()
{
    this->get_all();
    this->debug->update(this->is_b_motor_running(),
        this->is_b_pressure_ok(),
        this->is_b_mcu_valid(),
        this->is_b_apt_valid(),
        this->is_b_am_valid());
    this->debug->update_state((uint8_t)this->get_current_state());
}

bool main_controller::need_to_stall_static()
{
    static const char stall_run_str[] = "Couldn't start run - stall\r\n";
	static const char stall_reason_p[] = "Pressure - static stall\r\n";
	static const char stall_reason_i[] = "Interlock - static stall\r\n";
    static const char stall_reason_am[] = "AM disconnected - static stall\r\n";

    this->adc->read(Pressure);
    this->get_adc()->pressure_check();

    // Motor not running - check if we should stall on start
    if(!this->get_adc()->pressure_ok)
    {
        if (this->get_state()->bypass.value != 3)
        {

            Serial_PutString(stall_run_str);
            Serial_PutString(stall_reason_p);
            return true; // after 2 reads (averaged) we just stall
        }
    }

	
	this->mem->get_max();
	this->un32_max_count_ = this->get_un32_max_count();
	this->mem->Max_search_Am(End_4k_BORDER, START_BLOCK_ADD_AM);
	this->get_un32_pulses();
    if (this->un32_pulses_ >= this->un32_max_count_)
    {
        // stall because too much pulses
        uint32_t time_delay = HAL_GetTick();
        do
        {
            this->screen->update_error_remaining(0);
            HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
            HAL_Delay(10);
        } while (HAL_GetTick() - time_delay < 2000) ;
        return true;
    }
    if (!this->is_b_am_valid())
    {
	    uint32_t start_time = HAL_GetTick();
	    bool is_disconnected = true;
	    while ((HAL_GetTick() - start_time < 250) && is_disconnected) 
	    {
		    this->get_mem()->check_connections(3);
		    HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
		    if (this->is_b_am_valid()) 
		    {
			    is_disconnected = false;
		    }
	    }
	    if (is_disconnected) 
	    {
		    Serial_PutString(stall_run_str);
		    Serial_PutString(stall_reason_am);
		    return true;
	    }
    }
        
    if(!this->is_b_is_interlocked())
    {
        Serial_PutString(stall_run_str);
        Serial_PutString(stall_reason_i);
        return true;
    }
    return false;
}

uint8_t main_controller::need_to_stall_dynamic()
{
	bool pressure_is_not_restored;
	uint32_t timer_adc;
	uint8_t reason = 0;
	char current_print_back[15];
	static uint32_t Start_Am_disconnect_Time = 0;
	static uint32_t  Time_Of_Disconnect = 0;
	static const char stall_dynamic_str[] = "Stop while run - stall\r\n";
	static const char stall_dynamic_reason_p[] = "Pressure - stall\r\n";
	static const char stall_dynamic_reason_c[] = "Current - stall\r\n";
	static const char stall_dynamic_reason_T[] = "Temperature - stall\r\n";
	static const char stall_dynamic_reason_m[] = "Max Count - stall\r\n";
	static const char stall_dynamic_reason_i[] = "Interlock - stall\r\n";
	static const char stall_dynamic_reason_mem[] = "memory check - stall\r\n";
	static const char stall_dynamic_reason_a[] = "ADC is bad - stall\r\n";
	static uint32_t pressure_stall_lingering_timer = HAL_GetTick();
	static uint8_t pressure_drops = 0;
	static uint8_t AM_Fail_Status = 0;
	static uint32_t Start_PulseAmFail = 0;
	static uint32_t Diff_PulseAmFail = 0;
	static volatile uint32_t last_diff_sent = 0;
	static char buff_temp[40];
	STALL_TEST test_status = MAX_PULSE_TEST;
	// UPDATE DATA ON ENTTRY::
	this->get_un32_pulses();
	this->get_un32_max_count();
	this->adc->read(Pressure);
	this->get_adc()->pressure_check();
	// END UPDATE

	// Check if there is remaining pulses left
	switch(test_status)
	{
	case MAX_PULSE_TEST:
		{
			//checks absolute pulse count
			if(this->un32_pulses_ > this->un32_max_count_)
			{
				Serial_PutString(stall_dynamic_str);
				Serial_PutString(stall_dynamic_reason_m);
				reason = 1;
				this->get_mem()->LOG_DEBUG(0x100 + reason, this->un32_am_sn_, this->un32_apt_sn_, this->un32_pulses_);
				return reason;
			}
			/* Check if the pressure is still above a certain threshold
			 * The pressure function is implemented heuristically based on experiments.
			 * We have a behaviour in which the pulses are releasing pressure in the chamber.
			 * So we need to filter out temporary drops in pressure
			 * Also we have a steady decline in the pressure due to usage of gas
			 * So what we try to do, is to stop a slow decline below a certain level
			 * But filter out fast changes
			 * And still be able to stall if the pressure drops because the valve is closed off
			 *
			 * NOTE: the function was written to accomodate the old regulators which had significant drop in pressure while working
			 */
		case PRESSURE_TEST:
			{
				if (this->get_adc()->stall_pressure)
				{
					if (this->get_state()->bypass.value != 3)
					{
						pressure_is_not_restored = true;
						timer_adc = HAL_GetTick();
						// Stall on pressure disabled if value of bypass is == 3
						//checking for pressure drop for 3 times of 100ms
						while(pressure_is_not_restored)
						{
							HAL_GPIO_TogglePin(WATCHDOG_OUT_GPIO_Port, WATCHDOG_OUT_Pin);
							this->adc->read(Pressure);
							this->adc->read(V12);
							this->get_adc()->pressure_check();
							pressure_is_not_restored = this->get_adc()->stall_pressure;
							//checking for normal pressure
							if(COUNTER_V == 2)
							{
								// In case counter is in debounced mode - running, we cant wait 300 ms between the restoration
								this->update_opto_counter();
							}

							HAL_Delay(3);

							if ((HAL_GetTick() - timer_adc > 100))
							{
								if (pressure_drops == 0)
								{
									pressure_stall_lingering_timer = HAL_GetTick();
								}
								Serial_PutString("Stall Pressure - Timer Expired\r\n");
								pressure_drops++;
								break;
							}
						}

						if (pressure_drops > 3)
						{
							Serial_PutString(stall_dynamic_str);
							Serial_PutString(stall_dynamic_reason_p);
							reason = 3;
							this->get_mem()->LOG_DEBUG(0x100 + reason, this->un32_am_sn_, this->un32_apt_sn_, this->un32_pulses_);
							return reason; // after 2 reads (averaged) we just stall
						}
					}
				}
				//zeroing pressure_drops after starting waiting for 1500 from first pressure drop
				if(pressure_drops > 0)
				{
					if (HAL_GetTick() - pressure_stall_lingering_timer > 1500)
					{
						pressure_drops = 0;
					}
					// the timer is reloaded only when we get the first pressure drop
				}

			}
		}
	case TEMPERATURE_TEST:
		{
			// else pressure_drops is 0
			if(this->get_adc()->temperature_celsius > ADC_STALL_TEMP)
			{
				Serial_PutString(stall_dynamic_str);
				Serial_PutString(stall_dynamic_reason_T);
				//this->get_screen()->tx_send_general_error(669);     // the adc has reached high current sense
				reason = 7;
				this->get_mem()->LOG_DEBUG(0x100 + reason, this->un32_am_sn_, this->un32_apt_sn_, this->un32_pulses_);
				return reason;
			}
			// Stall if the current on motor is too high
			if((HAL_GetTick() - this->last_time_fire) > 2000)
			{
				if(this->get_adc()->Motor_current_sense_ > ADC_STALL_CURRENT)
					{
						Serial_PutString(stall_dynamic_str);
						Serial_PutString(stall_dynamic_reason_c);
						sprintf(current_print_back, "Current %d\r\n", (int)this->get_adc()->Motor_current_sense_);
						Serial_PutString(current_print_back);
						//this->get_screen()->tx_send_general_error(669);     // the adc has reached high current sense
						reason = 4;
						this->get_mem()->LOG_DEBUG(0x100 + reason, this->un32_am_sn_, this->un32_apt_sn_, this->un32_pulses_);
						return reason;	
					}
//				
			}
		case INTERLOCK_TEST:
			{
				if (!this->is_b_is_interlocked())
				{
					Serial_PutString(stall_dynamic_str);
					Serial_PutString(stall_dynamic_reason_i);
					reason = 5;
					this->get_mem()->LOG_DEBUG(0x100 + reason, this->un32_am_sn_, this->un32_apt_sn_, this->un32_pulses_);
					return reason;
				}
			}

		case AM_FAIL_TEST:
			{
				if (!this->is_b_am_valid())
				{
					/*checking the number of pulses that it had disconnection to the AM memory*/
					if (AM_Fail_Status == 0)
					{
						Start_Am_disconnect_Time = HAL_GetTick();
						Start_PulseAmFail = this->un32_pulses_;
						AM_Fail_Status = 1;
						last_diff_sent = 0;
						Diff_PulseAmFail = 0;
						Serial_PutString("AM disconnect detected\r\n");
					}
					else
					{   
						/*counting and showing the number of pulses -the time of disconnection*/
						Diff_PulseAmFail = this->un32_pulses_ - Start_PulseAmFail;
					}
        
					if ((((Diff_PulseAmFail % 10) == 0) && (last_diff_sent != Diff_PulseAmFail)))
					{

						sprintf(buff_temp, "Accum lost %d\r\n", (int)Diff_PulseAmFail);
						Serial_PutString(buff_temp);
						last_diff_sent = Diff_PulseAmFail;
					}
					this->get_mem()->check_connections(3);
					// valids have 4% fail rate
					// If we take .04 as p_fail - the statistics that we get 10 random fails in sequence
					// is .04^10 = 1^-14 which is kinda small. So if we get a Fail here
					// Its either non random distributed or a real fail
				   /* report problem the Am is disconnected more then 50 pulse*/
				    if(Diff_PulseAmFail > 50)  
					{
						Diff_PulseAmFail = 0;
						AM_Fail_Status = 0;
						Serial_PutString(stall_dynamic_str);
						Serial_PutString(stall_dynamic_reason_mem);
						reason = 6;
						this->get_mem()->LOG_DEBUG(0x100 + reason, this->un32_am_sn_, this->un32_apt_sn_, this->un32_pulses_);
						return reason;
					}
				}
				else if (AM_Fail_Status) 
				{
					Time_Of_Disconnect = HAL_GetTick() - Start_Am_disconnect_Time;
					/*resuming connections to the Am*/
					sprintf(buff_temp, "Resumed after p: %d,t: %d\r\n", (int)Diff_PulseAmFail, (int)Time_Of_Disconnect);
					Serial_PutString(buff_temp);
					Diff_PulseAmFail = 0;
					AM_Fail_Status = 0;
				}
			}
		case V12_TEST:
			{
				if (this->get_adc()->V12_ == 0)
				{
					this->get_screen()->tx_send_general_error(668);        // the adc is bad dynamicly
					Serial_PutString(stall_dynamic_str);
					Serial_PutString(stall_dynamic_reason_a);
					reason = 2;
					this->get_mem()->LOG_DEBUG(0x100 + reason, this->un32_am_sn_, this->un32_apt_sn_, this->un32_pulses_);
					return reason;
				}
			}
		default:
			{
        
				break;
			}
    
		}
		return reason;
	}
	return reason;
}


    


void main_controller::stall_error_write_back()
{
    if (!this->is_b_am_valid())
    {
        /*
         *  Write to MCU memory:
         *  Error type, (4byte)
         *  AM serial, (4byte)
         *  pulses when it occured, (4byte)
         *  validity_matrix (4byte) - zeroed out -> means this error was overcome
         *  
         */
        
    }
}

bool main_controller::fire_is_pressed()
{
    static uint16_t button_history = 0;
    bool pressed = false;    
    this->is_b_is_interlocked();
    if (this->b_is_interlocked_)
    {
        // In the event of UartDebug, we want to disable read on fire pin cause otherwise a free floating pin is high. and causes a loop of start - stop motor.
        if (this->get_state()->bypass.value == 2 || reset_button_instead_of_fire)
        {
            if (RESET_PRESSED)
            {
                RESET_PRESSED = false;
                return true;
            }
            return false;
        }
        button_history = button_history << 1;
        this->device->read();
        button_history |= (bool)!this->device->StatusReg->current.bits.FIRE_STATUS;
        if ((button_history & 0xFF0F) == 0xFF00)
        { 
            pressed = true;
            button_history = 0x0;
        }
        if (pressed)
        {
            static char fire_str[] = "Fire Pressed!\r\n";
            Serial_PutString(fire_str);
        }
        return pressed;
    }
    return false;
}

void main_controller::init_PF_struct(struct PF_memory* pf_memory)
{
    pf_memory->pulses_last_time_written = &(this->last_written_pulse_count_);
    pf_memory->pulses_last_time_written_v = this->get_un32_pulses();
    pf_memory->pulses_need_to_write_on_PF = &(this->un32_pulses_);
    pf_memory->pulses_need_to_write_on_PF_v = this->get_un32_pulses();
    pf_memory->AM_address_to_write_on_PF = &(this->un32_cursor_);
    pf_memory->AM_address_to_write_on_PF_v = this->get_un32_cursor();
    pf_memory->state = &(this->get_state()->current);
}
