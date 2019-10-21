/*
 * digitalIO.h
 *
 *  Created on: Jun 30, 2019
 *      Author: Arjun Dongre
 */

#ifndef USRINC_DIGITALIO_H_
#define USRINC_DIGITALIO_H_

#ifndef _STDBOOL_H
#include "stdbool.h"
#endif
//--------- USER_LED_DIO------------------------------------------
//define pin numbers
#define USER_LED_1_LINE_1_PIN			GPIOB_PIN5			//	Port B or D
#define USER_LED_1_LINE_2_PIN			GPIOB_PIN4			//	Port B
#define USER_LED_2_LINE_1_PIN			GPIOB_PIN3			//	Port B
#define USER_LED_2_LINE_2_PIN			GPIOD_PIN1			//	Port D
#define USER_LED_3_LINE_1_PIN			GPIOA_PIN15			//	Port A
#define USER_LED_3_LINE_2_PIN			GPIOC_PIN4			//	Port C

//define port names
#define USER_LED_1_LINE_1_PIN_PORT		GPIOB
#define USER_LED_1_LINE_2_PIN_PORT		GPIOB
#define USER_LED_2_LINE_1_PIN_PORT		GPIOB
#define USER_LED_2_LINE_2_PIN_PORT		GPIOD
#define USER_LED_3_LINE_1_PIN_PORT		GPIOA
#define USER_LED_3_LINE_2_PIN_PORT		GPIOC


//LED toggling macros
#define TOGGLE_LED_1_LINE_1()			palTogglePad(USER_LED_1_LINE_1_PIN_PORT,USER_LED_1_LINE_1_PIN)
#define TOGGLE_LED_1_LINE_2()			palTogglePad(USER_LED_1_LINE_2_PIN_PORT,USER_LED_1_LINE_2_PIN)
#define TOGGLE_LED_2_LINE_1()			palTogglePad(USER_LED_2_LINE_1_PIN_PORT,USER_LED_2_LINE_1_PIN)
#define TOGGLE_LED_2_LINE_2()			palTogglePad(USER_LED_2_LINE_2_PIN_PORT,USER_LED_2_LINE_2_PIN)
#define TOGGLE_LED_3_LINE_1()			palTogglePad(USER_LED_3_LINE_1_PIN_PORT,USER_LED_3_LINE_1_PIN)
#define TOGGLE_LED_3_LINE_2()			palTogglePad(USER_LED_3_LINE_2_PIN_PORT,USER_LED_3_LINE_2_PIN)



//LED ON macros
#define SET_LED_1_LINE_1()				palSetPad(USER_LED_1_LINE_1_PIN_PORT,USER_LED_1_LINE_1_PIN)
#define SET_LED_1_LINE_2()				palSetPad(USER_LED_1_LINE_2_PIN_PORT,USER_LED_1_LINE_2_PIN)
#define SET_LED_2_LINE_1()				palSetPad(USER_LED_2_LINE_1_PIN_PORT,USER_LED_2_LINE_1_PIN)
#define SET_LED_2_LINE_2()				palSetPad(USER_LED_2_LINE_2_PIN_PORT,USER_LED_2_LINE_2_PIN)
#define SET_LED_3_LINE_1()				palSetPad(USER_LED_3_LINE_1_PIN_PORT,USER_LED_3_LINE_1_PIN)
#define SET_LED_3_LINE_2()				palSetPad(USER_LED_3_LINE_2_PIN_PORT,USER_LED_3_LINE_2_PIN)

//LED OFF macros
#define RESET_LED_1_LINE_1()			palClearPad(USER_LED_1_LINE_1_PIN_PORT,USER_LED_1_LINE_1_PIN)
#define RESET_LED_1_LINE_2()			palClearPad(USER_LED_1_LINE_2_PIN_PORT,USER_LED_1_LINE_2_PIN)
#define RESET_LED_2_LINE_1()			palClearPad(USER_LED_2_LINE_1_PIN_PORT,USER_LED_2_LINE_1_PIN)
#define RESET_LED_2_LINE_2()			palClearPad(USER_LED_2_LINE_2_PIN_PORT,USER_LED_2_LINE_2_PIN)
#define RESET_LED_3_LINE_1()			palClearPad(USER_LED_3_LINE_1_PIN_PORT,USER_LED_3_LINE_1_PIN)
#define RESET_LED_3_LINE_2()			palClearPad(USER_LED_3_LINE_2_PIN_PORT,USER_LED_3_LINE_2_PIN)

//Defining Server Status LED, Gprs status LED and RS485 LED
#define SERVER_LED_OFF()				RESET_LED_1_LINE_1();RESET_LED_1_LINE_2()
#define SERVER_LED_ON()					SET_LED_1_LINE_1();SET_LED_1_LINE_2()

#define GPRS_LED_OFF()					RESET_LED_2_LINE_1();RESET_LED_2_LINE_2()
#define GPRS_LED_ON()					SET_LED_2_LINE_1();SET_LED_2_LINE_2()

#define RS485_RX_LED_ON()				SET_LED_3_LINE_1()
#define RS485_TX_LED_ON()				SET_LED_3_LINE_2()

#define RS485_RX_LED_OFF()				RESET_LED_3_LINE_1()
#define RS485_TX_LED_OFF()				RESET_LED_3_LINE_2()

//--------- GSM_Modem_DIO-------------------------------------------------------------------------------
//define pin numbers
#define GSM_MODEM_ON_PIN 				GPIOB_PIN0			//	PORTB
#define GSM_MODEM_RESET_PIN				GPIOC_PIN5			//	PORTC

//define poprt names
#define GSM_MODEM_ON_PIN_PORT 			GPIOB
#define GSM_MODEM_RESET_PIN_PORT		GPIOC

//pin set macros
#define SET_GSM_MODEM_ON()				palSetPad(GSM_MODEM_ON_PIN_PORT,GSM_MODEM_ON_PIN)
#define SET_GSM_MODEM_RESET()			palSetPad(GSM_MODEM_RESET_PIN_PORT,GSM_MODEM_RESET_PIN)

// pin reset macros
#define RESET_GSM_MODEM_ON()			palClearPad(GSM_MODEM_ON_PIN_PORT,GSM_MODEM_ON_PIN)
#define RESET_GSM_MODEM_RESET()			palClearPad(GSM_MODEM_RESET_PIN_PORT,GSM_MODEM_RESET_PIN)

//--------- SIM selection pin-------------------------------------------------------------------------------
//(This macros are of no use as there is no connection for sim select pin in modem)
//define pin no
#define MODEM_SIM_SELECT_PIN1			GPIOD_PIN10
#define MODEM_SIM_SELECT_PIN2			GPIOD_PIN11

//define port names
#define MODEM_SIM_SELECT_PIN1_PORT		GPIOD
#define MODEM_SIM_SELECT_PIN2_PORT		GPIOD

//pin set macro
#define SET_MODEM_SIM_SELECT_PIN1()		palSetPad(MODEM_SIM_SELECT_PIN1_PORT,MODEM_SIM_SELECT_PIN1)
#define SET_MODEM_SIM_SELECT_PIN2()		palSetPad(MODEM_SIM_SELECT_PIN2_PORT,MODEM_SIM_SELECT_PIN2)

//pin reset macro
#define RESET_MODEM_SIM_SELECT_PIN1()	palClearPad(MODEM_SIM_SELECT_PIN1_PORT,MODEM_SIM_SELECT_PIN1)
#define RESET_MODEM_SIM_SELECT_PIN2()	palClearPad(MODEM_SIM_SELECT_PIN2_PORT,MODEM_SIM_SELECT_PIN2)

// macro function to select pin
#define SELECT_SIM1()					RESET_MODEM_SIM_SELECT_PIN1();RESET_MODEM_SIM_SELECT_PIN2();g_lcrGlobalFlags.simSelect = SIM1
#define SELECT_SIM2()					SET_MODEM_SIM_SELECT_PIN1();SET_MODEM_SIM_SELECT_PIN2();g_lcrGlobalFlags.simSelect = SIM2

//--------- MODBUS_DE_&_RE DIO---------------
//define pin no
#define RS485_DE_PIN					GPIOD_PIN15			//	Port D
#define RS485_RE_PIN					GPIOD_PIN14			//	Port D

//define port name for pins
#define RS485_DE_PIN_PORT				GPIOD		//	Port D
#define RS485_RE_PIN_PORT				GPIOD		//	Port D

//pin set macros
#define SET_RS485_DE()					palSetPad(RS485_DE_PIN_PORT,RS485_DE_PIN)
#define SET_RS485_RE()					palSetPad(RS485_RE_PIN_PORT,RS485_RE_PIN)

//pin reset macros
#define RESET_RS485_DE()				palClearPad(RS485_DE_PIN_PORT,RS485_DE_PIN)
#define RESET_RS485_RE()				palClearPad(RS485_RE_PIN_PORT,RS485_RE_PIN)



/*__________________________________________________________________________

 IO Card Read*/
// define pin no
#define IO_CARD_DIGITAL_INPUT0_PIN		GPIOE_PIN0
#define IO_CARD_DIGITAL_INPUT1_PIN		GPIOE_PIN1
#define IO_CARD_DIGITAL_INPUT2_PIN		GPIOE_PIN2
#define IO_CARD_DIGITAL_INPUT3_PIN		GPIOE_PIN3
#define IO_CARD_DIGITAL_INPUT4_PIN		GPIOE_PIN4
#define IO_CARD_DIGITAL_INPUT5_PIN		GPIOE_PIN5
#define IO_CARD_DIGITAL_INPUT6_PIN		GPIOE_PIN6
#define IO_CARD_DIGITAL_INPUT7_PIN		GPIOE_PIN7
#define IO_CARD_DIGITAL_INPUT8_PIN		GPIOE_PIN8
#define IO_CARD_DIGITAL_INPUT9_PIN		GPIOE_PIN9
#define IO_CARD_DIGITAL_INPUT10_PIN		GPIOE_PIN10
#define IO_CARD_DIGITAL_INPUT11_PIN		GPIOE_PIN11


#define IO_CARD_TAMPER_SWITCH0_PIN		GPIOE_PIN12
#define IO_CARD_TAMPER_SWITCH1_PIN		GPIOE_PIN13
#define IO_CARD_TAMPER_SWITCH2_PIN		GPIOE_PIN14
#define IO_CARD_TAMPER_SWITCH3_PIN		GPIOE_PIN15
#define IO_CARD_TAMPER_SWITCH4_PIN		GPIOB_PIN1

//define port names for pins
#define IO_CARD_DIGITAL_INPUT0_PIN_PORT			GPIOE
#define IO_CARD_DIGITAL_INPUT1_PIN_PORT			GPIOE
#define IO_CARD_DIGITAL_INPUT2_PIN_PORT			GPIOE
#define IO_CARD_DIGITAL_INPUT3_PIN_PORT			GPIOE
#define IO_CARD_DIGITAL_INPUT4_PIN_PORT			GPIOE
#define IO_CARD_DIGITAL_INPUT5_PIN_PORT			GPIOE
#define IO_CARD_DIGITAL_INPUT6_PIN_PORT			GPIOE
#define IO_CARD_DIGITAL_INPUT7_PIN_PORT			GPIOE
#define IO_CARD_DIGITAL_INPUT8_PIN_PORT			GPIOE
#define IO_CARD_DIGITAL_INPUT9_PIN_PORT			GPIOE
#define IO_CARD_DIGITAL_INPUT10_PIN_PORT		GPIOE
#define IO_CARD_DIGITAL_INPUT11_PIN_PORT		GPIOE

#define IO_CARD_TAMPER_SWITCH0_PIN_PORT			GPIOE
#define IO_CARD_TAMPER_SWITCH1_PIN_PORT			GPIOE
#define IO_CARD_TAMPER_SWITCH2_PIN_PORT			GPIOE
#define IO_CARD_TAMPER_SWITCH3_PIN_PORT			GPIOE
#define IO_CARD_TAMPER_SWITCH4_PIN_PORT			GPIOB

// Digital outputs
//define pin no
#define IO_CARD_DIGITAL_OUTPUT0_PIN				GPIOD_PIN4
#define IO_CARD_DIGITAL_OUTPUT1_PIN				GPIOD_PIN5
#define IO_CARD_DIGITAL_OUTPUT2_PIN				GPIOD_PIN6
#define IO_CARD_DIGITAL_OUTPUT3_PIN				GPIOD_PIN7

//define port names for pins
#define IO_CARD_DIGITAL_OUTPUT0_PIN_PORT		GPIOD
#define IO_CARD_DIGITAL_OUTPUT1_PIN_PORT		GPIOD
#define IO_CARD_DIGITAL_OUTPUT2_PIN_PORT		GPIOD
#define IO_CARD_DIGITAL_OUTPUT3_PIN_PORT		GPIOD

//macro functions to set digital output
#define SET_DIGITAL_OUT0()		palSetPad(IO_CARD_DIGITAL_OUTPUT0_PIN_PORT,IO_CARD_DIGITAL_OUTPUT0_PIN)
#define SET_DIGITAL_OUT1()		palSetPad(IO_CARD_DIGITAL_OUTPUT1_PIN_PORT,IO_CARD_DIGITAL_OUTPUT1_PIN)
#define SET_DIGITAL_OUT2()		palSetPad(IO_CARD_DIGITAL_OUTPUT2_PIN_PORT,IO_CARD_DIGITAL_OUTPUT2_PIN)
#define SET_DIGITAL_OUT3()		palSetPad(IO_CARD_DIGITAL_OUTPUT3_PIN_PORT,IO_CARD_DIGITAL_OUTPUT3_PIN)

//macro functions to reset digital output
#define RESET_DIGITAL_OUT0()	palClearPad(IO_CARD_DIGITAL_OUTPUT0_PIN_PORT,IO_CARD_DIGITAL_OUTPUT0_PIN)
#define RESET_DIGITAL_OUT1()	palClearPad(IO_CARD_DIGITAL_OUTPUT1_PIN_PORT,IO_CARD_DIGITAL_OUTPUT1_PIN)
#define RESET_DIGITAL_OUT2()	palClearPad(IO_CARD_DIGITAL_OUTPUT2_PIN_PORT,IO_CARD_DIGITAL_OUTPUT2_PIN)
#define RESET_DIGITAL_OUT3()	palClearPad(IO_CARD_DIGITAL_OUTPUT3_PIN_PORT,IO_CARD_DIGITAL_OUTPUT3_PIN)

//macro functions to toggle digital output
#define TOGGLE_DIGITAL_OUT0()	palTogglePad(IO_CARD_DIGITAL_OUTPUT0_PIN_PORT,IO_CARD_DIGITAL_OUTPUT0_PIN)
#define TOGGLE_DIGITAL_OUT1()	palTogglePad(IO_CARD_DIGITAL_OUTPUT1_PIN_PORT,IO_CARD_DIGITAL_OUTPUT1_PIN)
#define TOGGLE_DIGITAL_OUT2()	palTogglePad(IO_CARD_DIGITAL_OUTPUT2_PIN_PORT,IO_CARD_DIGITAL_OUTPUT2_PIN)
#define TOGGLE_DIGITAL_OUT3()	palTogglePad(IO_CARD_DIGITAL_OUTPUT3_PIN_PORT,IO_CARD_DIGITAL_OUTPUT3_PIN)

//_________________________________________________________________________________________________
#ifndef USRSRC_LCRCONFIG_H_
#include "lcrConfig.h"
#endif //USRSRC_LCRCONFIG_H_

#ifdef LCR_CONF_DIGITAL_IO_BOARD_CONNECTED

#define DIGITAL_IO_BOARD_HANDLE_STACK_SIZE		1*1024
#define DIGITAL_IO_BOARD_HANDLE_PRIORITY		3

//All digital input pins connected to portE
#define DIGITAL_IO_PINS_PORT					GPIOE

//A structure to store digital io and analog status.
typedef struct _dIoAdcData
{
	union
	{
		struct
		{
			uint16_t dIn00	: 1;
			uint16_t dIn01	: 1;
			uint16_t dIn02	: 1;
			uint16_t dIn03	: 1;
			uint16_t dIn04	: 1;
			uint16_t dIn05	: 1;
			uint16_t dIn06	: 1;
			uint16_t dIn07	: 1;
			uint16_t dIn08	: 1;
			uint16_t dIn09	: 1;
			uint16_t dIn10	: 1;
			uint16_t dIn11	: 1;
			uint16_t dIn12	: 1;
			uint16_t dIn13	: 1;
			uint16_t dIn14	: 1;
			uint16_t dIn15	: 1;

			uint8_t dIn16	: 1;
			uint8_t 		: 7;

			uint8_t dOt00	: 1;
			uint8_t dOt01	: 1;
			uint8_t dOt02	: 1;
			uint8_t dOt03	: 1;
			uint8_t spare00	: 1;
			uint8_t spare01	: 1;
			uint8_t spare02	: 1;
			uint8_t spare03	: 1;

			uint16_t adc0;

			uint16_t adc1;

			uint16_t adc2;

			uint16_t adc3;
		};
		struct
		{
			uint16_t digitalInput1;
			uint8_t	 digitalInput2;
			uint8_t  digitalOutput;
		};
	};
} dIoAdcData_t;
extern dIoAdcData_t g_dIoAdcData_t;

//Global functions.
void digitalIOBoardHandle(void *p);
#endif //LCR_CONF_DIGITAL_IO_BOARD_CONNECTED

#endif /* USRINC_DIGITALIO_H_ */
