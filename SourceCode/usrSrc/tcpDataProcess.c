#include "hal.h"
#include "ch.h"
#include "chprintf.h"

#include "string.h"
#include "stdio.h"

#include "crc16.h"
#include "digitalIO.h"
#include "eventmanager.h"
#include "global.h"
#include "lcrConfig.h"
#include "modem.h"
#include "rs485Communication.h"
#include "tcpDataProcess.h"
#include "uartModem.h"


static uint16_t	g_tcpDataPacketSequence = 0;

/*------------------------------------------------------------------------
*Function 	: 	usfn_GetInitializedPacket.
*Description: 	Appends prefix to the tcp packet.
*Arguments	: 	*tcpData - Pointer to the TCP packet.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
void usfn_GetInitializedPacket(tcpData_t *tcpData)
{
	uint16_t	year	= 0;
	RTCDateTime rtcDateTimeHal 	= {0};
	struct tm 	rtcStdTime		= {0};

	rtcGetTime(&RTCD1, &rtcDateTimeHal);
	rtcConvertDateTimeToStructTm(&rtcDateTimeHal, &rtcStdTime, NULL);

//	chprintf(g_chp,"usfn_GetInitializedPacket : Year- %u, Month- %u, Date- %u, Hour- %u, Minutes- %u, Seconds- %u.\r\n",
//			rtcDateTimeHal.year,rtcDateTimeHal.month,rtcDateTimeHal.day,rtcStdTime.tm_hour, rtcStdTime.tm_min, rtcStdTime.tm_sec);

	year = rtcDateTimeHal.year + TCP_DATA_BASE_YEAR;

	tcpData->stx 				= TCP_DATA_STX;
	tcpData->appID 				= TCP_DATA_APP_ID;
	tcpData->year				= (uint16_t)(year >> 8) | (year << 8);
	tcpData->month				= (uint8_t) rtcDateTimeHal.month;
	tcpData->date				= (uint8_t) rtcDateTimeHal.day;
	tcpData->hours				= (uint8_t) rtcStdTime.tm_hour;
	tcpData->minutes			= (uint8_t) rtcStdTime.tm_min;
	tcpData->seconds			= (uint8_t) rtcStdTime.tm_sec;
	g_tcpDataPacketSequence++;
	if(0xFFFF == g_tcpDataPacketSequence)
	{
		g_tcpDataPacketSequence = 0;
	}
	tcpData->packetSequence 	= (uint16_t)(g_tcpDataPacketSequence >> 8) | (g_tcpDataPacketSequence << 8);
	tcpData->imei			= g_gsmModemDetails_t.imeiHex;
	tcpData->imeiEnd 		= TCP_DATA_IMEI_END;
	tcpData->deviceVersionID = TCP_DATA_DEVICE_VERSION_ID;
}


#if LCR_CONF_TCP_DATA_IN_SINGLE_FRAME
/*------------------------------------------------------------------------
*Function 	: 	usfn_CollectRegularDataInSingleFrame.
*Description: 	Collects regular/device data.
*Arguments	: 	*tcpData - Pointer to the TCP packet.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
void usfn_CollectRegularDataInSingleFrame(tcpData_t *tcpData)
{
	uint8_t modBusConfigIdx  = 0;
	uint16_t tcpDataLength 	 = 0;

	tcpData->deviceID 		= LCR_CONF_SINGLE_FRAME_TCP_DATA_DEVICE_ID;
	tcpData->deviceVersionID= TCP_DATA_DEVICE_VERSION_ID;
	tcpData->frameID		= LCR_CONF_SINGLE_FRAME_TCP_DATA_FRAME_ID;

#if LCR_CONF_DIGITAL_IO_BOARD_CONNECTED
	memcpy(tcpData->data + tcpDataLength,&g_dIoAdcData_t,TCP_DATA_LEN_DIGITAL_IO);
	tcpDataLength = tcpDataLength + TCP_DATA_LEN_DIGITAL_IO;
#endif // LCR_CONF_DIGITAL_IO_BOARD_CONNECTED
	chMtxLock(&g_mutex_rs485DataLock);
	while(modBusConfigIdx < g_lcrGlobalData.noOfModbusConfigs)
	{
		memcpy(tcpData->data + tcpDataLength,g_modbusConfigMap[modBusConfigIdx].modBusData, g_modbusConfigMap[modBusConfigIdx].dataLength);
		tcpDataLength 	= tcpDataLength + g_modbusConfigMap[modBusConfigIdx].dataLength;
		modBusConfigIdx++;
	}
	chMtxUnlock(&g_mutex_rs485DataLock);
#if LCR_CONF_TCP_DATA_ADD_ZEROS_AT_THE_END
	tcpDataLength = tcpDataLength + LCR_CONF_TCP_DATA_EXTRA_BYTES_PADD_SIZE;
	memset(tcpData->data + tcpDataLength,0,LCR_CONF_TCP_DATA_EXTRA_BYTES_PADD_SIZE);
#endif
	tcpData->noOfBytes 	= tcpDataLength;
}
#else //#if LCR_CONF_TCP_DATA_IN_SINGLE_FRAME
/*------------------------------------------------------------------------
*Function 	: 	usfn_CollectRegularData.
*Description: 	Collects regular/device data.
*Arguments	: 	*tcpData - Pointer to the TCP packet.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
void usfn_CollectRegularDataInMultiFrame(tcpData_t *tcpData, uint8_t modBusConfigIndex)
{

			tcpData->noOfBytes 	= g_modbusConfigMap[modBusConfigIndex].dataLength;
			tcpData->deviceID 	= g_modbusConfigMap[modBusConfigIndex].modBusConfigCmd[MODBUS_CFG_FRAME_ID_INDEX];
			tcpData->frameID	= g_modbusConfigMap[modBusConfigIndex].modBusConfigCmd[MODBUS_CFG_SLAVE_ID_INDEX];
			memcpy(tcpData->data,g_modbusConfigMap[modBusConfigIndex].modBusData, g_modbusConfigMap[modBusConfigIndex].dataLength);
}
#endif //#if LCR_CONF_TCP_DATA_IN_SINGLE_FRAME



/*------------------------------------------------------------------------
*Function 	: 	usfn_CollectDataAndPushToServer.
*Description: 	Frames tcp packet and pushes to server.
*Arguments	: 	packetType - A macro which defines type of packet to be sent
*Return	 	: 	bool	- 	TRUE - on Error.
							FALSE- on Success.
*------------------------------------------------------------------------*/
bool usfn_CollectDataAndPushToServer(uint8_t packetType)
{
	static 	tcpData_t tcpData={0};
	bool 		status = TRUE;
	uint16_t	tcpDataLength	= 0;
	switch(packetType)
	{
		case SEND_TCP_DATA_REGULAR_FLAG 	:	chprintf(g_chp,"usfn_CollectDataAndPushToServer : Pushing regular data.\r\n");
												tcpData.packetType 	= TCP_DATA_PCKET_TYPE_REGULAR;
												tcpData.deviceVersionID = TCP_DATA_DEVICE_VERSION_ID;
#if LCR_CONF_TCP_DATA_IN_SINGLE_FRAME
												usfn_GetInitializedPacket(&tcpData);
												usfn_CollectRegularDataInSingleFrame(&tcpData);
												tcpDataLength = usfn_CompleteTheTcpPacket(&tcpData);
												if(usfn_PushDataToServer(&tcpData, tcpDataLength))
													return TRUE;
#else //#if LCR_CONF_TCP_DATA_IN_SINGLE_FRAME
												uint8_t		modBusConfigIdx = 0;
												while(modBusConfigIdx < g_lcrGlobalData.noOfModbusConfigs)
												{
													if(0 == g_modbusConfigMap[modBusConfigIdx].modBusErrorCount)
													{
														usfn_GetInitializedPacket(&tcpData);
														usfn_CollectRegularDataInMultiFrame(&tcpData, modBusConfigIdx);
														tcpDataLength = usfn_CompleteTheTcpPacket(&tcpData);
														if(usfn_PushDataToServer(&tcpData, tcpDataLength))
															return TRUE;
													}
													else
														chprintf(g_chp,"usfn_CollectDataAndPushToServer : No data for %X modbus configuration.\r\n", g_modbusConfigMap[modBusConfigIdx].modBusConfigCmd[MODBUS_CFG_FRAME_ID_INDEX]);
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
												chprintf(g_chp,"usfn_CollectDataAndPushToServer : Sending DIO status\r\n");
												if(usfn_PushDataToServer(&tcpData, tcpDataLength))
													return TRUE;
#endif // LCR_CONF_DIGITAL_IO_BOARD_CONNECTED
#endif //#if LCR_CONF_TCP_DATA_IN_SINGLE_FRAME
												status = FALSE;
												break;
		case SEND_TCP_DATA_ALARM_FLAG 		:	chprintf(g_chp,"usfn_CollectDataAndPushToServer : Pushing Alarm packet.\r\n");
												usfn_GetInitializedPacket(&tcpData);
												tcpData.packetType 	= TCP_DATA_PCKET_TYPE_FAULT;
												tcpData.deviceID	= TCP_DATA_DEVICE_ID_ALARM;
												tcpData.deviceVersionID = TCP_DATA_DEVICE_VERSION_ID;
												tcpData.frameID		= TCP_DATA_FRAME_ID;
												tcpData.noOfBytes	= (uint16_t)TCP_DATA_LEN_ALARM;
												tcpData.data[0]		= g_lcrAlarmStructure.alarmArray[5];
												tcpData.data[1]		= g_lcrAlarmStructure.alarmArray[2];
												tcpData.data[2]		= g_lcrAlarmStructure.alarmArray[0];
												tcpData.data[3]		= g_lcrAlarmStructure.alarmArray[1];
												tcpDataLength = usfn_CompleteTheTcpPacket(&tcpData);
												status = usfn_PushDataToServer(&tcpData, tcpDataLength);
												break;
		case SEND_TCP_DATA_HEART_BEAT_FLAG 	:	chprintf(g_chp,"usfn_CollectDataAndPushToServer : Pushing HeartBeat packet.\r\n");
												usfn_GetInitializedPacket(&tcpData);
												tcpData.packetType 	= TCP_DATA_PCKET_TYPE_REGULAR;
												tcpData.deviceID	= TCP_DATA_DEVICE_ID_HEART_BEAT;
												tcpData.deviceVersionID = TCP_DATA_DEVICE_VERSION_ID;
												tcpData.frameID		= TCP_DATA_FRAME_ID;
												tcpData.noOfBytes	= (uint16_t)TCP_DATA_LEN_HEART_BEAT;
												tcpData.data[0]		= 0x01;
												tcpData.data[1]		= 0x01;
												tcpData.data[2]		= g_lcrGlobalData.simSignalStrngth;
												tcpDataLength = usfn_CompleteTheTcpPacket(&tcpData);
												status = usfn_PushDataToServer(&tcpData, tcpDataLength);
												break;
		case SEND_TCP_POWER_UP_MSG			:	chprintf(g_chp,"usfn_CollectDataAndPushToServer : Pushing PowerUp packet.\r\n");
												usfn_GetInitializedPacket(&tcpData);
												tcpData.packetType 	= TCP_DATA_PCKET_TYPE_REGULAR;
												tcpData.deviceID	= TCP_DATA_DEVICE_ID_POWER_UP_MSG;
												tcpData.frameID		= TCP_DATA_FRAME_ID;
												tcpData.noOfBytes	= (uint16_t)TCP_DATA_LEN_POWER_UP_MSG;
												tcpDataLength = usfn_CompleteTheTcpPacket(&tcpData);
												status = usfn_PushDataToServer(&tcpData, tcpDataLength);
												break;
		case SEND_TCP_MODBUS_CFG_ACK_FLAG	:	chprintf(g_chp,"usfn_CollectDataAndPushToServer : Pushing Acknoledgement packet.\r\n");
												usfn_GetInitializedPacket(&tcpData);
												tcpData.packetType 	= TCP_DATA_PCKET_TYPE_REGULAR;
												tcpData.deviceID	= TCP_DATA_DEVICE_ID_CMD_ACK;
												tcpData.deviceVersionID = TCP_DATA_DEVICE_VERSION_ID;
												tcpData.frameID		= TCP_DATA_FRAME_ID;
												tcpData.noOfBytes	= (uint16_t)TCP_DATA_LEN_CMD_ACK;
												tcpData.data[0]		= (uint8_t)(g_lcrGlobalData.modBusConfigAck >> 8);
												tcpData.data[1]		= (uint8_t)(g_lcrGlobalData.modBusConfigAck & 0x00FF);
												tcpDataLength = usfn_CompleteTheTcpPacket(&tcpData);
												status = usfn_PushDataToServer(&tcpData, tcpDataLength);
												break;
	}
	return status;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_CompleteTheTcpPacket.
*Description: 	Adds crc and ETX to the tcp packet.
*Arguments	: 	*tcpData 	- Pointer to the TCP packet.
*Return	 	: 	uint16_t	- 	length of the TCP packet.
*------------------------------------------------------------------------*/
uint16_t usfn_CompleteTheTcpPacket(tcpData_t * tcpData)
{
	uint16_t tcpDataLength 		= 0;
	uint16_t payLoadLength		= 0;
	uint16_t crc				= 0;

	payLoadLength = tcpData->noOfBytes;
	tcpDataLength = payLoadLength + TCP_DATA_LEN_PREFIX;
	tcpData->noOfBytes = (uint16_t)(payLoadLength >> 8) | (payLoadLength << 8);

	crc  = crcCalculator((uint8_t*)tcpData , tcpDataLength);
	tcpData->data[payLoadLength++] = (uint8_t)(crc & 0x00FF);
	tcpData->data[payLoadLength++] = (uint8_t)((crc & 0xFF00)>>8);
	tcpData->data[payLoadLength++] = (uint8_t)TCP_DATA_ETX;

	return (tcpDataLength+TCP_DATA_LEN_SUFFIX_LENGTH);

}
/*------------------------------------------------------------------------
*Function 	: 	usfn_PushDataToServer.
*Description: 	Pushed data to the server.
*Arguments	: 	*tcpData 	- Pointer to the TCP packet.
				tcpDataLen	- Length of TCP data.
*Return	 	: 	bool	- 	TRUE - on Error.
							FALSE- on Success.
*------------------------------------------------------------------------*/
bool usfn_PushDataToServer(tcpData_t *tcpData, uint16_t tcpDataLen)
{
	char atModemResp[MODEM_UART_BUFFER_SIZE] = "";
	char atModemCmd[MODEM_UART_BUFFER_SIZE] = "";
	bool status					= TRUE;
	uint8_t	retryCount			= 5;


	usfn_PrintArray((uint8_t*)tcpData, tcpDataLen);
	sprintf(atModemCmd,"AT+KTCPSND=%u,%u\r", g_lcrGlobalData.cloudServerSession, tcpDataLen);
	usfn_SendAtCommand(atModemCmd,atModemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
	while(retryCount)
	{
		usfn_SendAtCommand(NULL,atModemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
		if(strstr(atModemResp,"CONNECT"))
		{
			status = FALSE;
			break;
		}
		memset(atModemResp,0,MODEM_UART_BUFFER_SIZE);
		retryCount--;
	}
	memset(atModemResp,0,MODEM_UART_BUFFER_SIZE);
	retryCount = 5;
	if(!status)
	{
		status = TRUE;
		sdWrite(MODEM_PORT, (const uint8_t *)tcpData, (size_t)tcpDataLen);
		usfn_SendAtCommand(TCP_DATA_TXN_END_OF_PATTERN,atModemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
		while(retryCount)
		{
			usfn_SendAtCommand(NULL,atModemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
			if(strstr(atModemResp,"OK"))
			{
				status = FALSE;
				break;
			}
			memset(atModemResp,0,MODEM_UART_BUFFER_SIZE);
			retryCount--;
		}
		if(!status)
		{
#if LCR_CONF_TCP_COM_DEBUG_ENABLE
			chprintf(g_chp,"usfn_PushDataToServer : SUCCESS - Data sent to cloud.\r\n");
#endif
			return FALSE;
		}
		else
		{
			chprintf(g_chp,"%s.\r\n",atModemResp);
			chprintf(g_chp,"usfn_PushDataToServer : ERROR -	Couldn't send data to the cloud - 2.\r\n");
		}
	}
	else
	{
		chprintf(g_chp,"%s.\r\n",atModemResp);
		chprintf(g_chp,"usfn_PushDataToServer : ERROR -	Couldn't send data to the cloud - 1.\r\n");
		return TRUE;
	}

	return TRUE;
}

