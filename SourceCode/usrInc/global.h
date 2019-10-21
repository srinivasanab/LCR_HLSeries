/*
 * global.h
 *
 *  Created on: Jun 30, 2019
 *      Author: Arjun Dongre
 */

#ifndef USRINC_GLOBAL_H_
#define USRINC_GLOBAL_H_


//defining serial ports
#define DEBUG_PORT		&SD1 // for debugging.
#define RS485_PORT		&SD6 // for rs485 communication.
#define MODEM_PORT		&SD2 // for modem communication.

//stream for chprintf function.
extern BaseSequentialStream *g_chp;

//RTC macros
#define RTC_NO_OF_YEARS_ADDED_TO_BASE	20	//No of years to be added to the base year set in the device.
#define RTC_SECONDS_TO_BE_ADDED_FOR_GST 19800	// No of seconds to be added to get IST timestamp.


//Periodicity to restart the device.
#define LCR_RESTART_PERIOD				60*60*6 //in seconds
//Periodicity to resynch the time from the server.
#define LCR_TIME_RESYNCH_PERIOD			60*30 //in seconds
//structure to store modem details.
typedef struct _gsmModemDetails
{
	char		cgmm[20];	//modem model.
	char		cmgr[70];	//model revision.
	char		imeiStr[20];//IMEI in string format.
	uint64_t	imeiHex;	//IMEI in hex format.
}gsmModemDetails;
extern gsmModemDetails g_gsmModemDetails_t;

//structure to store SIM and Cloud details.
typedef struct _simCloudDetails
{
	char		apnName1[20];		//APN name for the SIM1.
	char		apnName2[20];		//APN name for the SIM2.
	uint16_t	periTime;			//peri time to send regular data to the server.
	char		timeServerIP[20];	//Timer cloud Ip.
	uint16_t	timeServerPort;		//Timer cloud port.
	char		cloudServerIP[20];	//Cloud Ip.
	uint16_t	cloudServerPort;	//Cloud port.
	char		ftpIP[30];			//FTP Ip.
	char		ftpUserName[30];	//FTP username.
	char		ftpPassword[30];	//FTP password.
}simCloudDetails;
extern simCloudDetails g_simCloudDetails_t;


#ifndef USRINC_RS485PROCESS_H_
#include "rs485Process.h"
#endif
//structure to store global data required for the program.
typedef struct _lcrGlobalData
{
	uint8_t		noOfModbusConfigs;	//No of modbus configurations exists.
	uint8_t 	modBusConfigSlots[NO_OF_MAX_MODBUS_CONFIGURATION]; // Each byte in the array refers to the configuration's existance.
	uint8_t		timeServerSession; //Session ID used for the timer sever connection.
	uint8_t		cloudServerSession; //Session ID used for the cloud connection.
	uint8_t		ftpServerSession; //Session ID used for the ftp connection.
	uint8_t 	simSignalStrngth; //SIM Signal strength.
	uint16_t 	modBusConfigAck; // packet sequence id to acknoledge the modbus configuration received from the cloud.
}lcrGlobalData_t;
extern lcrGlobalData_t g_lcrGlobalData;

//global flags required for the program.
typedef union _lcrGlobalFlags
{
	struct
		{
			unsigned simSelect			:	1; //defines current sim selected.
			unsigned apnName1Exists		:	1; //defines whether apn name for SIM 1 exists in the LCRCONFIG.TXT file
			unsigned apnName2Exists		:	1; //defines whether apn name for SIM 2 exists in the LCRCONFIG.TXT file
			unsigned dateTimeSet		:	1; //Flag to define whether date and time set.
			unsigned lcrPowerOn			:	1; //Flag to define LCR is powered ON. Used to send power up message.
			unsigned tcpBackupDataExists:	1; //Flag to check whether tcp backup data exists.
			unsigned spare05			:	1; // not used.
			unsigned spare06			:	1; // not used.
		};
		uint8_t  flagArray;
}lcrGlobalFlags_t;
extern lcrGlobalFlags_t g_lcrGlobalFlags;


//A structure defines alarms.
typedef union _lcrAlarmStructure
{
	struct
		{
			unsigned errCfg_0		:	1;	//Error in modbus config slot 0 communication.
			unsigned errCfg_1		:	1;	//Error in modbus config slot 1 communication.
			unsigned errCfg_2		:	1;	//Error in modbus config slot 2 communication.
			unsigned errCfg_3		:	1;	//			""
			unsigned errCfg_4		:	1;	//			""
			unsigned errCfg_5		:	1;	//			""
			unsigned errCfg_6		:	1;	//			""
			unsigned errCfg_7		:	1;	//			""
			unsigned errCfg_8		:	1;	//			""
			unsigned errCfg_9		:	1;	//			""
			unsigned errCfg_10		:	1;	//			""
			unsigned errCfg_11		:	1;	//			""
			unsigned errCfg_12		:	1;	//			""
			unsigned errCfg_13		:	1;	//			""
			unsigned errCfg_14		:	1;	//			""
			unsigned errCfg_15		:	1;	//			""
			unsigned errCfg_16		:	1;	//			""
			unsigned errCfg_17		:	1;	//			""
			unsigned errCfg_18		:	1;	//			""
			unsigned errCfg_19		:	1;	//			""
			unsigned errCfg_20		:	1;	//			""
			unsigned errCfg_21		:	1;	//			""
			unsigned errCfg_22		:	1;	//			""
			unsigned errCfg_23		:	1;	//			""
			unsigned errCfg_24		:	1;	//			""
			unsigned errCfg_25		:	1;	//			""
			unsigned errCfg_26		:	1;	//			""
			unsigned errCfg_27		:	1;	//			""
			unsigned errCfg_28		:	1;	//			""
			unsigned errCfg_29		:	1;	//			""
			unsigned errCfg_30		:	1;	//			""
			unsigned errCfg_31		:	1;	//Error in modbus config slot 31 communication.
			unsigned spare27		:	1;
			unsigned spare28		:	1;
			unsigned spare29		:	1;
			unsigned spare30		:	1;
			unsigned spare31		:	1;
			unsigned spare32		:	1;
			unsigned spare33		:	1;
			unsigned spare34		:	1;
			unsigned spare35		:	1;
			unsigned spare36		:	1;
			unsigned spare37		:	1;
			unsigned spare38		:	1;
			unsigned spare39		:	1;
			unsigned spare40		:	1;
			unsigned errGprs		:	1;	//GPRS error.
			unsigned errCloud		:	1;	//SERVER error.
		};
		uint8_t  alarmArray[6];
}lcrAlarmStructure;
extern lcrAlarmStructure g_lcrAlarmStructure;

#endif /* USRINC_GLOBAL_H_ */
