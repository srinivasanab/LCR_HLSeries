/*
 * modem.h
 *
 *  Created on: Jun 30, 2019
 *      Author: Arjun Dongre
 */

#ifndef USRINC_MODEM_H_
#define USRINC_MODEM_H_

#define MODEM_COMMUNICATION_RETRY_COUNT		10 	//Max retries done while sending AT command.
#define MODEM_RESTART_RETRY_COUNT			5	//Max modem restarts done while checking the modem communication.
#define MODEM_CMD_RESPONSE_WAIT_TIME		500 //Wait time to receive AT command response.(In milliseconds)

#define MODEM_TURN_ON_WAIT_TIME				10 //in seconds.

#define SIM1								0 	//Defines SIM 1 is selected.
#define SIM2								1	//Defines SIM 2 is selected.

#define MMODEM_SIM_DETECTION_ATTEMPTS		5	//If SIM doesn't exists then attempts will be limited this.

#define MODEM_STATUS_CHECK_TIME				5 // Checks tcp status for these many seconds.

#define MODEM_SIM_DETECTION_RETRY_COUNT		120//Attempts to check SIM registration.
#define MODEM_GPRS_CONNECTION_TIME_OUT		8000//wait time to activate gprs.

//macros to define tcp connection health.
#define MODEM_TCP_SOCKET_STATUS_DEFINED_BUT_NOT_USED 	1
#define MODEM_TCP_SOCKET_STATUS_CLOSING					4
#define MODEM_TCP_SOCKET_STATUS_CLOSED					5
#define MODEM_TCP_SOCKET_STATUS_CONNECTING				3


#define MODEM_CONNECTING_SERVER_RETRY_COUNT	5	//No of retries to connect server.
#define MODEM_CONNECTING_GPRS_RETRY_COUNT	3	//No of retries to activate gprs.


//Defined Index no in the received TCP packet
#define MODEM_TCP_RXD_MODBUS_CFG_START_INDEX 			11
#define MODEM_TCP_RXD_MODBUS_CFG_SLOT_INDEX				1 + MODEM_TCP_RXD_MODBUS_CFG_START_INDEX
#define MODEM_TCP_RXD_MODBUS_CFG_DATA_LENGTH_HIGH_INDEX 4 + MODEM_TCP_RXD_MODBUS_CFG_START_INDEX
#define MODEM_TCP_RXD_MODBUS_CFG_DATA_LENGTH_LOW_INDEX 	5 + MODEM_TCP_RXD_MODBUS_CFG_START_INDEX
#define MODEM_TCP_RXD_MODBUS_CFG_PREFIX_LEN				6
#define MODEM_TCP_RXD_MODBUS_CFG_ACTUAL_CONFIG_INDEX	MODEM_TCP_RXD_MODBUS_CFG_PREFIX_LEN + MODEM_TCP_RXD_MODBUS_CFG_START_INDEX
#define MODEM_TCP_RXD_MODBUS_CFG_END					0x04

#define MODEM_TCP_RXD_MODBUS_CFG_SLOT_BASE				0x50

//structure to store retry counts
typedef struct _modemRetryCount
{
	uint8_t gprsConnect;	//GPRS retry count.
	uint8_t serverConnect;	//Server retry count.
}modemRetryCount_t;


//Enum defines the health of the modem.
typedef enum {
	MODEM_STATUS_INVALID,	//Invalid event.
	MODEM_STATUS_TCP_ERROR,
	MODEM_STATUS_TCP_READY,
	MODEM_STATUS_TCP_CPNNECTED,
	MODEM_STATUS_DEVICE_RESTART,
	MODEM_STATUS_END
}modemHealth_e;

//Global functions.
void usfn_TurnONModem(void);
bool 	usfn_CheckModemCommunication(void);
bool 	usfn_GetModemDetails(void);
bool 	usfn_ConfigModemSettings(void);
char* 	usfn_GetAPNName(char*);
bool 	usfn_ModemSimInit(bool);
bool 	usfn_ConnectToCloud(void);
modemHealth_e	usfn_CheckModemStatus(void);
bool 	usfn_GetTimeAndDate(void);



#endif /* USRINC_MODEM_H_ */
