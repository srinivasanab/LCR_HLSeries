/*
 * rs485Communication.h
 *
 *  Created on: Jul 6, 2019
 *      Author: Arjun Dongre
 */

#ifndef USRINC_RS485PROCESS_H_
#define USRINC_RS485PROCESS_H_


#define NO_OF_MAX_MODBUS_CONFIGURATION			30 //Max no of modbus configurations considered.
#define RS485_MAX_DATA_LENGTH					250	// Max data from each modbus configuration in bytes.

#define RS485_MAX_CONFIG_LENGTH					500 // Max length of the modbus configuration in bytes.
#define RS485_MIN_CONFIG_LENGTH					3 	// Min length of the modbus configuration in bytes.

#define RS485_CONFIG_NEXT_FUNCTION_REG			0x3B	//Delimeter present in the modbus configurations.
#define RS485_CONFIG_NEXT_SLAVE					0x2B	//Delimeter present in the modbus configurations.
#define RS485_CONFIG_END						0x2E	//Delimeter represent end of modbus configurations.


#define FUNCTION_CODE_COIL_STATUS				0x01	//Function code for coil.
#define FUNCTION_CODE_INPUT_STATUS				0x02	//Function code for input.
#define FUNCTION_CODE_HOLDING_REG				0x03	//Function code for holding register.
#define FUNCTION_CODE_INPUT_REG					0x04	//Function code for Input registers

//Index in the modbus configuration.
#define MODBUS_CFG_FRAME_ID_INDEX				0x00
#define MODBUS_CFG_SLAVE_ID_INDEX				0x01
#define MODBUS_CFG_BAUDRATE_INDEX				0x02

//structure to hold modbus configuration and its information.
typedef struct _modbusConfigMap
{
//	uint8_t		frameId;
//	uint16_t	queryCount;
	uint8_t		configChannel;		// modbus configuration slot.
	uint16_t	dataLength;			// expected modbus data length.
	uint16_t	modBusCfgCmdLength;	// Length of the modbus configuration buffer.
	uint16_t	modBusErrorCount;	// Error count which is generated while polling.
	uint8_t		modBusConfigCmd[RS485_MAX_CONFIG_LENGTH]; // modbus configuration.
	uint8_t		modBusData[RS485_MAX_DATA_LENGTH];	// buffer to store modbus.
}modbusConfigMap_t;
extern modbusConfigMap_t *g_modbusConfigMap;

#ifndef cJSON__h
#include "cJSON.h"
#endif

//Global funtions.
void usfn_SetupModbusRegisters(cJSON*);
uint16_t usfn_VerifyAndGetDetailsOfModBusConfiguration(uint8_t*, uint16_t);

#endif /* USRINC_RS485COMMUNICATION_H_ */
