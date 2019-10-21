/*
 * rs485Communication.c
 *
 *  Created on: Jul 6, 2019
 *      Author: Arjun Dongre
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "string.h"

#include "crc16.h"
#include "digitalIO.h"
#include "global.h"
#include "lcrConfig.h"
#include "rs485Communication.h"
#include "rs485Process.h"

mutex_t	g_mutex_rs485DataLock = {0};

static uint8_t 	g_modBusDataStoreBuffer[RS485_MAX_DATA_LENGTH] = "";
static uint16_t	g_modBusDataStoreBufferIndex = 0;

static void startModBusComm(uint8_t);
static void usfn_ChangeRs485BaudRate(uint8_t);
static void usfn_SendModBusQuery(uint8_t*, uint8_t);
static uint64_t usfn_pow(uint8_t);



/*------------------------------------------------------------------------
*Function 	: 	rs485CommunicationHandle.
*Description: 	Thread for rs485 communication.
*Arguments	: 	p	-	Arguments to the function(Not used).
*Return	 	: 	None.
*------------------------------------------------------------------------*/
THD_FUNCTION(rs485CommunicationHandle, p)
{
	chRegSetThreadName("RS485_COMMUNICATION_THREAD");
	(void)p;
	chMtxObjectInit(&g_mutex_rs485DataLock);
	uint8_t modBudConfigSelect 	= 0;
	while (TRUE)
	{
		chMtxLock(&g_mutex_rs485DataLock);
		startModBusComm(modBudConfigSelect);
	    chMtxUnlock(&g_mutex_rs485DataLock);
		modBudConfigSelect++;
		if(modBudConfigSelect >= g_lcrGlobalData.noOfModbusConfigs)
			modBudConfigSelect = 0;
#if LCR_CONF_RS485_DEBUG_ENABLE
		chprintf(g_chp,"rs485CommunicationHandle : Moving to next configuration\r\n");
#endif
	  chThdSleepMilliseconds(500);
	}

}

/*------------------------------------------------------------------------
*Function 	: 	startModBusComm.
*Description: 	Starts modbus communication for particular configuration
				present in the modbus config structure.
*Arguments	: 	modbusConfigMapIndex	- Slot number.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void startModBusComm(uint8_t modbusConfigMapIndex)
{
	uint8_t 	configCmdIndex 	= 1;
	uint8_t		noOfQueries 	= 0;
	uint8_t		rs485BaudRate 	= 0;
	uint8_t		rs485SlaveID 	= 0;
	uint8_t		modBusQuery[MODBUS_QUERY_SIZE] = "";

#if LCR_CONF_RS485_DEBUG_ENABLE
		chprintf(g_chp,"startModBusComm : Entry.\r\n");
#endif
	g_modBusDataStoreBufferIndex = 0;
	memset(g_modBusDataStoreBuffer,0,RS485_MAX_DATA_LENGTH);

	//This loop creates modbus command from the configuration, sends the command, verifies the receieved data.
	for(;;)
	{
		rs485SlaveID		=	g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
		rs485SlaveID		=	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
		configCmdIndex++;
		if(rs485BaudRate	!=	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex])
		{
			rs485BaudRate	=	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
			usfn_ChangeRs485BaudRate(rs485BaudRate);
		}
#if LCR_CONF_RS485_DEBUG_ENABLE
		chprintf(g_chp,"startModBusComm : SlaveID - %u.\r\n", rs485SlaveID);
#endif
		configCmdIndex = configCmdIndex+2;
		for(;;)
		{
			modBusQuery[0]	= rs485SlaveID;
			switch(g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex])
			{
				case 01 :
							configCmdIndex++;
							noOfQueries				= (uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
							configCmdIndex++;
							for(int i=noOfQueries;i>0;i--)
							{
								modBusQuery[0]		= 	rs485SlaveID;
								modBusQuery[1]		=	0x01;
								modBusQuery[2] 		= 	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
								configCmdIndex++;
								modBusQuery[3]  	= 	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
								configCmdIndex++;
								modBusQuery[4]		=	0x00;
								modBusQuery[5]		=	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
								usfn_SendModBusQuery(modBusQuery, modbusConfigMapIndex);
								configCmdIndex++;
							}
							break;
				case 02 :
							configCmdIndex++;
							noOfQueries				= (uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
							configCmdIndex++;
							for(int i=noOfQueries;i>0;i--)
							{
								modBusQuery[0]		= rs485SlaveID;
								modBusQuery[1]		=	0x02;
								modBusQuery[2] 		= 	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
								configCmdIndex++;
								modBusQuery[3]  	= 	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
								configCmdIndex++;
								modBusQuery[4]		=	0x00;
								modBusQuery[5]		=	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
								usfn_SendModBusQuery(modBusQuery,modbusConfigMapIndex);
								configCmdIndex++;
							}
							break;
				case 03 :
							configCmdIndex++;
							noOfQueries				= (uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
							configCmdIndex++;
							for(int i=noOfQueries;i>0;i--)
							{
								modBusQuery[0]		= rs485SlaveID;
								modBusQuery[1]		=	0x03;
								modBusQuery[2] 		= 	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
								configCmdIndex++;
								modBusQuery[3]  	= 	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
								configCmdIndex++;
								modBusQuery[4]		=	0x00;
								modBusQuery[5]		=	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
								usfn_SendModBusQuery(modBusQuery,modbusConfigMapIndex);
								configCmdIndex++;
							}
							break;
				case 04 :
							configCmdIndex++;
							noOfQueries				= (uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
							configCmdIndex++;
							for(int i=noOfQueries;i>0;i--)
							{
								modBusQuery[0]		= rs485SlaveID;
								modBusQuery[1]		=	0x04;
								modBusQuery[2] 		= 	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
								configCmdIndex++;
								modBusQuery[3]  	= 	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
								configCmdIndex++;
								modBusQuery[4]		=	0x00;
								modBusQuery[5]		=	(uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex];
								usfn_SendModBusQuery(modBusQuery,modbusConfigMapIndex);
								configCmdIndex++;
							}
							break;
			}
			if(g_modbusConfigMap[modbusConfigMapIndex].modBusErrorCount > MODBUS_COMMUNICATION_ERROR_COUNT_TH)
			{
#if LCR_CONF_RS485_DEBUG_ENABLE
				chprintf(g_chp,"startModBusComm : ERROR - While receiving modbus data.\r\n");
#endif
				return;
			}
			if((uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex] == RS485_CONFIG_NEXT_FUNCTION_REG)
			{
				configCmdIndex++;
			}
			else if(g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex] == RS485_CONFIG_NEXT_SLAVE)
			{
				break;
			}
			else if((uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex] == RS485_CONFIG_END)
			{
				if((g_modbusConfigMap[modbusConfigMapIndex].dataLength == g_modBusDataStoreBufferIndex) && (g_modbusConfigMap[modbusConfigMapIndex].modBusErrorCount == 0))
				{
#if LCR_CONF_RS485_DEBUG_ENABLE
					chprintf(g_chp,"startModBusComm : Total modbus data Length is verified.\r\n");
#endif
					//If there is no error in communication store the received data.
					memcpy(g_modbusConfigMap[modbusConfigMapIndex].modBusData,g_modBusDataStoreBuffer,g_modBusDataStoreBufferIndex);
				}
				else
				{
#if LCR_CONF_RS485_DEBUG_ENABLE
					chprintf(g_chp,"startModBusComm : ERROR - Total modbus data Length is incorrect.\r\n");
#endif
				}
				return;
			}
			else
			{
#if LCR_CONF_RS485_DEBUG_ENABLE
				chprintf(g_chp,"startModBusComm1 : ERROR - Didn't end with modbus cfg end .\r\n");
#endif
				return;
			}
		}
		if((uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex] == RS485_CONFIG_NEXT_SLAVE)
		{
			configCmdIndex++;
		}
#if 0
		else if((uint8_t)g_modbusConfigMap[modbusConfigMapIndex].modBusConfigCmd[configCmdIndex] == RS485_CONFIG_END)
		{
			return;
		}
#endif
		else
		{
#if LCR_CONF_RS485_DEBUG_ENABLE
			chprintf(g_chp,"startModBusComm2 : ERROR - Didn't end with modbus cfg end.\r\n");
#endif
			break;
		}
	}
	return;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_ChangeRs485BaudRate.
*Description: 	sets baudrate for the communication.
*Arguments	: 	baudRate	- Baudarate no in corresponding to connectm's protocol.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void usfn_ChangeRs485BaudRate(uint8_t baudRate)
{
	uint32_t	baud;
	sdStop(RS485_PORT);
	switch(baudRate)
	{
		case	0x01	:	baud	= 300;
							break;
		case	0x02	:	baud	= 600;
							break;
		case	0x03	:	baud	= 1200;
							break;
		case	0x04	:	baud	= 2400;
							break;
		case	0x05	:	baud	= 4800;
							break;
		case	0x06	:	baud	= 9600;
							break;
		case	0x07	:	baud	= 19200;
							break;
		case	0x08	:	baud	= 38400;
							break;
		case	0x09	:	baud	= 57600;
							break;
		case	0x0A	:	baud	= 115200;
							break;
		case	0x0B	:	baud	= 230400;
							break;
		case	0x0C	:	baud	= 460800;
							break;
		case	0x0D	:	baud	= 921600;
							break;
		default			:	baud	= 9600;
							break;
	}
	SerialConfig g_SerialConfig =	{
										baud,
										0,
										0,
										0
									};

	sdStart(RS485_PORT, &g_SerialConfig);
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_SendModBusQuery.
*Description: 	Sends modbus command and verifies received data.
*Arguments	: 	modBusQuery				- modbus command buffer.
				modbusConfigMapIndex	- modbus configuration's slot no.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void usfn_SendModBusQuery(uint8_t* modBusQuery, uint8_t modbusConfigMapIndex)
{
#if LCR_CONF_RS485_DEBUG_ENABLE
		chprintf(g_chp,"usfn_SendModBusQuery : Entry.\r\n");
#endif
	uint16_t 	modBusCrc = 0;
	uint8_t 	modBusRxBuffer[MODBUS_RX_BUFFER_MAX] = "";
	uint16_t	modBusRxDataLength = 0;
	uint16_t	modBusExpectedDataLength = 0;
	uint8_t 	modBusRetryCount	= 0;

	//calculates the crc.
	modBusCrc = crcCalculator( modBusQuery , MODBUS_QUERY_LEN_WITHOUT_CRC);
	modBusQuery[6] = (uint8_t)(modBusCrc & 0x00FF);
	modBusQuery[7] = (uint8_t)((modBusCrc & 0xFF00)>>8);

	//Calculate the expected data length for the current modbus command.
	switch(modBusQuery[1])
	{
		case FUNCTION_CODE_COIL_STATUS	:	modBusExpectedDataLength	= modBusQuery[5]/8;
											if(modBusQuery[5]%8)
												modBusExpectedDataLength++;
											break;
		case FUNCTION_CODE_INPUT_STATUS	:	modBusExpectedDataLength	= modBusQuery[5]/8;
											if(modBusQuery[5]%8)
												modBusExpectedDataLength++;
											break;
		case FUNCTION_CODE_HOLDING_REG	:	modBusExpectedDataLength	= modBusQuery[5]*2;
											break;
		case FUNCTION_CODE_INPUT_REG	:	modBusExpectedDataLength	= modBusQuery[5]*2;
											break;
	}
	modBusExpectedDataLength = modBusExpectedDataLength + MODBUS_EXPECTED_DATA_LENGTH_INCLUDE;
	uint64_t *lcrAlarmStructurePtr = (uint64_t*)&g_lcrAlarmStructure.alarmArray[0];
	uint64_t alarmMask = 0;
	for(modBusRetryCount = 0; modBusRetryCount < MODBUS_QUERY_RETRY_COUNT; modBusRetryCount++)
	{
#if LCR_CONF_RS485_DEBUG_ENABLE
		usfn_PrintArray(modBusQuery,MODBUS_QUERY_SIZE);
#endif
		modBusRxDataLength = 0;
		RS485_TX_LED_ON();
		RS485_TX_ENABLE();
		chThdSleepMicroseconds(9);
		sdWrite(RS485_PORT, (const uint8_t *)modBusQuery, MODBUS_QUERY_SIZE);
		chThdSleepMicroseconds(8225);
		RS485_RX_ENABLE();
		RS485_TX_LED_OFF();
		modBusRxDataLength = sdReadTimeout (RS485_PORT, (uint8_t *)modBusRxBuffer, modBusExpectedDataLength,TIME_S2I(MODBUS_RESPONSE_WAIT_TIME));
#if LCR_CONF_RS485_DEBUG_ENABLE
		usfn_PrintArray(modBusRxBuffer,modBusRxDataLength);
#endif
		if((crcCalculator( modBusRxBuffer , modBusRxDataLength) == 0) && (modBusExpectedDataLength == modBusRxDataLength))
		{
#if LCR_CONF_RS485_DEBUG_ENABLE
			chprintf(g_chp,"usfn_SendModBusQuery : Valid modbus data received. Expected Length - %u: Received Length - %u.\r\n", modBusExpectedDataLength,modBusRxDataLength);
#endif
			RS485_RX_LED_ON();
			memcpy(g_modBusDataStoreBuffer+g_modBusDataStoreBufferIndex,modBusRxBuffer+MODBUS_ACTUAL_DATA_OFFSET,modBusExpectedDataLength-MODBUS_EXPECTED_DATA_LENGTH_INCLUDE);
			g_modBusDataStoreBufferIndex = g_modBusDataStoreBufferIndex+modBusExpectedDataLength-MODBUS_EXPECTED_DATA_LENGTH_INCLUDE;
			if(g_modbusConfigMap[modbusConfigMapIndex].modBusErrorCount)
			{
				g_modbusConfigMap[modbusConfigMapIndex].modBusErrorCount--;
			}
			else
			{
				alarmMask = usfn_pow(g_modbusConfigMap[modbusConfigMapIndex].configChannel+8);
				*lcrAlarmStructurePtr = (uint64_t)((*lcrAlarmStructurePtr) & ~(alarmMask));
			}
			RS485_RX_LED_OFF();
			return;

		}
		else
		{
#if LCR_CONF_RS485_DEBUG_ENABLE
			chprintf(g_chp,"usfn_SendModBusQuery : ERROR - Inalid modbus data received. Expected Length - %u: Received Length - %u.\r\n", modBusExpectedDataLength,modBusRxDataLength);
#endif

			alarmMask = usfn_pow(g_modbusConfigMap[modbusConfigMapIndex].configChannel + 8);
			*lcrAlarmStructurePtr = (uint64_t)((*lcrAlarmStructurePtr) | (alarmMask));

		}
	}
	g_modbusConfigMap[modbusConfigMapIndex].modBusErrorCount++;
	if(g_modbusConfigMap[modbusConfigMapIndex].modBusErrorCount >= 0x0F)
		g_modbusConfigMap[modbusConfigMapIndex].modBusErrorCount = 0x0F;


}


/*------------------------------------------------------------------------
*Function 	: 	usfn_pow.w
*Description: 	Get two to the power of passed no(2^configNo).
				This is used to set/reset the flag for modbus communication.
*Arguments	: 	configNo	-	value of exponent.
*Return	 	: 	uint64_t	-	value of (2^configNo)
*------------------------------------------------------------------------*/
static uint64_t usfn_pow(uint8_t configNo)
{
	uint64_t retData = 1;
	for(int i=0; i<configNo; i++)
	{
		retData = retData*2;
	}
	return retData;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_PrintArray.
*Description: 	A simple function to print array values to console.
*Arguments	: 	*data	-	Pointer to buffer.
				dataLen - 	Length of buffer.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
void usfn_PrintArray(uint8_t *data, uint16_t dataLen)
{
	chprintf(g_chp,"\r\n\r\n");
	for(uint16_t i = 0; i< dataLen;i++)
		chprintf(g_chp,"%X ", data[i]);
	chprintf(g_chp,"\r\n\r\n");
}
