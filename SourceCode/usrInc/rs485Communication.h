/*
 * rs485Communication.h
 *
 *  Created on: Jul 6, 2019
 *      Author: Arjun Dongre
 */

#ifndef USRINC_RS485COMMUNICATION_H_
#define USRINC_RS485COMMUNICATION_H_

//rs485communication thread priority.
#define RS485_COMMUNICATION_HANDLE_PRIORITY				3
//rs485communication stack size.
#define RS485_COMMUNICATION_HANDLE_STACK_SIZE			15*1024 //in kB.


#define MODBUS_RX_BUFFER_MAX							261 //Max data could be received on modbus
#define MODBUS_QUERY_SIZE								8	//Size of the modbus command.
#define MODBUS_QUERY_LEN_WITHOUT_CRC					6	//modbus command without crc.
#define MODBUS_COMMUNICATION_ERROR_COUNT_TH				9	//Max modbus communcation error count. If device doesn't communicate then move to next configuration.
#define MODBUS_EXPECTED_DATA_LENGTH_INCLUDE				5	//offset added to the expected data length from the device.
#define MODBUS_QUERY_RETRY_COUNT						3	// Max modbus retries done if there is no response from the modbus device.
#define MODBUS_RESPONSE_WAIT_TIME						1 	//Wait time to receive modbus data in Seconds.
#define MODBUS_ACTUAL_DATA_OFFSET						3	//Offset where actual data starts in the modbus response.

//Enables rs485 transmission in the RS485 chip.
#define RS485_TX_ENABLE()		SET_RS485_DE();SET_RS485_RE()
//Enables rs485 reception in the RS485 chip.
#define RS485_RX_ENABLE()		RESET_RS485_DE();RESET_RS485_RE()


//Globals
extern mutex_t	g_mutex_rs485DataLock;

void rs485CommunicationHandle(void *p);
void usfn_PrintArray(uint8_t*, uint16_t);

#endif /* USRINC_RS485COMMUNICATION_H_ */
