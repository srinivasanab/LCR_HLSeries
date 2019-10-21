/*
 * modem.c
 *
 *  Created on: Jun 30, 2019
 *      Author: Arjun Dongre
 */
#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "crc16.h"
#include "digitalIO.h"
#include "eventManager.h"
#include "global.h"
#include "lcrConfig.h"
#include "modem.h"
#include "rs485Communication.h"
#include "rs485Process.h"
#include "sdCard.h"
#include "sms.h"
#include "tcpDataProcess.h"
#include "uartModem.h"

//Global Space
static lcrEvent 		g_lcrEvent = LCR_EVENT_INVALID;	//Event received.
static int usfn_RtcDayofweek(int d, int m, int y);
/*------------------------------------------------------------------------
*Function 	: 	usfn_TurnONModem.
*Description: 	TurnsOn/Resets the modem
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
void usfn_TurnONModem(void)
{
	chprintf(g_chp,"usfn_TurnONModem : Turning ON the modem.\r\n");

	RESET_GSM_MODEM_ON();
	RESET_GSM_MODEM_RESET();
	chThdSleepSeconds(1);
	SET_GSM_MODEM_ON();
	chThdSleepMilliseconds(25);
	SET_GSM_MODEM_RESET();
	chThdSleepMilliseconds(50);
	RESET_GSM_MODEM_RESET();
	chThdSleepMilliseconds(25);
	RESET_GSM_MODEM_ON();

	chThdSleepSeconds(MODEM_TURN_ON_WAIT_TIME);

	chprintf(g_chp,"usfn_TurnONModem : End.\r\n");

}


/*------------------------------------------------------------------------
*Function 	: 	usfn_CheckModemCommunication.
*Description: 	Checks for modem communication. If communicating hardware
				flow control is enabled.
*Arguments	: 	None.
*Return	 	: 	FALSE	-	On success.
				TRUE	- 	On error.
*------------------------------------------------------------------------*/
bool usfn_CheckModemCommunication(void)
{
	enum modemCommunication_e	{
								AT,
								ATE0,
								AT_K_0_3,
								MODEM_COMM_END
							};
	usfn_ModemUartInitialization();
	char modemResp[MODEM_UART_BUFFER_SIZE]="";
	uint8_t retryCount = 0, restartCount = 0;
	uint8_t atCmdState = AT;
	chprintf(g_chp, "usfn_CheckModemCommunication : Test modem communication ,switchoff the echo & enable HW Flow control.\r\n");
	for(restartCount = 0; restartCount < MODEM_RESTART_RETRY_COUNT; restartCount++)
	{
		for(retryCount = 0; retryCount < MODEM_COMMUNICATION_RETRY_COUNT; retryCount++)
		{
			switch(atCmdState)
			{
				case AT			:	usfn_SendAtCommand("AT\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
									if(strstr((char*)modemResp,"OK"))
									{
										atCmdState = ATE0;
										retryCount = 0;
										break;
									}
									chprintf(g_chp,"usfn_CheckModemCommunication : ERROR - No modem Communication..!\r\n");
									break;
					case ATE0	:	usfn_SendAtCommand("ATE0\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
									if(strstr((char*)modemResp,"OK"))
									{
										atCmdState = AT_K_0_3;
										retryCount = 0;
										break;
									}
									chprintf(g_chp,"usfn_CheckModemCommunication : ERROR - Couldn't switch off the echo..!\r\n");
									break;

				case AT_K_0_3		:
#if LCR_CONF_MODEM_SERIAL_HW_FLOW_CONTROL
									usfn_SendAtCommand("AT&K3\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
#else
									usfn_SendAtCommand("AT&K0\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
#endif
									if(strstr((char*)modemResp,"OK"))
									{
										atCmdState = MODEM_COMM_END;
										retryCount = 0;
										break;
									}
								chprintf(g_chp,"usfn_CheckModemCommunication : ERROR - Couldn't set the Hardware Flow Control..!\r\n");
								break;
			}
#if LCR_CONF_MODEM_RESPONSE_ENABLE
			chprintf(g_chp, "usfn_CheckModemCommunication : AT response - \r\n%s\r\n", modemResp);
			chThdSleepMilliseconds(200);
#endif
			memset(modemResp,0x00,sizeof(modemResp));
			if(atCmdState == MODEM_COMM_END)
			{
				return FALSE;
			}
		}
		usfn_TurnONModem();
		chprintf(g_chp,"usfn_CheckModemCommunication : ERROR - Couldn't test the modem communication. Restarting the modem...\r\n");
	}
	chprintf(g_chp,"usfn_CheckModemCommunication : ERROR - Couldn't test modem communication.\r\n");
	return TRUE;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_GetModemDetails.
*Description: 	Gets modem details like model no, firmware version, IMEI
*Arguments	: 	None.
*Return	 	: 	FALSE	-	On success.
				TRUE	- 	On error.
*------------------------------------------------------------------------*/
bool usfn_GetModemDetails(void)
{
	enum modemDetails_e	{
							AT_CGMM,
							AT_CGMR,
							AT_GSN,
							MODEM_DETAILS_END
						};
	char modemResp[MODEM_UART_BUFFER_SIZE]="";
	uint8_t retryCount = 0;
	uint8_t atCmdState = AT_CGMM;
	char *charPtr = NULL;
	chprintf(g_chp, "usfn_GetModemDetails : Getting Modem details.\r\n");
	for(retryCount = 0; retryCount < MODEM_COMMUNICATION_RETRY_COUNT; retryCount++)
	{
		switch(atCmdState)
		{
			case AT_CGMM	:	usfn_SendAtCommand("AT+CGMM\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
								if(strstr((char*)modemResp,"OK"))
								{
									charPtr = modemResp+2;
									charPtr = strtok((char*)charPtr,"\r");
									strcpy((char*)g_gsmModemDetails_t.cgmm,charPtr);
									atCmdState = AT_CGMR;
									retryCount = 0;
									break;
								}
								chprintf(g_chp,"usfn_GetModemDetails : ERROR - Getting modem model number..!\r\n");
								break;
			case AT_CGMR	:	usfn_SendAtCommand("AT+CGMR\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
								if(strstr((char*)modemResp,"OK"))
								{
									charPtr = modemResp+2;
									charPtr = strtok((char*)charPtr,"\r");
									strcpy((char*)g_gsmModemDetails_t.cmgr,charPtr);
									atCmdState = AT_GSN;
									retryCount = 0;
									break;
								}
								chprintf(g_chp,"usfn_GetModemDetails : ERROR - Getting modem FW version..!\r\n");
								break;
			case AT_GSN	:	usfn_SendAtCommand("AT+GSN\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
								if(strstr((char*)modemResp,"OK"))
								{
									charPtr = modemResp+2;
									charPtr = strtok((char*)charPtr,"\r");
									strcpy((char*)g_gsmModemDetails_t.imeiStr,charPtr);
									atCmdState = MODEM_DETAILS_END;
									retryCount = 0;
									break;
								}
								chprintf(g_chp,"usfn_GetModemDetails : ERROR - Getting IMEI number..!\r\n");
								break;
		}
#if LCR_CONF_MODEM_RESPONSE_ENABLE
		chprintf(g_chp, "usfn_GetModemDetails : AT response - \r\n%s\r\n", modemResp);
		chThdSleepMilliseconds(200);
#endif
		memset(modemResp,0x00,sizeof(modemResp));
		if(atCmdState == MODEM_DETAILS_END)
		{
			//--------- Convert IMEI string to IMEI Hex
			uint64_t	*endptr = NULL, tempStoreImei = 0;
			uint8_t		*bytePtr = NULL, *ptr = NULL;
			tempStoreImei 	= strtoll(g_gsmModemDetails_t.imeiStr,(char**) &endptr, 10);
			ptr		= (uint8_t*)&g_gsmModemDetails_t.imeiHex;
			bytePtr = (uint8_t*)(&tempStoreImei)+7;
			//Storing IMEI no in Big Endian format.
			*(ptr++) = (uint8_t)*(bytePtr--);
			*(ptr++) = (uint8_t)*(bytePtr--);
			*(ptr++) = (uint8_t)*(bytePtr--);
			*(ptr++) = (uint8_t)*(bytePtr--);
			*(ptr++) = (uint8_t)*(bytePtr--);
			*(ptr++) = (uint8_t)*(bytePtr--);
			*(ptr++) = (uint8_t)*(bytePtr--);
			*(ptr++) = (uint8_t)*(bytePtr--);
			return FALSE;
		}
	}
	return TRUE;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_ConfigModemSettings.
*Description: 	Configures modem for band selection, sms mode etc.
*Arguments	: 	None.
*Return	 	: 	FALSE	-	On success.
				TRUE	- 	On error.
*------------------------------------------------------------------------*/
bool usfn_ConfigModemSettings(void)
{
	enum modemDetails_e	{
							AT_KSREP,
							AT_CMGF,
							AT_CMEE,
							AT_CTZR,
							AT_CTZU,
							MODEM_CONFIG_END
						};
	char modemResp[MODEM_UART_BUFFER_SIZE]="";
	uint8_t retryCount = 0;
	uint8_t atCmdState = AT_KSREP;
	chprintf(g_chp, "usfn_ConfigModemSettings : Setting Modem configurations.\r\n");
	for(retryCount = 0; retryCount < MODEM_COMMUNICATION_RETRY_COUNT; retryCount++)
	{
		switch(atCmdState)
		{
			case AT_KSREP	:	usfn_SendAtCommand("AT+KSREP=0\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
								if(strstr((char*)modemResp,"OK"))
								{
									atCmdState = AT_CMGF;
									retryCount = 0;
									break;
								}
								chprintf(g_chp,"usfn_ConfigModemSettings : ERROR - Couldn't disable the Unsolicited result code..!\r\n");
								break;
			case AT_CMGF	:	usfn_SendAtCommand("AT+CMGF=1\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
								if(strstr((char*)modemResp,"OK"))
								{
									atCmdState = AT_CMEE;
									retryCount = 0;
									break;
								}
								chprintf(g_chp,"usfn_ConfigModemSettings : ERROR - Couldn't set the SMS mode..!\r\n");
								break;
			case AT_CMEE	:	usfn_SendAtCommand("AT+CMEE=2\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
								if(strstr((char*)modemResp,"OK"))
								{
									atCmdState = AT_CTZR;
									retryCount = 0;
									break;
								}
								chprintf(g_chp,"usfn_ConfigModemSettings : ERROR - Couldn't enable CMEE ..!\r\n");
								break;
			case AT_CTZR	:	usfn_SendAtCommand("AT+CTZR=0\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
								if(strstr((char*)modemResp,"OK"))
								{
									atCmdState = MODEM_CONFIG_END;
									retryCount = 0;
									break;
								}
								chprintf(g_chp,"usfn_ConfigModemSettings : ERROR - Couldn't disable CTZR ..!\r\n");
								break;
			case AT_CTZU	:	usfn_SendAtCommand("AT+CTZU=0\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
								if(strstr((char*)modemResp,"OK"))
								{
									atCmdState = MODEM_CONFIG_END;
									retryCount = 0;
									break;
								}
								chprintf(g_chp,"usfn_ConfigModemSettings : ERROR - Couldn't disable CTZU ..!\r\n");
								break;
		}
#if LCR_CONF_MODEM_RESPONSE_ENABLE
		chprintf(g_chp, "usfn_ConfigModemSettings : AT response - \r\n%s\r\n", modemResp);
		chThdSleepMilliseconds(200);
#endif
		memset(modemResp,0x00,sizeof(modemResp));
		if(atCmdState == MODEM_CONFIG_END)
		{
			return FALSE;
		}
	}
	return TRUE;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_GetAPNName.
*Description: 	Gets APN name based on the operator name.
				(Only indian networks, this will not work for iot sim cards)
*Arguments	: 	*operatorName 	- A pointer to Operator name.
*Return	 	: 	char*			- APN name.
*------------------------------------------------------------------------*/
char* usfn_GetAPNName(char *operatorName)
{
	strupr(operatorName); // Converts to all characters to the upper case.
	chprintf(g_chp, "usfn_GetAPNName: Operator name received  %s.\r\n",operatorName);

	if( strstr((char*) operatorName , "AIRTEL"))
	{
		return "airtelgprs.com";
	}
	else if((strstr((char*) operatorName , "VODAFONE")) || (strstr((char*) operatorName ,"HUTCH")))
	{
		return "www";
	}
	else if((strstr((char*) operatorName , "IDEA")) || (strstr((char*) operatorName ,"SPICE")))
	{
		return "internet";
	}
	else if((strstr((char*) operatorName , "JIO")))
	{
		return "jionet";
	}
	else
	{
		chprintf(g_chp, "usfn_GetAPNName: Using default operator name.\r\n");
		return "unlimitiot";
	}
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_ModemSimInit.
*Description: 	Initializes SIM card and registers to network.
*Arguments	: 	simSel	- SIM slot
*Return	 	: 	FALSE	-	On success.
				TRUE	- 	On error.
*------------------------------------------------------------------------*/
bool usfn_ModemSimInit(bool simSel)
{
	enum modem_e	{
						AT_KSIMSEL,
						AT_KSRAT,
						AT_CPIN,
						AT_CREG,
						SIM_INIT_END,
					};
	char modemResp[MODEM_UART_BUFFER_SIZE]="";
	uint8_t retryCount = 0;
	uint8_t simDetectionErrorCount = 0;
	uint8_t atCmdState = AT_KSIMSEL;
	chprintf(g_chp, "usfn_ModemSimInit : Sim Initialization.\r\n");
	for(retryCount = 0; retryCount < MODEM_SIM_DETECTION_RETRY_COUNT; retryCount++)
	{
		chprintf(g_chp, "usfn_ModemSimInit : Attempt - %u\r\n", retryCount);
		switch(atCmdState)
		{
			case AT_KSIMSEL	:	if(simSel)
								{
									chprintf(g_chp,"\r\n_____________________________________\r\n");
									chprintf(g_chp, "usfn_ModemSimInit : Selecting SIM2.\r\n");
									chprintf(g_chp,"\r\n_____________________________________\r\n");
									usfn_SendAtCommand("AT+KSIMSEL=2\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
								}
								else
								{
									chprintf(g_chp,"\r\n_____________________________________\r\n");
									chprintf(g_chp, "usfn_ModemSimInit : Selecting SIM1.\r\n");
									chprintf(g_chp,"\r\n_____________________________________\r\n");
									usfn_SendAtCommand("AT+KSIMSEL=1\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
								}
								if(strstr(modemResp,"OK"))
								{
									atCmdState = AT_CPIN;
									retryCount = 0;
									break;
								}
								chprintf(g_chp,"usfn_ModemSimInit : ERROR - Couldn't select the SIM slot..!\r\n");
								break;
			case AT_KSRAT	:
#if LCR_CONF_MODEM_4G_IS_PRESENT
	#if LCR_CONF_MODEM_SET_2G_MODE
		usfn_SendAtCommand("AT+KSRAT=1\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
	#else
		usfn_SendAtCommand("AT+KSRAT=9\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
	#endif

#else
	#if LCR_CONF_MODEM_SET_2G_MODE
		usfn_SendAtCommand("AT+KSRAT=1\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME*6);
	#else
		usfn_SendAtCommand("AT+KSRAT=4\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME*4);
	#endif
#endif
								if(strstr((char*)modemResp,"OK"))
								{
									atCmdState = AT_CPIN;
									retryCount = 0;
									break;
								}
								chprintf(g_chp,"usfn_ModemSimInit : ERROR - Couldn't set the GSM band..!\r\n");
								break;

			case AT_CPIN	:	usfn_SendAtCommand("AT+CPIN?\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
								if(strstr(modemResp,"READY"))
								{
									atCmdState = AT_CREG;
									retryCount = 0;
									break;
								}
								simDetectionErrorCount++;
								chprintf(g_chp,"usfn_ModemSimInit : ERROR - Couldn't Detect the SIM..!\r\n");
								if(simDetectionErrorCount >= MMODEM_SIM_DETECTION_ATTEMPTS)
									return TRUE;
								break;
			case AT_CREG	:	usfn_SendAtCommand("AT+CREG?\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
								if(strstr(modemResp,"OK"))
								{
									if(strstr(modemResp,"0,0"))
									{
										chprintf(g_chp,"usfn_ModemSimInit: SIM is not registered, ME is not currently searching a new operator.\r\n");
										atCmdState = AT_CREG;
									}
									else if(strstr(modemResp,"0,1"))
									{
										chprintf(g_chp,"usfn_ModemSimInit: SIM is registered, home network.\r\n");
										atCmdState = SIM_INIT_END;
									}
									else if(strstr(modemResp,"0,2"))
									{
										chprintf(g_chp,"usfn_ModemSimInit: SIM is not registered, but ME is currently searching a new operator.\r\n");
										atCmdState = AT_CREG;
									}
									else if(strstr(modemResp,"0,3"))
									{
										chprintf(g_chp,"usfn_ModemSimInit: SIM registration denied.\r\n");
										atCmdState = AT_CREG;
									}
									else if(strstr(modemResp,"0,4"))
									{
										chprintf(g_chp,"usfn_ModemSimInit: SIM is not registered unknown.\r\n");
										atCmdState = AT_CREG;
									}
									else if(strstr(modemResp,"0,5"))
									{
										chprintf(g_chp,"usfn_ModemSimInit: SIM is registered, roaming.\r\n");
										atCmdState = SIM_INIT_END;
									}
									break;
								}
								chprintf(g_chp,"usfn_ModemSimInit : ERROR - Couldn't read sim registration..!\r\n");
								break;
		}

#if LCR_CONF_MODEM_RESPONSE_ENABLE
		chprintf(g_chp, "usfn_ModemSimInit : AT response - \r\n%s\r\n", modemResp);
		chThdSleepMilliseconds(200);
#endif
		usfn_CheckSMS();
		memset(modemResp,0x00,MODEM_UART_BUFFER_SIZE);
		if(atCmdState == SIM_INIT_END)
		{
			chprintf(g_chp, "usfn_ModemSimInit : SIM registration End.\r\n");
			return FALSE;
		}
	}
	return TRUE;
}

#if !LCR_CONF_GET_TIME_FROM_OPERATOR
/*------------------------------------------------------------------------
*Function 	: 	usfn_GetTimeAndDate.
*Description: 	Gets date & time from the bsl server.
*Arguments	: 	None.
*Return	 	: 	FALSE	-	On success.
				TRUE	- 	On error.
*------------------------------------------------------------------------*/
bool usfn_GetTimeAndDate(void)
{
	chprintf(g_chp,"-------------------usfn_ConnectToCloud : Connecting to Cloud - %s : %u.-------------------\r\n", g_simCloudDetails_t.timeServerIP, g_simCloudDetails_t.timeServerPort);

	char modemAtCmd[50]	= "";
	char modemResp[MODEM_UART_BUFFER_SIZE]	= "";
	int retryCount 		= 100;
	bool status			= TRUE;

	sprintf(modemAtCmd,"AT+KTCPCNX=%u\r", g_lcrGlobalData.timeServerSession);
	usfn_SendAtCommand(modemAtCmd,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
	while(retryCount)
	{
		usfn_SendAtCommand(NULL,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
		if(strstr(modemResp,"KTCP_DATA"))
		{
			status = FALSE;
			break;
		}
		if(strstr(modemResp,"KTCP_NOTIF:") || strstr(modemResp, "KCNX_IND: 1,4,2"))
		{
			return TRUE;
		}
		chprintf(g_chp,"usfn_GetTimeAndDate : Waiting for response from the time server - %s.\r\n", modemResp);
		memset(modemResp,0,MODEM_UART_BUFFER_SIZE);
		retryCount--;
	}
	memset(modemResp,0,MODEM_UART_BUFFER_SIZE);
	memset(modemAtCmd,0,50);
	if(!status)
	{
		// If valid data is received then process it.
		sprintf(modemAtCmd,"AT+KTCPRCV=%u,51\r", g_lcrGlobalData.timeServerSession);
		usfn_SendAtCommand(modemAtCmd,modemResp,MODEM_CMD_RESPONSE_WAIT_TIME);
		if(strstr(modemResp,"CONNECT"))
		{
			char 		*charPtr		= NULL;
			RTCDateTime rtcDateTimeHal 	= {0};
			struct tm 	rtcStdTime		= {0}, *testRtcStdTime	= NULL;
			uint8_t 	hours = 0, minutes = 0, seconds = 0;
			time_t		unixTime		= 0;

			charPtr 			= strtok((char*)modemResp," ");
			charPtr 			= strtok(NULL,"-");
			rtcDateTimeHal.year = atoi(charPtr) + RTC_NO_OF_YEARS_ADDED_TO_BASE;
			charPtr 			= strtok(NULL,"-");
			rtcDateTimeHal.month= atoi(charPtr);
			charPtr 			= strtok(NULL," ");
			rtcDateTimeHal.day	= atoi(charPtr);
			charPtr 			= strtok(NULL,":");
			hours				= atoi(charPtr);
			charPtr 			= strtok(NULL,":");
			minutes 			= atoi(charPtr);
			charPtr 			= strtok(NULL," ");
			seconds 			= atoi(charPtr);
			rtcDateTimeHal.dayofweek = usfn_RtcDayofweek(rtcDateTimeHal.day,rtcDateTimeHal.month,rtcDateTimeHal.year);
			rtcDateTimeHal.millisecond = ((((hours*60)+minutes)*60)+seconds)*1000;

			rtcConvertDateTimeToStructTm(&rtcDateTimeHal, &rtcStdTime, NULL);
			unixTime = mktime(&rtcStdTime) + RTC_SECONDS_TO_BE_ADDED_FOR_GST;

			testRtcStdTime = localtime_r(&unixTime, &rtcStdTime);
			osalDbgCheck(&rtcStdTime == testRtcStdTime);

			rtcConvertStructTmToDateTime(&rtcStdTime, 0, &rtcDateTimeHal);
			rtcSetTime(&RTCD1, &rtcDateTimeHal);

			rtcGetTime(&RTCD1, &rtcDateTimeHal);
			rtcConvertDateTimeToStructTm(&rtcDateTimeHal, &rtcStdTime, NULL);
			chprintf(g_chp, "\r\nusfn_GetTimeAndDate : Current Date & Time - %s.\r\n", asctime(&rtcStdTime));
			return FALSE;
 		}
	}

	chprintf(g_chp, "usfn_GetTimeAndDate : ERROR - Couldn't get the time.\r\n",modemResp);
	return TRUE;
}

#else //LCR_CONF_GET_TIME_FROM_OPERATOR

/*------------------------------------------------------------------------
*Function 	: 	usfn_GetTimeAndDate.
*Description: 	Gets time from the modem.
*Arguments	: 	None.
*Return	 	: 	FALSE	-	On success.
				TRUE	- 	On error.
*------------------------------------------------------------------------*/
bool usfn_GetTimeAndDate(void)
{
	chprintf(g_chp,"---------------usfn_GetTimeAndDate : Sync with CCLK.----------------\r\n");
	char 		*charPtr		= NULL;
	RTCDateTime rtcDateTimeHal 	= {0};
	struct tm 	rtcStdTime		= {0};
	uint8_t 	hours = 0, minutes = 0, seconds = 0;

	char modemResp[MODEM_UART_BUFFER_SIZE]	= "";
	usfn_SendAtCommand("AT+CCLK?\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);


	charPtr 			= strtok((char*)modemResp,"\"");
	charPtr 			= strtok(NULL,"/");
	rtcDateTimeHal.year = atoi(charPtr) + RTC_NO_OF_YEARS_ADDED_TO_BASE;
	charPtr 			= strtok(NULL,"/");
	rtcDateTimeHal.month= atoi(charPtr);
	charPtr 			= strtok(NULL,",");
	rtcDateTimeHal.day	= atoi(charPtr);
	charPtr 			= strtok(NULL,":");
	hours				= atoi(charPtr);
	charPtr 			= strtok(NULL,":");
	minutes 			= atoi(charPtr);
	charPtr 			= strtok(NULL,"+");
	seconds 			= atoi(charPtr);

	chprintf(g_chp,"usfn_GetTimeAndDate : year : %u, months : %u, seconds : ;%u, hours : %u, minutes : %u, seconds : %u\r\n", rtcDateTimeHal.year,rtcDateTimeHal.month,rtcDateTimeHal.day, hours, minutes, seconds);

	rtcDateTimeHal.dayofweek = usfn_RtcDayofweek(rtcDateTimeHal.day,rtcDateTimeHal.month,rtcDateTimeHal.year);
	rtcDateTimeHal.millisecond = ((((hours*60)+minutes)*60)+seconds)*1000;

	rtcConvertDateTimeToStructTm(&rtcDateTimeHal, &rtcStdTime, NULL);
	//unixTime = mktime(&rtcStdTime) + RTC_SECONDS_TO_BE_ADDED_FOR_GST;

	//testRtcStdTime = localtime_r(&unixTime, &rtcStdTime);
	//osalDbgCheck(&rtcStdTime == testRtcStdTime);

	rtcConvertStructTmToDateTime(&rtcStdTime, 0, &rtcDateTimeHal);
	rtcSetTime(&RTCD1, &rtcDateTimeHal);

	rtcGetTime(&RTCD1, &rtcDateTimeHal);
	rtcConvertDateTimeToStructTm(&rtcDateTimeHal, &rtcStdTime, NULL);
	chprintf(g_chp, "\r\nusfn_GetTimeAndDate : Current Date & Time - %s.\r\n", asctime(&rtcStdTime));
	g_lcrGlobalFlags.dateTimeSet = TRUE;

	return FALSE;
}
#endif


/*------------------------------------------------------------------------
*Function 	: 	usfn_RtcDayofweek.
*Description: 	Calculates day in week.
*Arguments	: 	d	-	date.
				m	-	month.
				y	-	year.
*Return	 	: 	int	-	day in week.
*------------------------------------------------------------------------*/
static int usfn_RtcDayofweek(int d, int m, int y)
{
    static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

    int dow;

    y -= m < 3;
    dow = ( y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    dow++;

    return dow;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_ConnectToCloud.
*Description: 	Creates connection to cloud.
*Arguments	: 	None.
*Return	 	: 	FALSE	-	On success.
				TRUE	- 	On error.
*------------------------------------------------------------------------*/
bool usfn_ConnectToCloud(void)
{
	char modemAtCmd[50]	= "";
	char modemResp[MODEM_UART_BUFFER_SIZE]	= "";
	int retryCount 		= 10;
	chprintf(g_chp,"usfn_ConnectToCloud : Connecting to Cloud - %s : %u.\r\n", g_simCloudDetails_t.cloudServerIP, g_simCloudDetails_t.cloudServerPort);
	sprintf(modemAtCmd,"AT+KTCPCNX=%u\r", g_lcrGlobalData.cloudServerSession);
	usfn_SendAtCommand(modemAtCmd,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);

	while(retryCount)
	{
		usfn_SendAtCommand(NULL,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
		if(strstr(modemResp,"OK"))
		{
			//chprintf(g_chp,"usfn_ConnectToCloud : SUCCESS - Connected to the cloud.\r\n");
			return FALSE;
		}
		retryCount--;
	}
	return TRUE;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_CheckModemStatus.
*Description: 	Checks cloud connection status, any received data.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
modemHealth_e usfn_CheckModemStatus(void)
{
	char modemAtCmd[50]	= "";
	char modemResp[MODEM_UART_BUFFER_SIZE]	= "";
	char 		*charPtr		= NULL;
	uint8_t 	tcpSockState 	= 0;
	int			tcpNotification	= 0;
	uint16_t 	tcpRemData		= 0;
	uint16_t 	tcpRcvData		= 0;

	sprintf(modemAtCmd,"AT+KTCPSTAT=%u\r",g_lcrGlobalData.cloudServerSession);
	usfn_SendAtCommand(modemAtCmd,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
	if((charPtr =strstr(modemResp,"KTCPSTAT")))
	{
		chprintf(g_chp,"usfn_CheckModemStatus : AT Response -> %s.\r\n",modemResp);
		charPtr = strtok(charPtr, ":");
		charPtr = strtok(NULL, ","	);
		tcpSockState = (uint8_t)atoi(charPtr);;
		charPtr = strtok(NULL, ",");
		tcpNotification = atoi(charPtr);
		charPtr = strtok(NULL, ",");
		tcpRemData = (uint16_t)atoi(charPtr);
		charPtr = strtok(NULL, "\r");
		tcpRcvData = (uint16_t)atoi(charPtr);



#if LCR_CONF_TCP_COM_DEBUG_ENABLE
		chprintf(g_chp,"usfn_CheckModemStatus : tcpSockState - %d, tcpNotification - %d, tcpRemData - %u, tcpRcvData - %u.\r\n",
				tcpSockState,tcpNotification,tcpRemData,tcpRcvData);
#endif

		if(0 < tcpRcvData)
		{
			chprintf(g_chp,"usfn_CheckModemStatus : ModBusCofig Command received.\r\n");
			memset(modemAtCmd,0,50);
			uint8_t *tcpRxData = (uint8_t*) chHeapAlloc(NULL,(size_t)tcpRcvData+100);
			if(tcpRxData)
			{
				sprintf(modemAtCmd,"AT+KTCPRCV=%u,%u\r\n",g_lcrGlobalData.cloudServerSession,tcpRcvData);
				usfn_SendAtCommand(modemAtCmd,(char*)tcpRxData, MODEM_CMD_RESPONSE_WAIT_TIME);
				if(strstr((char*)tcpRxData, "CONNECT"))
				{
					usfn_PrintArray(tcpRxData+MODEM_TCP_RXD_MODBUS_CFG_START_INDEX,tcpRcvData);
					if(0 == crcCalculator(tcpRxData+MODEM_TCP_RXD_MODBUS_CFG_START_INDEX, tcpRcvData))
					{
						uint8_t modBusConifigSlot = tcpRxData[MODEM_TCP_RXD_MODBUS_CFG_SLOT_INDEX] - MODEM_TCP_RXD_MODBUS_CFG_SLOT_BASE;
						chprintf(g_chp,"usfn_CheckModemStatus : Received Configuration for the slot %u\r\n", modBusConifigSlot);
						if(modBusConifigSlot < NO_OF_MAX_MODBUS_CONFIGURATION)
						{
							uint16_t modBusConfigLen = (tcpRxData[MODEM_TCP_RXD_MODBUS_CFG_DATA_LENGTH_HIGH_INDEX] << 8)|(tcpRxData[MODEM_TCP_RXD_MODBUS_CFG_DATA_LENGTH_LOW_INDEX]);
							chprintf(g_chp,"usfn_CheckModemStatus : ModbusConfig Len -> %u.\r\n",modBusConfigLen);
							if(RS485_MAX_CONFIG_LENGTH > modBusConfigLen)
							{
								if(usfn_VerifyAndGetDetailsOfModBusConfiguration(tcpRxData+MODEM_TCP_RXD_MODBUS_CFG_ACTUAL_CONFIG_INDEX,modBusConfigLen-2))
								{
									if((tcpRxData[modBusConfigLen+MODEM_TCP_RXD_MODBUS_CFG_ACTUAL_CONFIG_INDEX] == MODEM_TCP_RXD_MODBUS_CFG_END) &&
											(tcpRxData[modBusConfigLen+MODEM_TCP_RXD_MODBUS_CFG_ACTUAL_CONFIG_INDEX-3] == RS485_CONFIG_END))
									{
										char modBusConfigParamName[20] = "";
										int *modBusConfigIntArray =(int*)chHeapAlloc(NULL,(size_t)((sizeof(int))*(modBusConfigLen-2)));
										if(modBusConfigIntArray)
										{
											sprintf(modBusConfigParamName,"%s%u",MODBUS_CFG_NO,modBusConifigSlot);
											for(int i = 0; i< modBusConfigLen-2;i++)
												modBusConfigIntArray[i] =	tcpRxData[MODEM_TCP_RXD_MODBUS_CFG_ACTUAL_CONFIG_INDEX+i];
											usfn_PrintArray(tcpRxData+MODEM_TCP_RXD_MODBUS_CFG_ACTUAL_CONFIG_INDEX,modBusConfigLen);
											usfn_SdCardEditLcrCfgFile(modBusConfigParamName,modBusConfigIntArray,modBusConfigLen-2,CONFIG_FILE_EDIT_ARRAY);
											g_lcrGlobalData.modBusConfigAck = (uint16_t)((tcpRxData[modBusConfigLen+MODEM_TCP_RXD_MODBUS_CFG_ACTUAL_CONFIG_INDEX-2] << 8) |
																				tcpRxData[modBusConfigLen+MODEM_TCP_RXD_MODBUS_CFG_ACTUAL_CONFIG_INDEX-1]);
											return MODEM_STATUS_DEVICE_RESTART;
										}
										else
											chprintf(g_chp,"usfn_CheckModemStatus : ERROR - Couldn't convert into int data.\r\n");
									}
									else
										chprintf(g_chp,"usfn_CheckModemStatus : ERROR - Invalid Modbus Configuration2.\r\n");
								}
								else
									chprintf(g_chp,"usfn_CheckModemStatus : ERROR - Invalid Modbus Configuration1.\r\n");
							}
							else
								chprintf(g_chp,"usfn_CheckModemStatus : ERROR - ModBus Config size is too large.\r\n");
						}
					}
					else
						chprintf(g_chp,"usfn_CheckModemStatus : ERROR - Invalid Crc in the rceived modbus config.\r\n");
				}
				else
					chprintf(g_chp,"usfn_CheckModemStatus : ERROR - Couldn't read the TCP modbus config.\r\n");
				chHeapFree((void*)tcpRxData);
			}
			else
				chprintf(g_chp,"usfn_CheckModemStatus : ERROR - Couldn't allocate memory to download modbus config.\r\n");
		}

		if((MODEM_TCP_SOCKET_STATUS_CLOSED == tcpSockState)&&(tcpNotification != -1))
		{
			chprintf(g_chp,"usfn_CheckModemStatus : ERROR - Socket is not connected to the server.\r\n");
			return MODEM_STATUS_TCP_ERROR;
		}

		if(MODEM_TCP_SOCKET_STATUS_CONNECTING == tcpSockState)
		{
			if(g_lcrAlarmStructure.errCloud)
			{
				return MODEM_STATUS_TCP_READY;
			}

			return MODEM_STATUS_TCP_CPNNECTED;
		}
	}
	else
	{
		chprintf(g_chp,"usfn_CheckModemStatus : ERROR -Couldn't get the status.\r\n");
	}
	return MODEM_STATUS_INVALID;
}
