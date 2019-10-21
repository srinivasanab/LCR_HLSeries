/*
 * sdCard.c
 *
 *  Created on: Jun 30, 2019
 *      Author: Arjun Dongre
 */
#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "string.h"
#include "stdio.h"
#include "ff.h"

#include "cJSON.h"
#include "crc16.h"
#include "global.h"
#include "sdCard.h"
#include "lcrConfig.h"

static 	FATFS 	g_SdCardHandle;
mailbox_t 		g_mailBoxStoreAndForward;
uint8_t			g_storeAndForwardEvent = 0;
static msg_t 	g_StoreAndForwardMsqQueue[ST_AND_FW_QUEUE_SIZE]; //Array to store received messages(LCR events).
uint32_t 		g_cursorPosInTcpBackupFile = 0;

/*------------------------------------------------------------------------
*Function 	: 	usfn_SdCardInitialization.
*Description: 	Initializes and connects the SD card.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
bool usfn_SdCardInitialization(void)
{
	chprintf(g_chp,"usfn_SdCardInitialization : Initializing SD Card.\r\n");
	uint8_t sd_scratchpad[SD_CARD_SCRATCHPAD_SIZE];
	SDCConfig sdccfg = {
						  sd_scratchpad,
						  SDC_MODE_4BIT
						};
	char *mode[] = {"SDV11", "SDV20", "MMC", NULL};
	sdcStart(&SDCD1, &sdccfg);
	chThdSleepSeconds(2);

	//Detect the SD Card.
	if (blkIsInserted(&SDCD1))
	{
		chprintf(g_chp, "usfn_SdCardInitialization : ERROR - blkIsInserted; SD Card not found.\r\n");
		return TRUE;
	}
	//Connect to SD card.
	chprintf(g_chp, "usfn_SdCardInitialization : SD Card Exists..Connecting..\r\n");
	if (sdcConnect(&SDCD1))
	{
		chprintf(g_chp, "usfn_SdCardInitialization : ERROR - sdcConnect; Cannot connect to SD Card..\r\n");
		return TRUE;
	}

	chprintf(g_chp, "usfn_SdCardInitialization : SD_Card Connected. Showing the info.\r\n");
	chprintf(g_chp, "_______________________________________________\r\n");
	chprintf(g_chp, "CSD      : %08X %8X %08X %08X \r\n", SDCD1.csd[3], SDCD1.csd[2], SDCD1.csd[1], SDCD1.csd[0]);
	chprintf(g_chp, "CID      : %08X %8X %08X %08X \r\n",	SDCD1.cid[3], SDCD1.cid[2], SDCD1.cid[1], SDCD1.cid[0]);
	chprintf(g_chp, "Mode     : %s\r\n", mode[SDCD1.cardmode & 3U]);
	chprintf(g_chp, "Capacity : %DMB\r\n", SDCD1.capacity / 2048);
	chprintf(g_chp, "_______________________________________________\r\n");


	//Mount
	FRESULT err;
	err = f_mount(&g_SdCardHandle, "/", SD_CARD_MOUNT_IMMEDIATELY);
	if (err != FR_OK)
	{
		chprintf(g_chp, "usfn_SdCardInitialization : ERROR - f_mount(). Is the SD card inserted?\r\n");
		return TRUE;
	}
	chprintf(g_chp, "usfn_SdCardInitializationFS : SUCCESSFUL - f_mount().\r\n");
	return FALSE;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_SdCardCheckFileExistance.
*Description: 	Checks whether file exists in the SD card.
*Arguments	: 	*fileName	- /Path/fileName.ext path and name of the file.
*Return	 	: 	uint32_t	- Size of file in bytes if exists.
				0			- If file doesn't exists.
*------------------------------------------------------------------------*/
uint32_t usfn_SdCardCheckFileExistance(char* fileName)
{
	FILINFO fileInfo = {0};
	if(FR_OK == f_stat(fileName, &fileInfo))
	{
		return fileInfo.fsize;
	}
	return FALSE;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_SdCardReadFile.
*Description: 	Reads file.
*Arguments	: 	*fileName	- /Path/fileName.ext path and name of the file.
				buf			-  Pointer to buffer to store data.
				len			-  No of bytes to be read from the file.
*Return	 	: 	bool		-  FALSE - On success.
							-  TRUE  - On error.
*------------------------------------------------------------------------*/
bool usfn_SdCardReadFile(char *fileName, uint8_t *buf, uint32_t len)
{
	FIL fileHandle		={0};
	UINT noOfBytesRead 	= 0;
	bool ret			= TRUE;
	if(FR_OK == f_open(&fileHandle, fileName, FA_READ))
	{
		if(FR_OK == f_read (
							&fileHandle,    /* [IN] File object */
							buf,  			/* [OUT] Buffer to store read data */
							len,    		/* [IN] Number of bytes to read */
							&noOfBytesRead  /* [OUT] Number of bytes read */
				))
		{
			if(noOfBytesRead == len)
			{
				buf[len]	= 0;
				ret 		= FALSE;
			}
		}
		else
			ret = TRUE;
		f_close(&fileHandle);
	}
	else
		ret = TRUE;
	return ret;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_SdCardWriteFile.
*Description: 	Wites a string to the file.
*Arguments	: 	*fileName	- /Path/fileName.ext path and name of the file.
				*data		- Pointer to string.
*Return	 	: 	bool		-  FALSE - On success.
							-  TRUE  - On error.
*------------------------------------------------------------------------*/
bool usfn_SdCardWriteFile(char *fileName, char *data)
{
	FIL fileHandle		={0};
	UINT noOfBytesRead 	= 0;
	bool ret			= TRUE;
	if(FR_OK == f_open(&fileHandle, fileName, FA_OPEN_APPEND | FA_WRITE))
	{
		if(FR_OK == f_write (
								&fileHandle,    /* [IN] Pointer to the file object structure */
								data, 			/* [IN] Pointer to the data to be written */
								strlen(data),	/* [IN] Number of bytes to write */
								&noOfBytesRead  /* [OUT] Pointer to the variable to return number of bytes written */
							))
		{
			f_close(&fileHandle);
			ret = FALSE;
		}
	}

	return ret;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_SdCardDeleteFile.
*Description: 	Deletes a file.
*Arguments	: 	*fileName	- /Path/fileName.ext path and name of the file.
*Return	 	: 	bool		-  FALSE - On success.
							-  TRUE  - On error.
*------------------------------------------------------------------------*/
bool usfn_SdCardDeleteFile(char* fileName)
{
	if(FR_OK == f_unlink (fileName))
		return FALSE;
	return TRUE;

}


/*------------------------------------------------------------------------
*Function 	: 	usfn_SdCardEditLcrCfgFile.
*Description: 	Edits or adds new configuration to the LCRCFG.TXT file.
*Arguments	: 	*paramName	- A string pointer to jSOn parameter name.
				*data		- A void pointer to the data to be written to the file.
				dataLen		- Length of array(Comes into picture when array type of
								data is written).
				type		- type of data is written (For types refer lcrConfigFileEditType)
*Return	 	: 	None.
*------------------------------------------------------------------------*/
void usfn_SdCardEditLcrCfgFile(char *paramName, void *data, uint16_t dataLen,lcrConfigFileEditType type)
{
	uint32_t fileSize	= 0;	// LCRCFG.TXT file size.
	char*	 fileData 	= NULL;	// Pointer to store the config file.

	//Check config file existence in the SD card
	fileSize = usfn_SdCardCheckFileExistance(LCR_CONFIG_FILE_NAME);
	if(fileSize)
	{
		//Allocate dynamic memory to store config file.
		fileData = (char*)chHeapAlloc(NULL,(size_t)fileSize+1);
		if(fileData)
		{	//Read config file from SD card
			if(!usfn_SdCardReadFile(LCR_CONFIG_FILE_NAME,(uint8_t*)fileData,fileSize))
			{
				//Parse the config file
				cJSON *lcrConfigJson = cJSON_Parse((char*)fileData);
				if(lcrConfigJson)
				{
					if(paramName)
						cJSON_DeleteItemFromObjectCaseSensitive(lcrConfigJson,paramName);

					if(CONFIG_FILE_EDIT_INT == type)
					{
						cJSON_AddNumberToObject(lcrConfigJson, paramName, *(uint16_t*)data);
					}
					else if(CONFIG_FILE_EDIT_STRING == type)
					{
						cJSON_AddStringToObject(lcrConfigJson, paramName, (char*)data);
					}
					else
					{
						if(data)
							cJSON_AddItemToObject(lcrConfigJson, paramName, cJSON_CreateIntArray((int*)data, dataLen));
						else
						{
							char modBusCfgName[20]	= "";
							uint8_t modBusCfgCount	= 0;
							while(modBusCfgCount < NO_OF_MAX_MODBUS_CONFIGURATION)
							{
								sprintf(modBusCfgName,"%s%u",MODBUS_CFG_NO,modBusCfgCount);
								if(cJSON_GetObjectItemCaseSensitive(lcrConfigJson, modBusCfgName))
									cJSON_DeleteItemFromObjectCaseSensitive(lcrConfigJson,modBusCfgName);
								modBusCfgCount++;
							}
						}
					}

					char *jSonStr=NULL;
					jSonStr = cJSON_Print(lcrConfigJson);
					chprintf(g_chp, "usfn_SdCardEditLcrCfgFile : Updated LCRCFG.TXT file - %s\r\n", jSonStr);
					if(FALSE == usfn_SdCardDeleteFile(LCR_CONFIG_FILE_NAME))
					{
						if(FALSE  == usfn_SdCardWriteFile(LCR_CONFIG_FILE_NAME,jSonStr))
						{
						}
						else
						{
							chprintf(g_chp, "usfn_SdCardEditLcrCfgFile : ERROR - Couldn't update the LCRCFG.TXT file.\r\n");
						}
					}
					else
					{
						chprintf(g_chp, "usfn_SdCardEditLcrCfgFile : ERROR - Couldn't delete the LCRCFG.TXT file.\r\n");
					}
					// Delete the jSon.
					cJSON_Delete(lcrConfigJson);
				}
				else
				{
					chprintf(g_chp,"usfn_AppInitialization : ERROR - Couldn't parse the JSON file.\r\n");
				}
			}
			else
			{
				chprintf(g_chp,"usfn_AppInitialization : ERROR - Couldn't read the file.\r\n");
			}
			chHeapFree((void*)fileData);
		}
		else
		{
			chprintf(g_chp,"usfn_AppInitialization : ERROR - Couldn't create the malloc.\r\n");
		}
	}
	else
	{
		chprintf(g_chp,"usfn_AppInitialization : ERROR - There is no LCRCFG.TXT file.\r\n");
	}
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_SdCardStoreTcpData.
*Description: 	Stores TCP data when device is offline.
*Arguments	: 	*tcpData		- Pointer to the TCP data.
				tcpDataLength	- Length of TCP data.
*Return	 	: 	bool		-  FALSE - On success.
							-  TRUE  - On error.
*------------------------------------------------------------------------*/
bool usfn_SdCardStoreTcpData(tcpData_t *tcpData, uint16_t tcpDataLength)
{
	FIL fileHandle		={0};
	UINT noOfBytesRead 	= 0;
	FRESULT res;
	if(FR_OK ==(res= f_open(&fileHandle, TCP_DATA_BACKUP_FILE_NAME, FA_OPEN_APPEND | FA_WRITE)))
	{
		if(FR_OK == f_write (
								&fileHandle,    /* [IN] Pointer to the file object structure */
								&tcpDataLength, 	/* [IN] Pointer to the data to be written */
								2,				/* [IN] Number of bytes to write */
								&noOfBytesRead  /* [OUT] Pointer to the variable to return number of bytes written */
							))
		{
			if(FR_OK == f_write (
									&fileHandle,    /* [IN] Pointer to the file object structure */
									tcpData, 	/* [IN] Pointer to the data to be written */
									tcpDataLength,	/* [IN] Number of bytes to write */
									&noOfBytesRead  /* [OUT] Pointer to the variable to return number of bytes written */
								))
			{
				f_close(&fileHandle);
				return FALSE;
			}
			chprintf(g_chp,"usfn_SdCardStoreTcpData : ERROR - Couldn't write tcp data.\r\n");
			return TRUE;
		}
		chprintf(g_chp,"usfn_SdCardStoreTcpData : ERROR - Couldn't write tcp data length.\r\n");
		f_close(&fileHandle);
		return TRUE;
	}
	chprintf(g_chp,"usfn_SdCardStoreTcpData : ERROR - Couldn't open the file. - %u\r\n",res);
	return TRUE;
}


/*------------------------------------------------------------------------
*Function 	: 	storeAndForwardHandle.
*Description: 	Stores data when event is received.
*Arguments	: 	p	- Arguments passed to the thread(Not used).
*Return	 	: 	None.
*------------------------------------------------------------------------*/
THD_FUNCTION(storeAndForwardHandle, p)
{
	chRegSetThreadName("STORE_AND_FORWARD_THREAD");
	(void)p;

	uint8_t *event = NULL;
	tcpData_t tcpData={0};
	uint16_t tcpDataLength 		= 0;
	chMBObjectInit(&g_mailBoxStoreAndForward, g_StoreAndForwardMsqQueue, ST_AND_FW_QUEUE_SIZE);
	while (TRUE)
	{
		chMBFetchTimeout(&g_mailBoxStoreAndForward, (msg_t *)&event, TIME_INFINITE);
		chprintf(g_chp,"g_storeAndForwardTimerCallBack : Backing up TCP data.\r\n");
		tcpData.packetType 	= TCP_DATA_PCKET_TYPE_REGULAR;
		tcpData.deviceVersionID = TCP_DATA_DEVICE_VERSION_ID;
#if LCR_CONF_TCP_DATA_IN_SINGLE_FRAME
		usfn_GetInitializedPacket(&tcpData);
		usfn_CollectRegularDataInSingleFrame(&tcpData);
		tcpDataLength = usfn_CompleteTheTcpPacket(&tcpData);
		if(usfn_SdCardStoreTcpData(&tcpData, tcpDataLength))
			chprintf(g_chp,"g_storeAndForwardTimerCallBack : ERROR - While backing up the tcp data.\r\n");
		else
		{
			g_lcrGlobalFlags.tcpBackupDataExists = TRUE;
			chprintf(g_chp,"g_storeAndForwardTimerCallBack : Successfully backed up the tcp data.\r\n");
		}
#else
		uint8_t		modBusConfigIdx = 0;
		while(modBusConfigIdx < g_lcrGlobalData.noOfModbusConfigs)
		{
			if(0 == g_modbusConfigMap[modBusConfigIdx].modBusErrorCount)
			{
				usfn_GetInitializedPacket(&tcpData);
				usfn_CollectRegularDataInMultiFrame(&tcpData,modBusConfigIdx);
				tcpDataLength = usfn_CompleteTheTcpPacket(&tcpData);
				if(usfn_SdCardStoreTcpData(&tcpData, tcpDataLength))
					chprintf(g_chp,"g_storeAndForwardTimerCallBack : ERROR - While backing up the tcp data.\r\n");
				else
				{
					g_lcrGlobalFlags.tcpBackupDataExists = TRUE;
					chprintf(g_chp,"g_storeAndForwardTimerCallBack : Successfully backed up the tcp data. Device ID -> %X, File Size-> %u bytes\r\n", tcpData.deviceID, usfn_SdCardCheckFileExistance(TCP_DATA_BACKUP_FILE_NAME));
				}
			}
			modBusConfigIdx++;
		}
#if LCR_CONF_DIGITAL_IO_BOARD_CONNECTED
		usfn_GetInitializedPacket(&tcpData);
		tcpData->packetType = TCP_DATA_PCKET_TYPE_FAULT;
		tcpData->deviceID	= TCP_DATA_DEVICE_ID_DIGITAL_IO;
		tcpData->frameID	= TCP_DATA_FRAME_ID;
		tcpData->noOfBytes	= (uint16_t)TCP_DATA_LEN_DIGITAL_IO;
		memcpy(tcpData->data,&g_dIoAdcData_t,TCP_DATA_LEN_DIGITAL_IO);
		tcpDataLength = usfn_CompleteTheTcpPacket(&tcpData);
		if(usfn_SdCardStoreTcpData(&tcpData, tcpDataLength))
			chprintf(g_chp,"g_storeAndForwardTimerCallBack : ERROR - While backing up the tcp data.\r\n");
		else
		{
			g_lcrGlobalFlags.tcpBackupDataExists = TRUE;
			chprintf(g_chp,"g_storeAndForwardTimerCallBack : Successfully backed up the tcp data.\r\n");
		}
#endif // LCR_CONF_DIGITAL_IO_BOARD_CONNECTED
#endif
		chThdSleepSeconds(1);
	}

}


/*------------------------------------------------------------------------
*Function 	: 	usfn_getAPacketFromTheTcpBackupFile.
*Description: 	Gets A packet from the store data.
*Arguments	: 	*tcpData	- Pointer to the buffer where tcp packet stored.
				*tcpDataLen	- Pointer to store length of data.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
bool usfn_getAPacketFromTheTcpBackupFile(tcpData_t *tcpData, uint16_t *tcpDataLen)
{
	FIL fileHandle		={0};
	UINT noOfBytesRead 	= 0;
	FRESULT res = FR_INVALID_PARAMETER;

	uint16_t tempTcpDataLen = 0;



	if(FR_OK ==(res= f_open(&fileHandle, TCP_DATA_BACKUP_FILE_NAME, FA_READ)))
	{
		if(FR_OK == f_lseek(&fileHandle,g_cursorPosInTcpBackupFile))
		{
			if(FR_OK == f_read (
						&fileHandle,     	/* [IN] File object */
						&tempTcpDataLen,  	/* [OUT] Buffer to store read data */
						2,    				/* [IN] Number of bytes to read */
						&noOfBytesRead   /* [OUT] Number of bytes read */
					))
				{
					if(FR_OK == f_lseek(&fileHandle,g_cursorPosInTcpBackupFile+2))
					{
						if(FR_OK == f_read (
												&fileHandle,     	/* [IN] File object */
												tcpData,  	/* [OUT] Buffer to store read data */
												tempTcpDataLen,    				/* [IN] Number of bytes to read */
												&noOfBytesRead   /* [OUT] Number of bytes read */
											))
											{
												*tcpDataLen = tempTcpDataLen;
												f_close(&fileHandle);
												chprintf(g_chp,"usfn_getAPacketFromTheTcpBackupFile : SUCCESS - Read tcp packet.\r\n");
												return FALSE;
											}
					}
					f_close(&fileHandle);
					chprintf(g_chp,"usfn_getAPacketFromTheTcpBackupFile : ERROR - Couldn't set the tcp packet. - %u\r\n",res);
					return TRUE;
				}
			f_close(&fileHandle);
			chprintf(g_chp,"usfn_getAPacketFromTheTcpBackupFile : ERROR - Couldn't read the tcp packet length. - %u\r\n",res);
			return TRUE;
		}
		f_close(&fileHandle);
		chprintf(g_chp,"usfn_getAPacketFromTheTcpBackupFile : ERROR - Couldn't set the cursor position1. - %u\r\n",res);
		return TRUE;
	}
	f_close(&fileHandle);
	chprintf(g_chp,"usfn_getAPacketFromTheTcpBackupFile : ERROR - Couldn't open the file. - %u\r\n",res);
	return TRUE;
}
