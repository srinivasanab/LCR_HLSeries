/*
 * boardInitialization.c
 *
 *  Created on: Jun 29, 2019
 *      Author: Arjun Dongre
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "string.h"
#include "stdlib.h"
#include "stdio.h"

#include "cJSON.h"
#include "digitalIO.h"
#include "global.h"
#include "initialization.h"
#include "lcrConfig.h"
#include "modem.h"
#include "rs485Communication.h"
#include "rs485Process.h"
#include "sdCard.h"

static bool usfn_ExtractLcrConfig(cJSON *lcrConfigJson);

/*------------------------------------------------------------------------
*Function 	: 	usfn_BoardInitialization
*Description: 	Initializes SD Card, checks modem communication and
				configures the modem.
*Arguments	: 	None
*Return	 	: 	FALSE 	- On Success
				TRUE	- On error
*------------------------------------------------------------------------*/
bool usfn_BoardInitialization(void)
{
	// Initialize SD card.
	if(FALSE == usfn_SdCardInitialization())
	{
		usfn_TurnONModem();
		// Check Modem communication.
		if(FALSE == usfn_CheckModemCommunication())
		{
			// Configure the modem.
			if(FALSE == usfn_ConfigModemSettings())
			{
				// Retrieve modem details.
				if(FALSE == usfn_GetModemDetails())
				{
					return FALSE;
				}
				chprintf(g_chp,"usfn_BoardInitialization : ERROR - usfn_GetModemDetails.\r\n");
				return TRUE;
			}
			chprintf(g_chp,"usfn_BoardInitialization : ERROR - usfn_ConfigModemSettings.\r\n");
			return TRUE;
		}
			chprintf(g_chp,"usfn_BoardInitialization : ERROR - usfn_CheckModemCommunication.\r\n");
			return TRUE;
	}
	chprintf(g_chp,"usfn_BoardInitialization : ERROR - usfn_SdCardInitialization.\r\n");
	return TRUE;
}

/*------------------------------------------------------------------------
*Function 	: 	usfn_AppInitialization
*Description: 	Reads configuration present in the SD Card and configures
				parameters required for the application.
*Arguments	: 	None
*Return	 	: 	FALSE 	- On Success
				TRUE	- On error
*------------------------------------------------------------------------*/
bool usfn_AppInitialization(void)
{
	uint32_t fileSize	= 0;	// LCRCFG.TXT file size.
	char*	 fileData 	= NULL;	// Pointer to store the config file.
	bool ret = TRUE;
	//Check config file existence in the SD card
	fileSize = usfn_SdCardCheckFileExistance(LCR_CONFIG_FILE_NAME);
	if(fileSize)
	{
		chprintf(g_chp,"usfn_AppInitialization : Configuration exists in the SD Card. File size -> %u.\r\n", fileSize);
		//Allocate dynamic memory to store config file.
		fileData = (char*)chHeapAlloc(NULL,(size_t)fileSize+1);
		if(fileData)
		{	//Read config file from SD card
			if(!usfn_SdCardReadFile(LCR_CONFIG_FILE_NAME,(uint8_t*)fileData,fileSize))
			{
				chprintf(g_chp,"\r\n%s\r\n",fileData);
				//Parse the config file
				cJSON *lcrConfigJson = cJSON_Parse((char*)fileData);
				if(lcrConfigJson)
				{
					ret = TRUE;
					// Read all the parameters from the jSon.
					ret = usfn_ExtractLcrConfig(lcrConfigJson);
					// Delete the jSon.
					cJSON_Delete(lcrConfigJson);
					if(ret)
						chprintf(g_chp,"usfn_AppInitialization : ERROR - Error in the LCRCFG.TXT file.\r\n");
				}
				else
				{
					chprintf(g_chp,"usfn_AppInitialization : ERROR - Couldn't parse the JSON file.\r\n");
					return TRUE;
				}
			}
			else
			{
				chprintf(g_chp,"usfn_AppInitialization : ERROR - Couldn't read the file.\r\n");
				return TRUE;
				chHeapFree((void*)fileData); //Freeing memory.
			}
		}
		else
		{
			chprintf(g_chp,"usfn_AppInitialization : ERROR - Couldn't create the malloc.\r\n");
			return TRUE;
		}
	}
	else
		chprintf(g_chp,"usfn_AppInitialization : ERROR - Couldn't find the LCRCFG.TXT file or the file is empty.\r\n");

	fileSize = usfn_SdCardCheckFileExistance(TCP_DATA_BACKUP_CURSOR_POS_FILE_NAME);
	if(fileSize)
	{
		char cursorPosData[50]="";
		if(!usfn_SdCardReadFile(TCP_DATA_BACKUP_CURSOR_POS_FILE_NAME,(uint8_t*)cursorPosData,fileSize))
		{
			cJSON *extractJson =NULL;
			cJSON *paramJson =NULL;
			extractJson = cJSON_Parse((char*)cursorPosData);
			if(extractJson)
			{
				paramJson = cJSON_GetObjectItemCaseSensitive(extractJson, TCP_DATA_BACKUP_CURSOR_JSON);
				if (cJSON_IsNumber(paramJson))
				{
					g_cursorPosInTcpBackupFile = paramJson->valueint;
					chprintf(g_chp,"usfn_AppInitialization : Cursor position in the TCP file -> %u.\r\n", g_cursorPosInTcpBackupFile);
					cJSON_Delete(extractJson);
					return FALSE;
				}
				else
					chprintf(g_chp,"usfn_AppInitialization : ERROR - Esisting parameter of different type.\r\n");

				cJSON_Delete(extractJson);
				return TRUE;
			}
			else
				chprintf(g_chp,"usfn_AppInitialization : ERROR - Couldn't extract the json for the cursor position.\r\n");
	}
		else
			chprintf(g_chp,"usfn_AppInitialization : ERROR - Couldn't read the  \"TCPCURSOR.TXT\" file.\r\n");
	}
	else
		chprintf(g_chp,"usfn_AppInitialization : ERROR - Couldn't find the  \"TCPCURSOR.TXT\" file, or this file is empty.\r\n");

	return FALSE;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_ExtractLcrConfig
*Description: 	Extracts each parameters present in the configuration file.
*Arguments	: 	*lcrConfigJson - Extracted Json from the LCRCFG.TXT file.
*Return	 	: 	FALSE 	- On Success
				TRUE	- On error
*------------------------------------------------------------------------*/
static bool usfn_ExtractLcrConfig(cJSON *lcrConfigJson)
{
	cJSON *extractJson =NULL;
	//Check Lcr configuration file version.
	enum configParams_e	{
							LCR_CFG_VERSION,
							LCR_CFG_APN_NAME_1,
							LCR_CFG_APN_NAME_2,
							LCR_CFG_CLOUD_IP,
							LCR_CFG_CLOUD_PORT,
							LCR_CFG_TIME_IP,
							LCR_CFG_TIME_PORT,
							LCR_CFG_FTP_IP,
							LCR_CFG_FTP_USERNAME,
							LCR_CFG_FTP_PASSWORD,
							LCR_CFG_PERI_TIME,
							LCR_CFG_MODBUS_CFG,
							LCR_CFG_END
						};
	uint8_t	cfgState = LCR_CFG_VERSION;
	for(;;)
	{
		switch(cfgState)
		{
			case LCR_CFG_VERSION	: extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, LCR_CONFIG_VERSION);
									  if (cJSON_IsNumber(extractJson))
									  {
										  // Check whther the version of the config file is correct?
										  if(LCR_CONFIG_FILE_VERSION == extractJson->valueint)
										  {
											  cfgState = LCR_CFG_APN_NAME_1;
											  break;
										  }
										  chprintf(g_chp,"usfn_ExtractLcrConfig : ERROR - While reading LCR Config file version.\r\n");
										  return TRUE;
									  }
									  chprintf(g_chp,"usfn_ExtractLcrConfig : ERROR - While extracting LCR Config file version.\r\n");
									  return TRUE;
									  break;
			case LCR_CFG_APN_NAME_1	: extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, APN_NAME_1);
			  	  	  	  	  	  	  cfgState = LCR_CFG_APN_NAME_2;
									  if (cJSON_IsString(extractJson) && (extractJson->valuestring != NULL))
									  {
										  if(strlen(extractJson->valuestring) > 3)
										  {
											  strcpy(g_simCloudDetails_t.apnName1,extractJson->valuestring);
											  g_lcrGlobalFlags.apnName1Exists = TRUE; // No need to search for operator name to set the APN name.
											  break;
										  }
									  }
									  g_lcrGlobalFlags.apnName1Exists = FALSE;
									  chprintf(g_chp,"usfn_ExtractLcrConfig : WARNING - APN_NAME_1 is absent.\r\n");
									  break;
			case LCR_CFG_APN_NAME_2	: extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, APN_NAME_2);
			  	  	  	  	  	  	  cfgState = LCR_CFG_CLOUD_IP;
									  if (cJSON_IsString(extractJson) && (extractJson->valuestring != NULL))
									  {
										  if(strlen(extractJson->valuestring) > 3)
										  {
											  strcpy(g_simCloudDetails_t.apnName2,extractJson->valuestring);
											  g_lcrGlobalFlags.apnName2Exists = TRUE; // No need to search for operator name to set the APN name.
											  break;
										  }
									  }
									  chprintf(g_chp,"usfn_ExtractLcrConfig : WARNING - APN_NAME_2 is absent.\r\n");
									  break;
			case LCR_CFG_CLOUD_IP	: extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, CLOUD_IP);
			  	  	  	  	  	  	  cfgState = LCR_CFG_CLOUD_PORT;
									  if (cJSON_IsString(extractJson) && (extractJson->valuestring != NULL))
									  {
										  strcpy(g_simCloudDetails_t.cloudServerIP,extractJson->valuestring);
										  break;
									  }
									  chprintf(g_chp,"usfn_ExtractLcrConfig : WARNING - CLOUD_IP is absent. Setting the default IP.\r\n");
									  strcpy(g_simCloudDetails_t.cloudServerIP,DEFAULT_CLOUD_IP);
									  break;
			case LCR_CFG_CLOUD_PORT	: extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, CLOUD_PORT);
			  	  	  	  	  	  	  cfgState = LCR_CFG_TIME_IP;
			  	  	  	  	  	  	  if (cJSON_IsNumber(extractJson))
			  	  	  	  	  	  	  {
			  	  	  	  	  	  		  g_simCloudDetails_t.cloudServerPort = (uint16_t)extractJson->valueint;
										  break;
									  }
									  chprintf(g_chp,"usfn_ExtractLcrConfig : WARNING - CLOUD_PORT is absent. Setting the default PORT.\r\n");
									  g_simCloudDetails_t.cloudServerPort = DEFAULT_CLOUD_PORT;
									  break;
			case LCR_CFG_TIME_IP	: extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, TIME_IP);
			  	  	  	  	  	  	  cfgState = LCR_CFG_TIME_PORT;
									  if (cJSON_IsString(extractJson) && (extractJson->valuestring != NULL))
									  {
										  strcpy(g_simCloudDetails_t.timeServerIP,extractJson->valuestring);
										  break;
									  }
									  chprintf(g_chp,"usfn_ExtractLcrConfig : WARNING - CLOUD_IP is absent. Setting the default IP.\r\n");
									  strcpy(g_simCloudDetails_t.timeServerIP,DEFAULT_TIME_IP);
									  break;
			case LCR_CFG_TIME_PORT	: extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, TIME_PORT);
			  	  	  	  	  	  	  cfgState = LCR_CFG_FTP_IP;
			  	  	  	  	  	  	  if (cJSON_IsNumber(extractJson))
			  	  	  	  	  	  	  {
			  	  	  	  	  	  		  g_simCloudDetails_t.timeServerPort = (uint16_t)extractJson->valueint;
										  break;
									  }
									  chprintf(g_chp,"usfn_ExtractLcrConfig : WARNING - CLOUD_PORT is absent. Setting the default PORT.\r\n");
									  g_simCloudDetails_t.timeServerPort = DEFAULT_CLOUD_PORT;
									  break;
			case LCR_CFG_FTP_IP	: extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, FTP_IP);
			  	  	  	  	  	  	  cfgState = LCR_CFG_FTP_USERNAME;
									  if (cJSON_IsString(extractJson) && (extractJson->valuestring != NULL))
									  {
										  strcpy(g_simCloudDetails_t.ftpIP,extractJson->valuestring);
										  break;
									  }
									  chprintf(g_chp,"usfn_ExtractLcrConfig : WARNING - FTP_IP is absent. Setting the default IP.\r\n");
									  strcpy(g_simCloudDetails_t.ftpIP,DEFAULT_FTP_IP);
									  break;
			case LCR_CFG_FTP_USERNAME: extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, FTP_USERNAME);
			  	  	  	  	  	  	  cfgState = LCR_CFG_FTP_PASSWORD;
									  if (cJSON_IsString(extractJson) && (extractJson->valuestring != NULL))
									  {
										  strcpy(g_simCloudDetails_t.ftpUserName,extractJson->valuestring);
										  break;
									  }
									  chprintf(g_chp,"usfn_ExtractLcrConfig : WARNING - FTP_USERNAME is absent. Setting the default IP.\r\n");
									  strcpy(g_simCloudDetails_t.ftpUserName,DEFAULT_FTP_USERNAME);
									  break;
			case LCR_CFG_FTP_PASSWORD: extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, FTP_PASSWORD);
			  	  	  	  	  	  	  cfgState = LCR_CFG_PERI_TIME;
									  if (cJSON_IsString(extractJson) && (extractJson->valuestring != NULL))
									  {
										  strcpy(g_simCloudDetails_t.ftpPassword,extractJson->valuestring);
										  break;
									  }
									  chprintf(g_chp,"usfn_ExtractLcrConfig : WARNING - FTP_PASSWORD is absent. Setting the default IP.\r\n");
									  strcpy(g_simCloudDetails_t.ftpPassword,DEFAULT_FTP_PASSWORD);
									  break;
			case LCR_CFG_PERI_TIME	: extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, PERI_TIME);
			  	  	  	  	  	  	  cfgState = LCR_CFG_MODBUS_CFG;
			  	  	  	  	  	  	  if (cJSON_IsNumber(extractJson))
			  	  	  	  	  	  	  {
			  	  	  	  	  	  		  g_simCloudDetails_t.periTime = (uint16_t)extractJson->valueint;
										  break;
									  }
									  chprintf(g_chp,"usfn_ExtractLcrConfig : WARNING - PERI_TIME is absent. Setting the default PORT.\r\n");
									  g_simCloudDetails_t.periTime = DEFAULT_PERI_TIME;
									  break;
			case LCR_CFG_MODBUS_CFG	: cfgState = LCR_CFG_END;
									  //Reads modbus configurations and stores configuration in the global structure.
									  usfn_SetupModbusRegisters(lcrConfigJson);
									  break;

		}
		if(LCR_CFG_END == cfgState)
			break;
	}
	return FALSE;
}
