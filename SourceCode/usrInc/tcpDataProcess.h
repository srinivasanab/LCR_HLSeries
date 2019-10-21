#ifndef USRINC_TCPDATAPROCESS_H_
#define USRINC_TCPDATAPROCESS_H_

#ifndef RS485_MAX_DATA_LENGTH
#include "rs485Process.h"
#endif

#ifndef _STDBOOL_H
#include "stdbool.h"
#endif

// mcros for the type of the data to be sent to cloud.
#define SEND_TCP_DATA_REGULAR_FLAG		0
#define SEND_TCP_DATA_ALARM_FLAG		1
#define SEND_TCP_DATA_HEART_BEAT_FLAG	3
#define SEND_TCP_POWER_UP_MSG			4
#define SEND_TCP_MODBUS_CFG_ACK_FLAG	5

//mcros for the data present in the tcp packet.
#define TCP_DATA_STX					0x24	//Start of the packet.
#define TCP_DATA_ETX					0x7E	//End of the packet.
#define TCP_DATA_APP_ID					0x80	//Application ID
#define TCP_DATA_IMEI_END				0x46	//End of the IMEI no


#define TCP_DATA_PCKET_TYPE_REGULAR		0x52	// Regular type data
#define TCP_DATA_PCKET_TYPE_FAULT		0x4C	// Alarm type data

#define TCP_DATA_DEVICE_ID_ALARM		0xFB
#define TCP_DATA_DEVICE_ID_CMD_ACK		0xFD
#define TCP_DATA_DEVICE_ID_HEART_BEAT	0xFC
#define TCP_DATA_DEVICE_ID_POWER_UP_MSG	0xF4
#define TCP_DATA_DEVICE_ID_DIGITAL_IO	0xFA


#define TCP_DATA_DEVICE_VERSION_ID		0x06
#define TCP_DATA_FRAME_ID				0x01

#define TCP_DATA_LEN_ALARM				0x04
#define TCP_DATA_LEN_CMD_ACK			0x02
#define TCP_DATA_LEN_HEART_BEAT			0x03
#define TCP_DATA_LEN_POWER_UP_MSG		0x24
#define TCP_DATA_LEN_DIGITAL_IO			0x0C

//Indeces and offsets in the tcp packet.
#define TCP_DATA_OFFSET_REGULAR_DATA	0x1A
#define TCP_DATA_LEN_PREFIX				0x1A
#define TCP_DATA_LEN_SUFFIX_LENGTH		0x03


//AT command pattern to push data to the cloud.
#define TCP_DATA_TXN_END_OF_PATTERN		"--EOF--Pattern--"


//period to push data to server.
#define TCP_DATA_PERIOD_HEART_BEAT		6*60	//in seconds.
#define TCP_DATA_PERIOD_ALARM			5*60	//in seconds.
#define TCP_BACKUP_DATA_SEND_PERIOD		5		//in secs

#define TCP_DATA_BASE_YEAR				1980

#ifndef LCR_CONF_TCP_DATA_IN_SINGLE_FRAME
#include "lcrConfig.h"
#endif

#if LCR_CONF_TCP_DATA_IN_SINGLE_FRAME
	#define LCR_CONF_TCP_DATA_EXTRA_BYTES_PADD_SIZE 	40+40+20+20	// no of zeros to be padded at the end of packet.
	#define LCR_CONF_SINGLE_FRAME_TCP_DATA_DEVICE_ID	0xE3
	#define LCR_CONF_SINGLE_FRAME_TCP_DATA_FRAME_ID		0x12
	#define LCR_CONF_SINGLE_FRAME_TCP_DATA_LEN_MAX		5*1024
#endif //LCR_CONF_TCP_DATA_IN_SINGLE_FRAME

//tcp packet structure.
typedef struct __attribute__((packed)) _tcpData
{
	uint8_t		stx;		//start of the frame.
	uint8_t		appID;		//Apllication ID.
	uint16_t	year;		//Year.
	uint8_t		month;		//Month.
	uint8_t		date;		//Date
	uint8_t		hours;		//Hours.
	uint8_t		minutes;	//Minutes.
	uint8_t		seconds;	//Seconds.
	uint16_t	packetSequence; // Packet sequence.
	uint64_t	imei;		// IMEI no.
	uint8_t		imeiEnd;	// End of the IMEI no.
	uint8_t		packetType;	// Type of the packet.
	uint8_t		deviceID;	// Subtype eg. ID of modbus configuration or F4, FC
	uint8_t		deviceVersionID; // Normally the value will be 06 and for modbus data this defines the baudrate.
	uint8_t		frameID;	// modbus data this defines the slave id.
	uint16_t	noOfBytes;	// length of the actual data
#if LCR_CONF_TCP_DATA_IN_SINGLE_FRAME
	uint8_t		data[LCR_CONF_SINGLE_FRAME_TCP_DATA_LEN_MAX];
#else
	uint8_t		data[RS485_MAX_DATA_LENGTH]; //buffer to the actual data.
#endif
}tcpData_t;

//Global functions.
bool 	usfn_CollectDataAndPushToServer(uint8_t);
bool 	usfn_PushDataToServer(tcpData_t*, uint16_t);
#if LCR_CONF_TCP_DATA_IN_SINGLE_FRAME
void	usfn_CollectRegularDataInSingleFrame(tcpData_t*);
#else
void 	usfn_CollectRegularDataInMultiFrame(tcpData_t*, uint8_t);
#endif
void	 usfn_GetInitializedPacket(tcpData_t*);
uint16_t usfn_CompleteTheTcpPacket(tcpData_t*);
#endif
