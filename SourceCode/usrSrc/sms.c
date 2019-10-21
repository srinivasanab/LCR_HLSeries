#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "eventManager.h"
#include "uartModem.h"
#include "lcrConfig.h"
#include "rs485Process.h"
#include "rs485Communication.h"
#include "global.h"
#include "modem.h"
#include "version.h"
#include "sms.h"
#include "sdCard.h"

static lcrEvent 		g_lcrEvent = LCR_EVENT_INVALID;	//Event received.

static void usfn_SmsProcess(void);

/*------------------------------------------------------------------------
*Function 	: 	usfn_CheckSMS.
*Description: 	Checks whther any SMS present in the inbox.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
void usfn_CheckSMS(void)
{
	char modemResp[SMS_TEXT_MAX_SIZE] = "";
	char *charPtr = NULL;
	usfn_SendAtCommand("AT+CSQ\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
//#if LCR_CONF_MODEM_RESPONSE_ENABLE
//	chprintf(g_chp,"%s.\r\n",modemResp);
//#endif
	if(strstr(modemResp, "CSQ"))
	{
		charPtr = strtok(modemResp," ");
		charPtr = strtok(NULL,",");
		g_lcrGlobalData.simSignalStrngth = (uint8_t)atoi(charPtr);
	}
	memset(modemResp,0,SMS_TEXT_MAX_SIZE);
	usfn_SendAtCommand("AT+CMGR=1\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
	if(strstr(modemResp, "REC UNREAD"))
	{
		chprintf(g_chp,"usfn_CheckSMS : SMS Received...!\r\n");
		usfn_SmsProcess();
	}
	else if(strstr(modemResp, "REC READ"))
	{
		usfn_SendAtCommand("AT+CMGD=1,4\r",modemResp,MODEM_CMD_RESPONSE_WAIT_TIME);
		if(strstr(modemResp,"ERROR"))
		{
			chprintf(g_chp,"usfn_CheckSMS : ERROR - Clouldn't delete the SMS.\r\n");
		}
	}
	else
		return;
}

/*------------------------------------------------------------------------
*Function 	: 	usfn_SmsProcess.
*Description: 	Process's the received sms and creates appropriate reply.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void usfn_SmsProcess(void)
{
	chprintf(g_chp,"usfn_SmsProcess : Entry.\r\n");
	char modemResp[SMS_TEXT_MAX_SIZE] = "";
	char smsRd[SMS_TEXT_MAX_SIZE] = "";
	char smsCmd[50] = "";
	char smsReply[SMS_TEXT_MAX_SIZE+400] = "";
	char smsReplyTail[50] = "";
	char senderPhNo[15]="";
	char *charPtr	=	NULL;
	usfn_SendAtCommand("AT+CMGR=1\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
	strcpy(smsRd,modemResp);
//	usfn_SendAtCommand("AT+CMGD=1,4\r",modemResp,MODEM_CMD_RESPONSE_WAIT_TIME);
//	if(NULL == (strstr(modemResp,"OK")))
//	{
//		chprintf(g_chp,"usfn_SmsRead : ERROR - Clouldn't delete the SMS.\r\n");
//	}
	charPtr = strstr((char *)smsRd,"REC");

	if(NULL == charPtr)
	{
		chprintf(g_chp,"usfn_SmsProcess : ERROR - Clouldn't read the SMS.\r\n");
		return;
	}
	charPtr = strtok(charPtr,"\"");
	charPtr = strtok(NULL,"\"");
	charPtr = strtok(NULL,"\"");
	strcpy(senderPhNo,charPtr);
	charPtr = strtok(NULL,"\n");
	charPtr = strtok(NULL,"\n");
	strcpy(smsCmd,charPtr);
	charPtr = NULL;
	chprintf(g_chp,"usfn_SmsProcess : Sender -> %s : %s.\r\n", senderPhNo, smsCmd);

	if(strstr(smsCmd,SMS_READ_STATUS))
	{
		sprintf(smsReply,"Alarm : %X%X%X%X%X%X",g_lcrAlarmStructure.alarmArray[6],g_lcrAlarmStructure.alarmArray[5],g_lcrAlarmStructure.alarmArray[4],g_lcrAlarmStructure.alarmArray[3],g_lcrAlarmStructure.alarmArray[2],g_lcrAlarmStructure.alarmArray[1]);
	}
	else if(strstr(smsCmd,SMS_READ_IP_CFG))
	{
		sprintf(smsReply,"Cloud : %s,%u\nTimer : %s,%u", g_simCloudDetails_t.cloudServerIP,g_simCloudDetails_t.cloudServerPort,g_simCloudDetails_t.timeServerIP,g_simCloudDetails_t.timeServerPort);;
	}
	else if(strstr(smsCmd,SMS_READ_FTP_CFG))
	{
		sprintf(smsReply,"FTP: IP- %s, UserName- %s, Password- %s", g_simCloudDetails_t.ftpIP, g_simCloudDetails_t.ftpUserName, g_simCloudDetails_t.ftpPassword);
	}
	else if(strstr(smsCmd,SMS_READ_APNNAME1))
	{
		sprintf(smsReply,"Access Point Name 1 : %s", g_simCloudDetails_t.apnName1);
	}
	else if(strstr(smsCmd,SMS_READ_APNNAME2))
	{
		sprintf(smsReply,"Access Point Name 1 : %s", g_simCloudDetails_t.apnName2);
	}

	else if(strstr(smsCmd,SMS_READ_MODBUS_CFG))
	{
		char storeModbusCfg[50] = "";
		for(uint8_t i=0; i < g_lcrGlobalData.noOfModbusConfigs; i++)
		{
			chMtxLock(&g_mutex_rs485DataLock);

			sprintf(storeModbusCfg,"MbCfg%u:F-%X,S-%X,B-%X\n",
			i,g_modbusConfigMap[i].modBusConfigCmd[MODBUS_CFG_FRAME_ID_INDEX],
			g_modbusConfigMap[i].modBusConfigCmd[MODBUS_CFG_SLAVE_ID_INDEX],
			g_modbusConfigMap[i].modBusConfigCmd[MODBUS_CFG_BAUDRATE_INDEX]);
			strcat(smsReply,storeModbusCfg);
			memset(storeModbusCfg,0,50);

			chMtxUnlock(&g_mutex_rs485DataLock);
		}
	}
	else if(strstr(smsCmd,SMS_READ_PERI_TIME))
	{
		sprintf(smsReply,"Peri Time : %u\n", g_simCloudDetails_t.periTime);
	}
	else if(strstr(smsCmd,SMS_WRITE_CLOUD_CFG))
	{
	    charPtr = strtok(smsCmd,"#");
	    charPtr = strtok(NULL,"#");
	    charPtr = strtok(NULL,"#");
	    strcpy(g_simCloudDetails_t.cloudServerIP,charPtr);
	    charPtr = strtok(NULL,"\r");
	    g_simCloudDetails_t.cloudServerPort = atoi(charPtr);
	    usfn_SdCardEditLcrCfgFile(CLOUD_IP,g_simCloudDetails_t.cloudServerIP,0,CONFIG_FILE_EDIT_STRING);
	    usfn_SdCardEditLcrCfgFile(CLOUD_PORT,&(g_simCloudDetails_t.cloudServerPort),0,CONFIG_FILE_EDIT_INT);
	    chprintf(g_chp,"usfn_SmsProcess : Cloud configurations - %s - %u\r\n", g_simCloudDetails_t.cloudServerIP, g_simCloudDetails_t.cloudServerPort);
	    strcpy(smsReply,"Cloud configurations set.\nRestarting the device...");
	    g_lcrEvent = LCR_EVENT_RESTART;
	}
	else if(strstr(smsCmd,SMS_WRITE_TIMER_CFG))
	{
		charPtr = strtok(smsCmd,"#");
		charPtr = strtok(NULL,"#");
		charPtr = strtok(NULL,"#");
	    strcpy(g_simCloudDetails_t.timeServerIP,charPtr);
	    charPtr = strtok(NULL,"\r");
	    g_simCloudDetails_t.timeServerPort = atoi(charPtr);
	    usfn_SdCardEditLcrCfgFile(TIME_IP,g_simCloudDetails_t.timeServerIP,0,CONFIG_FILE_EDIT_STRING);
	    usfn_SdCardEditLcrCfgFile(TIME_PORT,&(g_simCloudDetails_t.timeServerPort),0,CONFIG_FILE_EDIT_INT);
	    chprintf(g_chp,"usfn_SmsProcess : Timer configurations - %s - %u\r\n", g_simCloudDetails_t.timeServerIP, g_simCloudDetails_t.timeServerPort);
	    strcpy(smsReply,"Timer configurations set.\nRestarting the device...");
	    g_lcrEvent = LCR_EVENT_RESTART;
	}
	else if(strstr(smsCmd,SMS_WRITE_FTP_CFG))
	{
		charPtr = strtok(smsCmd,"#");
		charPtr = strtok(NULL,"#");
		charPtr = strtok(NULL,"#");
		strcpy(g_simCloudDetails_t.ftpIP,charPtr);
		charPtr = strtok(NULL,"#");
		strcpy(g_simCloudDetails_t.ftpUserName,charPtr);
		charPtr = strtok(NULL,"\r");
		strcpy(g_simCloudDetails_t.ftpPassword,charPtr);
	    usfn_SdCardEditLcrCfgFile(FTP_IP,g_simCloudDetails_t.ftpIP,0,CONFIG_FILE_EDIT_STRING);
	    usfn_SdCardEditLcrCfgFile(FTP_USERNAME,g_simCloudDetails_t.ftpUserName,0,CONFIG_FILE_EDIT_STRING);
	    usfn_SdCardEditLcrCfgFile(FTP_PASSWORD,g_simCloudDetails_t.ftpPassword,0,CONFIG_FILE_EDIT_STRING);
	    chprintf(g_chp,"usfn_SmsProcess : FTP configurations - %s - %s - %s\r\n", g_simCloudDetails_t.ftpIP, g_simCloudDetails_t.ftpUserName, g_simCloudDetails_t.ftpPassword);
	    strcpy(smsReply,"FTP configurations set.\nRestarting the device...");
	    g_lcrEvent = LCR_EVENT_RESTART;
	}
	else if(strstr(smsCmd,SMS_WRITE_APNNAME1))
	{
		charPtr = strtok(smsCmd,"#");
		charPtr = strtok(NULL,"#");
		charPtr = strtok(NULL,"\r");
		strcpy(g_simCloudDetails_t.apnName1,charPtr);
	    usfn_SdCardEditLcrCfgFile(APN_NAME_1,g_simCloudDetails_t.apnName1,0,CONFIG_FILE_EDIT_STRING);
		chprintf(g_chp,"usfn_SmsProcess : APN_NAME_1 - %s\r\n", g_simCloudDetails_t.apnName1);
		strcpy(smsReply,"ApnName1 set.\nRestarting the device...");
		g_lcrEvent = LCR_EVENT_RESTART;
	}
	else if(strstr(smsCmd,SMS_WRITE_APNNAME2))
	{
		charPtr = strtok(smsCmd,"#");
		charPtr = strtok(NULL,"#");
		charPtr = strtok(NULL,"\r");
		strcpy(g_simCloudDetails_t.apnName2,charPtr);
	    usfn_SdCardEditLcrCfgFile(APN_NAME_2,g_simCloudDetails_t.apnName2,0,CONFIG_FILE_EDIT_STRING);
		chprintf(g_chp,"usfn_SmsProcess : APN_NAME_2 - %s\r\n", g_simCloudDetails_t.apnName2);
		strcpy(smsReply,"ApnName2 set.\nRestarting the device...");
		g_lcrEvent = LCR_EVENT_RESTART;
	}
	else if(strstr(smsCmd,SMS_WRITE_PERI_TIME))
	{
		charPtr = strtok(smsCmd,"#");
		charPtr = strtok(NULL,"#");
		charPtr = strtok(NULL,"#");
		charPtr = strtok(NULL,"\r");
		g_simCloudDetails_t.periTime = atoi(charPtr);
		if(g_simCloudDetails_t.periTime < PERI_TIME_MINIMUM)
			g_simCloudDetails_t.periTime = PERI_TIME_MINIMUM;
	    usfn_SdCardEditLcrCfgFile(PERI_TIME,&(g_simCloudDetails_t.periTime),0,CONFIG_FILE_EDIT_INT);
		chprintf(g_chp,"usfn_SmsProcess : Peri Time - %u\r\n", g_simCloudDetails_t.periTime);
		strcpy(smsReply,"Peri Time set.\nRestarting the device...");
		g_lcrEvent = LCR_EVENT_RESTART;
	}
	else if(strstr(smsCmd,SMS_CLEAR_APNNAME1))
	{
		memset(g_simCloudDetails_t.apnName1,0,sizeof(g_simCloudDetails_t.apnName1));
		usfn_SdCardEditLcrCfgFile(APN_NAME_1,NULL,0,CONFIG_FILE_EDIT_STRING);
		chprintf(g_chp,"usfn_SmsProcess : Cleared APN_NAME_1.\r\n");
		strcpy(smsReply,"ApnName1 Cleared.\nRestarting the device...");
		g_lcrEvent = LCR_EVENT_RESTART;
	}
	else if(strstr(smsCmd,SMS_CLEAR_APNNAME2))
	{
		memset(g_simCloudDetails_t.apnName1,0,sizeof(g_simCloudDetails_t.apnName2));
		usfn_SdCardEditLcrCfgFile(APN_NAME_2,NULL,0,CONFIG_FILE_EDIT_STRING);
		chprintf(g_chp,"usfn_SmsProcess : Cleared APN_NAME_2.\r\n");
		strcpy(smsReply,"ApnName2 Cleared.\nRestarting the device...");
		g_lcrEvent = LCR_EVENT_RESTART;
	}
	else if(strstr(smsCmd,SMS_CLEAR_MODBUS_CFG))
	{
		chprintf(g_chp,"usfn_SmsProcess : Cleared ModBus Configuration.\r\n");
		usfn_SdCardEditLcrCfgFile(NULL,NULL,0,CONFIG_FILE_EDIT_ARRAY);
		strcpy(smsReply,"Modbus Configuration Cleared.\nRestarting the device...");
		g_lcrEvent = LCR_EVENT_RESTART;
	}
	else if(strstr(smsCmd,SMS_DEVICE_RESET))
	{
		strcpy(smsReply,"Restarting the device...");

		g_lcrEvent = LCR_EVENT_RESTART;
	}
	else if(strstr(smsCmd,SMS_CLEAR_TCPBACKUP))
	{
		usfn_SdCardDeleteFile(TCP_DATA_BACKUP_FILE_NAME);
		chprintf(g_chp,"usfn_SmsProcess : Deleting the TCP Backup file.\r\n");
		strcpy(smsReply,"TCP Backup File is removed.\nRestarting the device...");
		g_lcrEvent = LCR_EVENT_RESTART;
	}
	else if(strstr(smsCmd,SMS_START_DOTA))
	{
//		sprintf(smsReply,"");
	}
	else
	{
		chprintf(g_chp,"Invalid Message received...\r\n");
		strcpy(smsReply,"Invalid Message received...");
	}

	sprintf(smsReplyTail,"\nVersion :LCR %u.%u.%u IMEI :%s\n", LCR_VERSION_MAJOR,LCR_VERSION_MINOR,LCR_VERSION_REVISION,g_gsmModemDetails_t.imeiStr);
	strcat(smsReply,smsReplyTail);

	usfn_SendSms(smsReply,senderPhNo);

	if(g_lcrEvent == LCR_EVENT_RESTART)
		chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);

}


/*------------------------------------------------------------------------
*Function 	: 	usfn_SendSms.
*Description: 	Sends SMS to the sender.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
void usfn_SendSms(char *smsText, char *smsPhNo)
{
	char smsReply[SMS_TEXT_MAX_SIZE] = "";
	char modemResp[30] = "";
	uint8_t cntrlZ = 0x1A;
	chprintf(g_chp,"usfn_SendSms : Reply SMS\r\n___________________\r\n%s\r\n_______________________\r\n", smsText);
	sprintf(smsReply,"AT+CMGS=\"%s\"\r",smsPhNo);
	usfn_SendAtCommand(smsReply,modemResp,MODEM_CMD_RESPONSE_WAIT_TIME);
//	chprintf(g_chp, "usfn_SendSms : 1 :%s - %s.\r\n",smsReply,modemResp);
	memset(modemResp,0,30);
	usfn_SendAtCommand(smsText,modemResp,MODEM_CMD_RESPONSE_WAIT_TIME);
//	chprintf(g_chp, "usfn_SendSms : 2: %s - %s.\r\n",smsText, modemResp);
	sdWrite(MODEM_PORT, &cntrlZ, 1);
	memset(modemResp,0,30);
	usfn_SendAtCommand(NULL,modemResp,MODEM_CMD_RESPONSE_WAIT_TIME*4);
//	chprintf(g_chp, "usfn_SendSms : 3: %s.\r\n",modemResp);
}
