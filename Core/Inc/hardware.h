#pragma once
#include <stdint.h>

/** @addtogroup CORE
  * @{
  */

/** @addtogroup Armenta
  * @{
  */

/** @addtogroup Globals
  * @{
  */
#define USARTx_TX_DMA_CHANNEL             DMA1_Channel2
#define USARTx_RX_DMA_CHANNEL             DMA1_Channel3
#define DMAx_CLK_ENABLE()                __HAL_RCC_DMA1_CLK_ENABLE()

/* Definition for USARTx's NVIC */
#define USARTx_DMA_TX_IRQn                DMA1_Channel2_3_IRQn
#define USARTx_DMA_RX_IRQn                DMA1_Channel2_3_IRQn
#define USARTx_DMA_TX_IRQHandler          DMA1_Channel2_3_IRQHandler
#define USARTx_DMA_RX_IRQHandler          DMA1_Channel2_3_IRQHandler

#define DEBUG_PRINT         0 /**< TRUE - prints all globals, FALSE - no print out*/
#define DEBUG_NO_BEEP       0 /**< TRUE - supress beep and blin instead, FALSE - do beep*/
#define TECHNICIAN_ENABLED  0 /**< TRUE - starts in UartDebug state*/
#define PASS_SPI            0 /**< TRUE - skips the spi init functionality*/
#define PASS_PRESSURE_CHECK 0 /**< TRUE - works with low pressure*/
#define EXPERIMENTAL        0 /**< TRUE - after 10k make an init done save*/
#define FROM_BOOTLOADER     1 /**< TRUE - the application is launched from boot loader*/
#define MAX_HOLE_SIZE		5 /**< maximum size for missed writes consecutively*/
#define HARDWARE_V			2 /**< generation of pcb for 1.1*/

#define ADC_Presure         0 /**< index of dma output to the array*/
#define ADC_Battery         1 /**< index of dma output to the array*/

#define ADC_Presure_24_bar 	305 /**< below this engine will not start*/ 
#define ADC_Presure_22_bar 	282  //Thershold for red green sign on the screen
#define ADC_Presure_20_bar 	264 /**< below this engine will stall while running*/
#define ADC_STALL_CURRENT   80
#define ADC_TEST_CURRENT    25 //1.3A
#define ADC_TEST_PRESSURE_TH 100
#define ADC_TEST_DC_PRESSURE_TH 310
#define ADC_TEST_COUNTER_TH 14
#define ADC_STALL_TEMP		8500
#define ADC_FIRE_TEMP		9000
#define VERSION			   	"TEST_AM " __DATE__	/**< software version*/
#define MCU_VERSION_MAJOR   8       /*!< Major version */
#define MCU_VERSION_MINOR   2       /*!< Minor version */
#define MCU_VERSION_PATCH   0       /*!< Patch version */
#define MCU_VERSION_RC      1       /*!< Release candidate version */

#define APP_ADDRESS				((uint32_t)0x08008000u)
#define APP_ADDRESS_P			(uint32_t*)APP_ADDRESS
/** End address of application space (address of last byte) */
#define END_ADDRESS             (uint32_t)0x08020000
#define APP_SIZE                (uint32_t)(END_ADDRESS - APP_ADDRESS)

/** @}*/

/** @addtogroup SPI
  * @{
  */
// Memory OPCODES
#define End_4k_BORDER  4096
#define START_BLOCK_ADD_AM 0x00
#define READ_ARRAY1 0x0B
#define READ_ARRAY2 0x03
#define READ_DUAL 0x3B
#define READ_QUAD 0x6B

#define BLOCK_ERASE_4K 0x20
#define BLOCK_ERASE_32K 0x52
#define BLOCK_ERASE_64K 0xD8
#define ERASE_CHIP1 0x60
#define ERASE_CHIP2 0xC7

#define BYTE_PROGRAM 0x02

#define WRITE_ENABLE 0x06
#define WRITE_DISABLE 0x04

#define READ_STATUS_REGISTER1 0x05
#define READ_STATUS_REGISTER2 0x35
#define WRITE_STATUS_REGISTER 0x01
#define WRITE_ENABLE_STATUS_REGISTER_VOLATILE 0x50

#define READ_MANUFACTURE_ID 0x9F
#define READ_DEVICE_ID 0x90

#define HIGH  (GPIO_PinState)1
#define LOW   (GPIO_PinState)0

//"AM", "APT", "MCU"
#define MEM_APPLICATOR		3
#define MEM_APT				2
#define MEM_MCU				1

#define RETRY_TIMES 5
/** @}*/

/** @addtogroup Auxilary
  * @{
  */
//	hdc1080_init(&hi2c1,Temperature_Resolution_14_bit,Humidity_Resolution_14_bit);
#define         HDC_1080_ADD                            0x40
#define         Configuration_register_add              0x02
#define         Temperature_register_add                0x00
#define         Humidity_register_add                   0x01

#define         START_BEEP 								100
#define         BEEP_COUNTER_200						110	
#define         BEEP_COUNTER_400						120	
#define         BEEP_COUNTER_0							130	
#define         KEY_PRESS								80	

#define         Beep_Every_X_Counting                   200 
		
#define SERIAL_PC									&huart1
#define SERIAL_SCREEN								&huart2
#define RXBUFFERSIZE									256

#define TIMEOUT_SCREEN									5000
#define TICKS_FOR_UPDATE								500
#define TICKS_SCREEN_UPDATE_LONG						1250
#define TICKS_SCREEN_UPDATE_SUPER_LONG					3500
#define AM_WARNING_TIMES								3
#define  MAX_APT_PULSE                                  204000
#define WARN_APT_PULSES									180000
// MOTOR PWM SETUP 16-10-18
#define MOTOR_1_PWM_ON_50 50
#define MOTOR_1_PWM_ON_75 75
#define MOTOR_1_PWM_ON 100
#define MOTOR_1_PWM_OFF 0

#define MOTOR_RUN										true
#define MOTOR_STOP										false
/** @}*/

/** @addtogroup ADC
  * @{
  */
#define SELECT_CHANNEL_0	0x0 //0b0000
#define SELECT_CHANNEL_1	0x1 //0b0001
#define SELECT_CHANNEL_2	0x2 //0b0010
#define SELECT_CHANNEL_3	0x3 //0b0011
#define SELECT_CHANNEL_4	0x4 //0b0100
#define SELECT_CHANNEL_5	0x5 //0b0101
#define SELECT_CHANNEL_6	0x6 //0b0110
#define SELECT_CHANNEL_7	0x7 //0b0111

#define ADC_RESERVED			0x8 //0b1000
#define ADC_RESERVED2			0x9 //0b1001

#define WRITE_CFR			0xa //0x0b1010
#define SELECT_TEST_VDIV2	0xb //0b1011
#define SELECT_TEST_REFM	0xc //0b1100
#define SELECT_TEST_REFP	0xd //0b1101

#define FIFOREAD			0xe //0b1110
#define HARDWARE_DEFAULT	0xf //0b1111

#define MAX_READ_ADC_CYCLES 3
#define ADC_OFFSET			0
/** @}*/

#define PIN_HIGH	GPIO_PIN_SET
#define PIN_LOW		GPIO_PIN_RESET
/// DEFINE OF ENUMS AND UNIONS USED IN CODE
typedef union  
{
	/*
	 * This is a union / 8 bit that can be accesed bitwise and as a byte.
	 */
	uint8_t value;
 
	struct {
		uint8_t INB_MOTOR : 1;
		uint8_t INA_MOTOR : 1;
		uint8_t QC : 1;
		uint8_t DECODER : 3;
		uint8_t QG : 1;
		uint8_t QH : 1;
	} bits;
 
	struct 
	{
		uint8_t nibble_PERIPH : 4;
		uint8_t nibble_CS : 4;
	} nibbles;
 
} byte8_t_reg_cs;

typedef union  
{
	/*
	 * This is a union / 8 bit that can be accesed bitwise and as a byte.
	 */
	uint8_t value;
 
	struct {
		uint8_t INB_MOTOR : 1;
		uint8_t INA_MOTOR : 1;
		uint8_t DECODER : 3;
		uint8_t CS_STATUS_INV : 1;
		uint8_t CS_MEM3 : 1;
		uint8_t CS_MEM2 : 1;
	} bits;
 
} byte8_t_reg_readback;
enum cs_decoder
{
Y0, 
// 0 - 000 Probly wrong 
CS_MEM3, 
// 1 - 001
CS_A2D, 
// 2 - 010
CS_READBACK_SER, 
// 3 - 011 Probly wrong
OPTO_CNT_CLRN, 
// 4 - 100 Probly wrong
CS_MEM2, 
// 5 - 101
CS_STATUS
// 6 - 110
};


typedef union  
{
	/*
	 * This is a union / 8 bit that can be accessed bitwise and as a byte.
	 */
	int16_t value;
	struct 
	{
		uint8_t b1;
		uint8_t b2;
	} bytes;
} channel;

enum select_channel
{
	V12,
	V_Motor,
	Motor_current_sense,
	Pressure,
	Undefined,
	Temp,
	V5,
	VDD_sense
};

typedef union  
{
	/*
	 * This is a union / 8 bit that can be accesed bitwise and as a byte.
		HAL_GPIO_WritePin(DEBUG_AM_OK_GPIO_Port, DEBUG_AM_OK_Pin, (GPIO_PinState)( value & (0x1 << 0)));
		HAL_GPIO_WritePin(DEBUG_APT_OK_GPIO_Port, DEBUG_APT_OK_Pin, (GPIO_PinState)(value & (0x1 << 1)));
		HAL_GPIO_WritePin(DEBUG_MCU_OK_GPIO_Port, DEBUG_MCU_OK_Pin, (GPIO_PinState)(value & (0x1 << 2)));
		HAL_GPIO_WritePin(DEBUG_PRESSURE_OK_GPIO_Port, DEBUG_PRESSURE_OK_Pin, (GPIO_PinState)(value & (0x1 << 3)));
		HAL_GPIO_WritePin(DEBUG_MOTOR_RUNNING_GPIO_Port, DEBUG_MOTOR_RUNNING_Pin, (GPIO_PinState)(value & (0x1 << 4)));
		HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_1_Pin, (GPIO_PinState)(value & (0x1 << 5)));
		HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_2_Pin, (GPIO_PinState)(value & (0x1 << 6)));
		HAL_GPIO_WritePin(DEBUG_STATE_1_GPIO_Port, DEBUG_STATE_3_Pin, (GPIO_PinState)(value & (0x1 << 7)));
	*/
	uint8_t value;
 
	struct {
		uint8_t DEBUG_AM_OK : 1;
		uint8_t DEBUG_APT_OK : 1;
		uint8_t DEBUG_MCU_OK : 1;
		uint8_t DEBUG_PRESSURE_OK : 1;
		uint8_t DEBUG_MOTOR_RUNNING : 1;
		uint8_t DEBUG_STATE_1 : 1;
		uint8_t DEBUG_STATE_2 : 1;
		uint8_t DEBUG_STATE_3 : 1;
	} bits;
 
} byte8_t_debug;


enum statesMachine
{
	Error,
	Init_before_main,
	Init,
	/**< Initialisation is still running. */
	Idle,
	/**< Idling and waiting for user input. */
	UpdateData,
	/**< Once in a while we send updates to screen to reduce traffic on uart1. */
	Active,
	/**< Motor is running, we have to update screen and write to memories. */
	UartDebug,
	/**< Special instrumentation state to when we just wait for uart4 commands. Entered by sending debug# */
	QA,
	TEST,
	AM_TEST_COMMAND,
};

typedef union
{
	uint8_t value;

	struct
	{
		uint8_t bypass1 : 1;
		uint8_t bypass2 : 1;
		uint8_t bypass3 : 1;
		uint8_t bypass4 : 1;
		uint8_t reserved : 4;
	}bits;
	
}bypass_state;

typedef union  
{
	/*
	 * This is a union / 8 bit that can be accesed bitwise and as a byte.
	 */
	uint8_t value;

	struct {
		uint8_t OPTO_A : 1;
		uint8_t OPTO_B : 1;
		uint8_t OPTO_C : 1;
		uint8_t OPTO_D : 1;
		uint8_t SWITCHES_STATUS : 1;
		uint8_t OPTO_STATUS : 1;
		uint8_t FIRE_STATUS : 1;
		uint8_t RESERVED : 1;

	} bits;
 
	struct 
	{
		uint8_t nibble_OPTO : 4;
		uint8_t nibble_PERIPH : 4;
	} nibbles;
 
} byte8_t_reg_status;

struct PF_memory
{
	uint32_t* pulses_last_time_written;
	uint32_t pulses_last_time_written_v;
	uint32_t* pulses_need_to_write_on_PF;
	uint32_t pulses_need_to_write_on_PF_v;
	uint32_t* AM_address_to_write_on_PF;
	uint32_t AM_address_to_write_on_PF_v;
	enum statesMachine* state;
};


typedef union  
{
	/*
	 * This is a union / 8 bit that can be accesed bitwise and as a byte.
	 */
	uint16_t value;
	struct 
	{
		uint8_t status1;
		uint8_t status2;
	} bytes;
 
	struct {
		uint8_t SRP0 : 1;
		uint8_t SEC  : 1;
		uint8_t TB   : 1;
		uint8_t BP2  : 1;
		uint8_t BP1  : 1;
		uint8_t BP0  : 1;
		uint8_t WEL  : 1;
		uint8_t WIP  : 1;
		uint8_t RES  : 1;
		uint8_t CMP  : 1;
		uint8_t LB3  : 1;
		uint8_t LB2  : 1;
		uint8_t LB1  : 1;
		uint8_t RES2  : 1;
		uint8_t QE  : 1;
		uint8_t SRP1 : 1;
	} bits;

 
} byte8_t_mem_status;
typedef union  
{
	/*
	 * This is a union / 8 bit that can be accesed bitwise and as a byte.
	 */
	uint8_t value;
 
	struct {
		uint8_t WIP  : 1;
		uint8_t WEL  : 1;
		uint8_t BP0  : 1;
		uint8_t BP1  : 1;
		uint8_t BP2  : 1;
		uint8_t TB   : 1;
		uint8_t SEC  : 1;

		uint8_t SRP0 : 1;

	} bits;

 
} byte8_t_mem_status1;
typedef union  
{
	/*
	 * This is a union / 8 bit that can be accesed bitwise and as a byte.
	 */
	uint8_t value;
 
	struct {
		uint8_t SRP1 : 1;
		uint8_t QE  : 1;
		uint8_t RES2  : 1;
		uint8_t LB1  : 1;
		uint8_t LB2  : 1;
		uint8_t LB3  : 1;
		uint8_t CMP  : 1;
		uint8_t RES  : 1;
	} bits;

 
} byte8_t_mem_status2;

typedef union  
{
	uint8_t serial[16];
	struct 
	{
		uint32_t error; // ST - 01 means stall reason 01
		uint32_t am_sn; // AM Serial
		uint32_t apt_sn;
		uint32_t am_count; // Number of counts the am have done
	} fields;
} stall_log_out;
/** @}*/
/** @}*/
