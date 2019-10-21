/*
 * sdCard.h
 *
 *  Created on: Jun 30, 2019
 *      Author: Arjun Dongre
 */

#ifndef USRINC_SDCARD_H_
#define USRINC_SDCARD_H_


//SD card functioning specific macros.
#define SD_CARD_SCRATCHPAD_SIZE		512
#define SD_CARD_MOUNT_IMMEDIATELY	1
#define SD_CARD_UNMOUNT				0


#define SD_CARD_DETECT_PIN			GPIOD_PIN3 //SD card detection pin.
#define SD_CARD_DETECT_PIN_PORT		GPIOD		// port name for SD card detection pin.

//Enum define type of data edited/added to the LCRCFG.TXT file.
typedef enum {
				CONFIG_FILE_EDIT_INT,
				CONFIG_FILE_EDIT_STRING,
				CONFIG_FILE_EDIT_ARRAY
}lcrConfigFileEditType;

//Lcr Config JSON objects
#define	LCR_CONFIG_VERSION			"VERSION"
#define	MODBUS_CFG_NO				"MODBUS_CFG_"
#define APN_NAME_1					"APN_NAME_1"
#define APN_NAME_2					"APN_NAME_2"
#define CLOUD_IP					"CLOUD_IP"
#define CLOUD_PORT					"CLOUD_PORT"
#define TIME_IP						"TIME_IP"
#define TIME_PORT					"TIME_PORT"
#define FTP_IP						"FTP_IP"
#define FTP_USERNAME				"FTP_USERNAME"
#define FTP_PASSWORD				"FTP_PASSWORD"
#define PERI_TIME					"PERI_TIME"

//Default Parameters
#define DEFAULT_CLOUD_IP			"3.248.24.117"
#define DEFAULT_CLOUD_PORT			5005
#define DEFAULT_TIME_IP				"bsl.connectm.com"
#define DEFAULT_TIME_PORT			13
#define DEFAULT_FTP_IP				"54.154.127.11"
#define DEFAULT_FTP_USERNAME		"connectm"
#define DEFAULT_FTP_PASSWORD		"ConnectM@FTP"
#define DEFAULT_PERI_TIME			15


//File name where tcp backup data is stored.
#define TCP_DATA_BACKUP_FILE_NAME	"TCP.TXT"

//Last cursor position in TCP.TXT file
#define TCP_DATA_BACKUP_CURSOR_POS_FILE_NAME "TCPCUR.TXT"
#define TCP_DATA_BACKUP_CURSOR_JSON			"CursorPos"


//TCP data backup thread's prioroty.
#define ST_AND_FW_HANDLE_PRIORITY	3
//TCP data backup thread's stack size.
#define ST_AND_FW_HANDLE_STACK_SIZE 10*1024
//Size of buffer to store event.
#define ST_AND_FW_QUEUE_SIZE		5

//Event to intimate store tcp data.
#define ST_AND_FW_EVENT				1

//Globals.
extern mailbox_t 	g_mailBoxStoreAndForward;
extern uint8_t		g_storeAndForwardEvent;
extern uint32_t 		g_cursorPosInTcpBackupFile;

bool 	 usfn_SdCardInitialization(void);
uint32_t usfn_SdCardCheckFileExistance(char*);
bool 	 usfn_SdCardReadFile(char*, uint8_t*, uint32_t);
void	 usfn_SdCardEditLcrCfgFile(char*, void*, uint16_t ,lcrConfigFileEditType);
bool 	 usfn_SdCardDeleteFile(char*);
bool 	 usfn_SdCardWriteFile(char*, char*);
#ifndef USRINC_TCPDATAPROCESS_H_
#include "tcpDataProcess.h"
#endif
bool 	 usfn_SdCardStoreTcpData(tcpData_t*, uint16_t);
void 	 storeAndForwardHandle(void *p);
bool usfn_getAPacketFromTheTcpBackupFile(tcpData_t*, uint16_t*);


#endif /* USRINC_SDCARD_H_ */
