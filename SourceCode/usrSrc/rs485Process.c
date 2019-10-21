#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "cJSON.h"
#include "global.h"
#include "rs485Process.h"
#include "sdCard.h"

modbusConfigMap_t *g_modbusConfigMap;	// Modbus structure to store all modbus related data.


/*------------------------------------------------------------------------
*Function 	: 	usfn_SetupModbusRegisters
*Description: 	Extracts each modbus configs, verifies and stores in structure.
*Arguments	: 	*lcrConfigJson - Extracted Json from the LCRCFG.TXT file.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
void usfn_SetupModbusRegisters(cJSON *lcrConfigJson)
{
	cJSON 	*extractJson			= NULL; // Store extracted Json object.
	cJSON 	*cJsonModBusConfigArray	= NULL; // Store Array type of Json.
	char 	modBusCfgName[20]		= "";	// Store modbus parameter name.
	uint8_t modBusCfgCount			= 0;
	uint8_t tempModBusCfgArray[RS485_MAX_CONFIG_LENGTH] = ""; // Store temporarily extracted array from the Json.
	int 	arraySize				= 0;

	uint16_t tempVar = 0;
	//Verify all the modbus configurations.
	while(modBusCfgCount<NO_OF_MAX_MODBUS_CONFIGURATION)
	{
		memset(modBusCfgName,0x00,sizeof(modBusCfgName));
		sprintf(modBusCfgName,"%s%u",MODBUS_CFG_NO,modBusCfgCount);
		extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, modBusCfgName);
		if(extractJson)	//Check whether jSon extraction done.
		{
			if(cJSON_IsArray(extractJson)) // Check whether extracted jSon is array.
			{
				arraySize = cJSON_GetArraySize(extractJson);
				if((arraySize > RS485_MIN_CONFIG_LENGTH ) && (arraySize < RS485_MAX_CONFIG_LENGTH))
				{
					for(int i = 0; i < arraySize; i++)
					{
						cJsonModBusConfigArray = cJSON_GetArrayItem(extractJson, i);
						tempModBusCfgArray[i]= (uint8_t)cJsonModBusConfigArray->valueint;
						chprintf(g_chp,"%X ",tempModBusCfgArray[i]);
						cJsonModBusConfigArray = NULL;
					}
					chprintf(g_chp,"\r\n");
					// Verify the configuration and get the total bytes read by this configuration.
					tempVar = usfn_VerifyAndGetDetailsOfModBusConfiguration(tempModBusCfgArray, (uint16_t)arraySize);
					if(tempVar)
					{
						chprintf(g_chp, "CONFIG_DATA_LENGTH -> %u.\r\n", tempVar);
						g_lcrGlobalData.noOfModbusConfigs++;
						g_lcrGlobalData.modBusConfigSlots[modBusCfgCount] = 1;
					}
					else
					{
						chprintf(g_chp,"usfn_SetupModbusRegisters1 : ERROR - Invalid configuration for %s.\r\n",modBusCfgName);
					}
				}
				else
				{
					chprintf(g_chp,"usfn_SetupModbusRegisters1 : ERROR - Invalid config size for %s.\r\n", modBusCfgName);
				}
			}
			else
			{
				chprintf(g_chp,"usfn_SetupModbusRegisters1 : ERROR - Extarcted config %s is not an array.\r\n", modBusCfgName);
			}
		}
		else
		{
			chprintf(g_chp,"usfn_SetupModbusRegisters1 : WARNING - Problem with or there is no configuration %s.\r\n", modBusCfgName);
		}
		modBusCfgCount++;
	}
	//Setup the g_modbusConfigMap structure.
	if(g_lcrGlobalData.noOfModbusConfigs)
	{
		modBusCfgCount = 0;
		//Allocate memory for modbus configurations.
		g_modbusConfigMap = (modbusConfigMap_t*)chHeapAlloc(NULL,(sizeof(modbusConfigMap_t)*g_lcrGlobalData.noOfModbusConfigs));
		memset(g_modbusConfigMap,0,(sizeof(modbusConfigMap_t)*g_lcrGlobalData.noOfModbusConfigs));
		if(g_modbusConfigMap)
		{
			uint8_t  modbusConfigMapIndex 	= 0;
			while(modBusCfgCount<NO_OF_MAX_MODBUS_CONFIGURATION)
			{
				if(1 == g_lcrGlobalData.modBusConfigSlots[modBusCfgCount])
				{
					memset(modBusCfgName,0x00,sizeof(modBusCfgName));
					sprintf(modBusCfgName,"%s%u",MODBUS_CFG_NO,modBusCfgCount);
					extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, modBusCfgName);
					if(extractJson)
					{
						if(cJSON_IsArray(extractJson)) // Check whether extracted jSon is array.
						{
							arraySize = cJSON_GetArraySize(extractJson);
							for(int i = 0; i < arraySize; i++)
							{
								cJsonModBusConfigArray = cJSON_GetArrayItem(extractJson, i);
								tempModBusCfgArray[i]	= (uint8_t)cJsonModBusConfigArray->valueint;
								cJsonModBusConfigArray = NULL;
							}
							memcpy(g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd,tempModBusCfgArray,arraySize);
							g_modbusConfigMap[modbusConfigMapIndex].configChannel	   = modBusCfgCount;
							g_modbusConfigMap[modbusConfigMapIndex].modBusCfgCmdLength = arraySize;
							g_modbusConfigMap[modbusConfigMapIndex].dataLength = usfn_VerifyAndGetDetailsOfModBusConfiguration(g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd, (uint16_t)arraySize);
							chprintf(g_chp,"\r\nusfn_SetupModbusRegisters2 : SUCCESS - Set up %s configuration. Length-> %u\r\n", modBusCfgName, g_modbusConfigMap[modbusConfigMapIndex].dataLength);
							modbusConfigMapIndex++;

						}
						else
						{
							chprintf(g_chp,"usfn_SetupModbusRegisters2 : ERROR - Extracted config %s is not an array.\r\n", modBusCfgName);
						}
					}
					else
					{
						chprintf(g_chp,"usfn_SetupModbusRegisters2 : ERROR - While extracting config %s.\r\n", modBusCfgName);
					}
				}
				modBusCfgCount++;
			}
		}
		else
		{
			g_lcrGlobalData.noOfModbusConfigs = 0;
			chprintf(g_chp,"usfn_SetupModbusRegisters2 : ERROR - Couldn't allocate memory for g_modbusConfigMap.\r\n.");
		}
	}
	else
	{
		chprintf(g_chp,"usfn_SetupModbusRegisters2 : WARNING - Modbus configuration doesn't exists.\r\n", modBusCfgName);
	}


#if 0 //For future use.
	//Setup the g_modbusConfigMap structure.
	if(g_lcrGlobalData.noOfModbusConfigs)
	{
		modBusCfgCount = 0;
		//Allocate memory for modbus configurations.
		g_modbusConfigMap = (modbusConfigMap_t*)chHeapAlloc(NULL,(size_t)g_lcrGlobalData.noOfModbusConfigs);
		memset(g_modbusConfigMap,0,(sizeof(modbusConfigMap_t)*g_lcrGlobalData.noOfModbusConfigs));
		if(g_modbusConfigMap)
		{
			uint8_t  modbusConfigMapIndex 	= 0;
			uint16_t modBusDataLength		= 0;
			uint8_t *tempModBusCfgArraytemp 	= NULL;

			while(modBusCfgCount<NO_OF_MAX_MODBUS_CONFIGURATION)
			{
				if(1 == modBusConfigSlots[modBusCfgCount])
				{
					memset(modBusCfgName,0x00,sizeof(modBusCfgName));
					sprintf(modBusCfgName,"%s%u",MODBUS_CFG_NO,modBusCfgCount);
					extractJson = cJSON_GetObjectItemCaseSensitive(lcrConfigJson, modBusCfgName);
					if(extractJson)
					{
						if(cJSON_IsArray(extractJson)) // Check whether extracted jSon is array.
						{
							arraySize = cJSON_GetArraySize(extractJson);
							chprintf(g_chp, "\r\n_________\r\n ArraySize -> %u\r\n_________\r\n", arraySize);
							tempModBusCfgArray = (uint8_t*)chHeapAlloc(NULL,(size_t)arraySize);
							if(tempModBusCfgArray)
							{
								for(int i = 0; i < arraySize; i++)
								{
									cJsonModBusConfigArray = cJSON_GetArrayItem(extractJson, i);
									tempModBusCfgArray[i]= cJsonModBusConfigArray->valueint;
									chprintf(g_chp,"%X ",tempModBusCfgArray[i]);
									cJsonModBusConfigArray = NULL;
								}
								modBusDataLength = usfn_VerifyAndGetDetailsOfModBusConfiguration(tempModBusCfgArray, arraySize);
								g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd = (uint8_t*)chHeapAlloc(NULL,(size_t)arraySize);
								if(g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd)
								{
									memcpy(g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd, tempModBusCfgArray, arraySize);
									g_modbusConfigMap[modbusConfigMapIndex].modBusData = (uint8_t*)chHeapAlloc(NULL,(size_t)modBusDataLength);
									if(g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd)
									{
										chprintf(g_chp,"usfn_SetupModbusRegisters : SUCCESS - Created %s configuration.\r\n",modBusCfgName);
									}
									else
									{
										chprintf(g_chp,"usfn_SetupModbusRegisters : ERROR - Couldn't allocate DATA memory for %s.\r\n",modBusCfgName);
										chHeapFree((void*)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd);
									}
								}
								else
								{
									chprintf(g_chp,"usfn_SetupModbusRegisters : ERROR - Couldn't allocate CMD memory for %s.\r\n",modBusCfgName);
								}
								chHeapFree((void*)tempModBusCfgArray);
								modbusConfigMapIndex++;
							}
							else
							{
								chprintf(g_chp,"usfn_SetupModbusRegisters : ERROR - While allocating memory to tempModBusCfgArray.\r\n");
							}
						}
						else
						{
							chprintf(g_chp,"usfn_SetupModbusRegisters : ERROR - Extarcted config %s is not an array.\r\n", modBusCfgName);
						}
					}
					else
					{

					}
				}
				modBusCfgCount++;
			}
		}
		else
		{
			g_lcrGlobalData.noOfModbusConfigs = 0;
			chprintf(g_chp,"usfn_SetupModbusRegisters : ERROR - Couldn't allocate memory for g_modbusConfigMap.\r\n.");
		}
	}
	else
	{
		chprintf(g_chp,"usfn_SetupModbusRegisters : WARNING - Modbus configuration doesn't exists.\r\n", modBusCfgName);
	}
#endif // For future use end.
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_VerifyAndGetDetailsOfModBusConfiguration.
*Description: 	Verifies the input modbus configuration.
*Arguments	: 	*mbConfig 		- Pointer to the modbus confiuration.
				mbConfigLength	- Length of modbus configuration.
*Return	 	: 	uint16_t		- Total length of bytes received after polling all the
	 						  	  modbus registers.
	 			0				- Invalid configuration.
*------------------------------------------------------------------------*/
uint16_t usfn_VerifyAndGetDetailsOfModBusConfiguration(uint8_t *mbConfig, uint16_t mbConfigLength)
{
	uint8_t  configArrayIndex = 4, noOfQueries, noOfRegs, noOfBytesPerQuery;
	uint16_t modBusDataLength = 0, queryCount=0;
	for(int j; j<mbConfigLength; j++)
	{
		switch(mbConfig[configArrayIndex])
		{
			case FUNCTION_CODE_COIL_STATUS :	configArrayIndex++;
												noOfQueries			= mbConfig[configArrayIndex];
												queryCount 			= queryCount+noOfQueries;
												configArrayIndex	= configArrayIndex+3;
												for(int i=noOfQueries;i>0;i--)
												{
													noOfRegs			= mbConfig[configArrayIndex];
													noOfBytesPerQuery	= noOfRegs/8;
													if(noOfRegs % 8)
													{
														noOfBytesPerQuery++;
													}
													modBusDataLength 	= modBusDataLength + noOfBytesPerQuery;
													configArrayIndex	= configArrayIndex+3;
												}
												configArrayIndex	= configArrayIndex-2;
												break;
			case FUNCTION_CODE_INPUT_STATUS : 	configArrayIndex++;
												noOfQueries		= mbConfig[configArrayIndex];
												queryCount = queryCount+noOfQueries;
												configArrayIndex	= configArrayIndex+3;
												for(int i=noOfQueries;i>0;i--)
												{
													noOfRegs			= mbConfig[configArrayIndex];
													noOfBytesPerQuery	= noOfRegs/8;
													if(noOfRegs % 8)
													{
														noOfBytesPerQuery++;
													}
													modBusDataLength = modBusDataLength + noOfBytesPerQuery;
													configArrayIndex	= configArrayIndex+3;
												}
												configArrayIndex	= configArrayIndex-2;
												break;
			case FUNCTION_CODE_HOLDING_REG : 	configArrayIndex++;
												noOfQueries		= mbConfig[configArrayIndex];
												queryCount 		= queryCount+noOfQueries;
												configArrayIndex	= configArrayIndex+3;

												for(int i=noOfQueries;i>0;i--)
												{
													noOfRegs			= mbConfig[configArrayIndex];
													noOfBytesPerQuery	= noOfRegs*2;
													modBusDataLength = modBusDataLength + noOfBytesPerQuery;
													configArrayIndex	= configArrayIndex+3;
												}
												configArrayIndex	= configArrayIndex-2;
												break;
			case FUNCTION_CODE_INPUT_REG   : 	configArrayIndex++;
												noOfQueries		= mbConfig[configArrayIndex];
												queryCount 		= queryCount+noOfQueries;
												configArrayIndex	= configArrayIndex+3;

												for(int i=noOfQueries;i>0;i--)
												{
													noOfRegs			= mbConfig[configArrayIndex];
													noOfBytesPerQuery	= noOfRegs*2;
													modBusDataLength = modBusDataLength + noOfBytesPerQuery;
													configArrayIndex	= configArrayIndex+3;
												}
												configArrayIndex	= configArrayIndex-2;
												break;
		}
		if(mbConfig[configArrayIndex] == RS485_CONFIG_NEXT_FUNCTION_REG)
		{
			configArrayIndex++;
		}
		else if(mbConfig[configArrayIndex] == RS485_CONFIG_NEXT_SLAVE)
		{
			configArrayIndex = configArrayIndex+4;
		}
		else if(mbConfig[configArrayIndex] == RS485_CONFIG_END)
		{
			if(RS485_MAX_DATA_LENGTH < modBusDataLength)
			{
//				chprintf(g_chp,"Verification return invalid length\r\n");
				return 0;
			}
			else
			{
//				chprintf(g_chp,"Verification return success\r\n");
				return modBusDataLength;
			}
		}
		else
		{
//			chprintf(g_chp,"Verification return invalid byte\r\n");
			return 0;
		}
	}
//	chprintf(g_chp,"Verification iterated all the bytes.\r\n");
	return 0;
}

