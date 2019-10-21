/*
 * eventManager.c
 *
 *  Created on: Jun 30, 2019
 *      Author: Arjun Dongre
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "crc16.h"
#include "digitalIO.h"
#include "eventManager.h"
#include "global.h"
#include "lcrConfig.h"
#include "modem.h"
#include "rs485Communication.h"
#include "sdCard.h"
#include "sms.h"
#include "tcpDataProcess.h"
#include "uartModem.h"

//Global variables.
static thread_t			*g_rs485CommunicationThd; 	//Thread handler for rs485 communication.
static thread_t			*g_eventManagerThd;			//Thread handler for event manager.
static thread_t			*g_storeAndForwardThd;			//Thread handler for event manager.
static lcrState			g_lcrState = LCR_STATE_INVALID;	//Status of LCR.
static lcrEvent 		g_lcrEvent = LCR_EVENT_INVALID;	//Event received.
static msg_t 			eventManagerMsgsQueue[EVENT_MANAGER_QUEUE_LEN]; //Array to store received messages(LCR events).
static mutex_t 			g_mutex_lcrStateMachine = {0};	//Lock to access stateMachine status.
static virtual_timer_t 	g_periTimer;	//Timer to send data to cloud.
static virtual_timer_t 	g_modemStatusCheckTimer;	//Timer to check modem status.
static virtual_timer_t 	g_hearBeatTimer;	//Timer to send heartbeat to cloud
static virtual_timer_t 	g_alarmTimer;	//Timer  to send alarm to cloud.
static virtual_timer_t 	g_storeAndForwardTimer;	// Timer to send backed up data.
static virtual_timer_t 	g_restartDeviceTimer;	// Timer to send restart the device.
static virtual_timer_t 	g_serverTimeReSynchTimer;	// Timer to send restart the device.
virtual_timer_t 		g_sendTcpBackUpDataTimer;	//Timer handle to store data when device is offline.


mailbox_t 				eventManagerMailBox; //Descriptor for mailbox.
static modemRetryCount_t g_modemRetryCount_t;
//Thread working area stack initializer.
static THD_WORKING_AREA(rs485CommunicationThdWorkigArea, RS485_COMMUNICATION_HANDLE_STACK_SIZE);
static THD_WORKING_AREA(storeAndForwardThdWorkigArea, ST_AND_FW_HANDLE_STACK_SIZE);
static THD_WORKING_AREA(eventManagerThdWorkigArea, EVENT_MANAGER_HANDLE_STACK_SIZE);
#if LCR_CONF_DIGITAL_IO_BOARD_CONNECTED
static thread_t			*g_dioBoardThd;				//Thread handler for digital IO board.
static THD_WORKING_AREA(dioBoardThdWorkigArea, DIGITAL_IO_BOARD_HANDLE_STACK_SIZE);
#endif


/*------------------------------------------------------------------------
*Function 	: 	usfn_ChangeLcrState
*Description: 	Changes LCR's state in the state machine.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
void usfn_ChangeLcrState(lcrState state)
{
    chMtxLock(&g_mutex_lcrStateMachine);
    g_lcrState = state;
    chMtxUnlock(&g_mutex_lcrStateMachine);
}


/*------------------------------------------------------------------------
*Function 	: 	g_periTimerCallBack
*Description: 	Callback for period timer.
*Arguments	: 	*arg	- Arguments to the callback(Not used).
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void g_periTimerCallBack(void *arg)
{
	(void)arg;
	chSysLockFromISR();
	chVTSetI(&g_periTimer, TIME_S2I(g_simCloudDetails_t.periTime), g_periTimerCallBack, NULL);
	g_lcrEvent = LCR_EVENT_PERI_TIME;
	chMBPostI(&eventManagerMailBox, (msg_t) &g_lcrEvent);
	chSysUnlockFromISR();
}


/*------------------------------------------------------------------------
*Function 	: 	g_modemStatusCheckTimerCallBack
*Description: 	Callbkack to test modem's status.
*Arguments	: 	*arg	- Arguments to the callback(Not used).
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void g_modemStatusCheckTimerCallBack(void *arg)
{
	(void)arg;
	chSysLockFromISR();
	chVTSetI(&g_modemStatusCheckTimer, TIME_S2I(MODEM_STATUS_CHECK_TIME), g_modemStatusCheckTimerCallBack, NULL);
	g_lcrEvent = LCR_EVENT_MODEM_STATUS_CHECK;
	chMBPostI(&eventManagerMailBox, (msg_t) &g_lcrEvent);
	chSysUnlockFromISR();
}


/*------------------------------------------------------------------------
*Function 	: 	g_hearBeatTimerCallBack
*Description: 	Callbkack to send heartbeat to server.
*Arguments	: 	*arg	- Arguments to the callback(Not used).
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void g_hearBeatTimerCallBack(void *arg)
{
	(void)arg;
	chSysLockFromISR();
	chVTSetI(&g_hearBeatTimer, TIME_S2I(TCP_DATA_PERIOD_HEART_BEAT), g_hearBeatTimerCallBack, NULL);
	g_lcrEvent = LCR_EVENT_UPDATE_HEART_BEAT;
	chMBPostI(&eventManagerMailBox, (msg_t) &g_lcrEvent);
	chSysUnlockFromISR();
}


/*------------------------------------------------------------------------
*Function 	: 	g_alarmTimerCallBack
*Description: 	Callbkack to send alarm to server.
*Arguments	: 	*arg	- Arguments to the callback(Not used).
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void g_alarmTimerCallBack(void *arg)
{
	(void)arg;
	chSysLockFromISR();
	chVTSetI(&g_alarmTimer, TIME_S2I(TCP_DATA_PERIOD_ALARM), g_alarmTimerCallBack, NULL);
	g_lcrEvent = LCR_EVENT_UPDATE_ALARM;
	chMBPostI(&eventManagerMailBox, (msg_t) &g_lcrEvent);
	chSysUnlockFromISR();
}


/*------------------------------------------------------------------------
*Function 	: 	g_storeAndForwardTimerCallBack
*Description: 	Callbkack to store data when device is offline.
*Arguments	: 	*arg	- Arguments to the callback(Not used).
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void g_storeAndForwardTimerCallBack(void *arg)
{
	(void)arg;
	chSysLockFromISR();
	g_storeAndForwardEvent = ST_AND_FW_EVENT;
	chMBPostI(&g_mailBoxStoreAndForward, (msg_t) &g_storeAndForwardEvent);
	chVTSetI(&g_storeAndForwardTimer, TIME_S2I(g_simCloudDetails_t.periTime), g_storeAndForwardTimerCallBack, NULL);
	chSysUnlockFromISR();
}


/*------------------------------------------------------------------------
*Function 	: 	g_sendTcpBackUpDataTimerCallBack
*Description: 	Callbkack to send stored data when device gets online.
*Arguments	: 	*arg	- Arguments to the callback(Not used).
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void g_sendTcpBackUpDataTimerCallBack(void *arg)
{
	(void)arg;
	chSysLockFromISR();
	chVTSetI(&g_sendTcpBackUpDataTimer, TIME_S2I(TCP_BACKUP_DATA_SEND_PERIOD), g_sendTcpBackUpDataTimerCallBack, NULL);
	g_lcrEvent = LCR_EVENT_SEND_STORE_AND_FORWARD;
	chMBPostI(&eventManagerMailBox, (msg_t) &g_lcrEvent);
	chSysUnlockFromISR();
}


/*------------------------------------------------------------------------
*Function 	: 	g_restartDeviceTimerCallBack
*Description: 	Callbkack to restart the device.
*Arguments	: 	*arg	- Arguments to the callback(Not used).
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void g_restartDeviceTimerCallBack(void *arg)
{
	(void)arg;
	chSysLockFromISR();
	g_lcrEvent = LCR_EVENT_RESTART;
	chMBPostI(&eventManagerMailBox, (msg_t) &g_lcrEvent);
	chSysUnlockFromISR();
}

/*------------------------------------------------------------------------
*Function 	: 	g_serverTimeReSynchTimerCallBack
*Description: 	Callbkack to resynch the system time.
*Arguments	: 	*arg	- Arguments to the callback(Not used).
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void g_serverTimeReSynchTimerCallBack(void *arg)
{
	(void)arg;
	chSysLockFromISR();
	g_lcrEvent = LCR_EVENT_RESYNCH_DEVICE_TIME;
	chVTSetI(&g_serverTimeReSynchTimer, TIME_S2I(LCR_TIME_RESYNCH_PERIOD), g_serverTimeReSynchTimerCallBack, NULL);
	chMBPostI(&eventManagerMailBox, (msg_t) &g_lcrEvent);
	chSysUnlockFromISR();
}


/*------------------------------------------------------------------------
*Function 	: 	noAction
*Description: 	No action is done.
*Arguments	: 	*arg	- Arguments to the callback(Not used).
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void noAction(void)
{
	//Do Nothing.
	chprintf(g_chp,"\r\n__________________\r\n");
	chprintf(g_chp,"\r\n    NO ACTION..!");
	chprintf(g_chp,"\r\n__________________\r\n");
}


/*------------------------------------------------------------------------
*Function 	: 	action00
*Description: 	Initialized timer objects, starts threads.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action00(void)
{
	chprintf(g_chp,"Action00 : Initializing and starting the threads.\r\n");
	g_lcrEvent = LCR_EVENT_INVALID;

	//Create Timer Objects.
	chVTObjectInit(&g_periTimer);
	chVTObjectInit(&g_modemStatusCheckTimer);
	chVTObjectInit(&g_alarmTimer);
	chVTObjectInit(&g_hearBeatTimer);
	chVTObjectInit(&g_storeAndForwardTimer);
	chVTObjectInit(&g_sendTcpBackUpDataTimer);
	chVTObjectInit(&g_restartDeviceTimer);
	chVTSet(&g_restartDeviceTimer, TIME_S2I(LCR_RESTART_PERIOD), g_restartDeviceTimerCallBack, NULL);
	chVTObjectInit(&g_serverTimeReSynchTimer);

	SERVER_LED_OFF();
	GPRS_LED_OFF();

	if(usfn_SdCardCheckFileExistance(TCP_DATA_BACKUP_FILE_NAME))
	{
		chprintf(g_chp,"Action00 : TCP Backup File Exists.\r\n");
		g_lcrGlobalFlags.tcpBackupDataExists = TRUE;
	}

	if(g_lcrGlobalData.noOfModbusConfigs)
	{
		for(int i=0; i<g_lcrGlobalData.noOfModbusConfigs; i++)
		{
			g_modbusConfigMap[i].modBusErrorCount = 10;
		}
		if((g_rs485CommunicationThd = chThdCreateStatic(rs485CommunicationThdWorkigArea, sizeof(rs485CommunicationThdWorkigArea), RS485_COMMUNICATION_HANDLE_PRIORITY, rs485CommunicationHandle, NULL)))
		{
			if((g_storeAndForwardThd = chThdCreateStatic(storeAndForwardThdWorkigArea, sizeof(storeAndForwardThdWorkigArea), ST_AND_FW_HANDLE_PRIORITY, storeAndForwardHandle, NULL)))
			{
#if LCR_CONF_DIGITAL_IO_BOARD_CONNECTED
				if((g_dioBoardThd = chThdCreateStatic(dioBoardThdWorkigArea, sizeof(dioBoardThdWorkigArea), DIGITAL_IO_BOARD_HANDLE_PRIORITY, digitalIOBoardHandle, NULL)))
				{

				}
				else
				{
					chprintf(g_chp,"Action00 : ERROR - while crating thread for digital IO board.\r\n");
					chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
					return;
				}
#endif //LCR_CONF_DIGITAL_IO_BOARD_CONNECTED
			}
			else
			{
				chprintf(g_chp,"Action00 : ERROR - while crating thread for store & forward.\r\n");
				chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
				return;
			}
		}
		else
		{
			chprintf(g_chp,"Action00 : ERROR - while crating thread for RS485 communication.\r\n");
			chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
			return;
		}
	}
	usfn_ChangeLcrState(LCR_STATE_SIM_INITIALIZATION);
	g_lcrEvent = LCR_EVENT_MODEM_INITIALIZATION_DONE;
	SELECT_SIM1(); //Initially selects sim slot 1.
	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);


}


/*------------------------------------------------------------------------
*Function 	: 	action01.
*Description: 	Initialized SIM card.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action01(void)
{
	chprintf(g_chp,"Action01 : Entry.\r\n");

	g_lcrEvent = LCR_EVENT_INVALID;
	usfn_TurnONModem();
	//Check communication after modem restart.
	if(FALSE == usfn_CheckModemCommunication())
	{
		//Configure modem.
		if(FALSE == usfn_ConfigModemSettings())
		{
			//Initialize the SIM.
			if(usfn_ModemSimInit(g_lcrGlobalFlags.simSelect))
			{
				chprintf(g_chp,"Action01 : ERROR - Couldn't initialize the SIM1.\r\n");
				g_lcrEvent = LCR_EVENT_SIM_ERROR;
			}
			else
			{
				chprintf(g_chp,"Action01 : SUCCESS - Initialized the SIM1.\r\n");
				g_lcrEvent = LCR_EVENT_SIM_READY;
			}
		}
		else
		{
			g_lcrEvent = LCR_EVENT_INVALID;
			chprintf(g_chp,"Action01 : ERROR - Couldn't achieve modem settings.\r\n");
		}
	}
	else
	{
		g_lcrEvent = LCR_EVENT_INVALID;
		chprintf(g_chp,"Action01 : ERROR - Couldn't communicate with the modem.\r\n");
	}

	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
}


/*------------------------------------------------------------------------
*Function 	: 	action02.
*Description: 	Initialized GPRS.
				FTP configurtion is not used as there is no DOTA.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action02(void)
{
	chprintf(g_chp,"Action02 : Entry.\r\n");
	g_lcrEvent = LCR_EVENT_GPRS_ERROR;
	enum modem_e	{
						AT_COPS,
						AT_CGATT_0,
						AT_CGDCONT,
						AT_CGCLASS,
						AT_CGACT_1_1,
						AT_CGATT_TEST,
						AT_CGDCONT_TEST_IP,
						AT_CGREG,
						AT_KCNXCFG,
						AT_KTCPCFG_TIME,
						AT_KTCPCFG_CLOUD,
						AT_KFTPCFG,
#if! LCR_CONF_MODEM_4G_IS_PRESENT
//If 3G modemm is connected, then modem needs to be set in filght mode and then exit.
						AT_CFUN_4_0,
						AT_CFUN_1_0,
#endif
						GPRS_INIT_END
					};
		char modemResp[MODEM_UART_BUFFER_SIZE]="";
		char modemATCmd[MODEM_UART_BUFFER_SIZE]="";
		char simApnName[20]="";
		uint8_t retryCount = 0;
		uint8_t atCmdState = AT_COPS;
		char *charPtr = NULL;
		chprintf(g_chp, "Action02 : GPRS Initialization.\r\n");
		for(retryCount = 0; retryCount < MODEM_COMMUNICATION_RETRY_COUNT; retryCount++)
		{
			switch(atCmdState)
			{
				case AT_COPS		:	if(g_lcrGlobalFlags.simSelect == SIM1)
										{
											//if there is apn then go to cgdcont to set apn name.
											if(g_lcrGlobalFlags.apnName1Exists)
											{
												strcpy(simApnName,g_simCloudDetails_t.apnName1);
												atCmdState = AT_CGATT_0;
												chprintf(g_chp, "Action02 : Using stored APN name - \"%s\".\r\n",g_simCloudDetails_t.apnName1);
												break;
											}
										}
										else
										{
											//if there is apn then go to cgdcont to set apn name.
											if(g_lcrGlobalFlags.apnName2Exists)
											{
												strcpy(simApnName,g_simCloudDetails_t.apnName2);
												atCmdState = AT_CGATT_0;
												chprintf(g_chp, "Action02 : Using stored APN name - \"%s\".\r\n",g_simCloudDetails_t.apnName2);
												break;
											}
										}
										// Getting operator name manuaaly.
										usfn_SendAtCommand("AT+COPS?\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
										if(strstr(modemResp,"+COPS: 0,0,\""))
										{
											strtok(modemResp,"\"");
											charPtr	= strtok(NULL,"\"");
											strcpy(g_simCloudDetails_t.apnName1, usfn_GetAPNName(charPtr));
											chprintf(g_chp, "Action02 : APN name for %s -> \"%s\"\r\n", charPtr, g_simCloudDetails_t.apnName1);
											atCmdState 	= AT_CGATT_0;
											retryCount = 0;
											break;
										}
										chprintf(g_chp,"Action02 : ERROR - Couldn't get the operator name..!\r\n");
										break;
				case AT_CGATT_0		:	usfn_SendAtCommand("AT+CGATT=0\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME*4);
										if(strstr(modemResp,"OK") || strstr(modemResp,"ERROR"))
										{
											atCmdState = AT_CGDCONT;
											retryCount = 0;
											break;
										}
										chprintf(g_chp,"Action02 : ERROR - Couldn't deactivate the GPRS..!\r\n");
										break;
				case AT_CGDCONT		:	sprintf(modemATCmd,"AT+CGDCONT=1,\"IP\",\"%s\"\r", g_simCloudDetails_t.apnName1);
										usfn_SendAtCommand(modemATCmd,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
										if(strstr(modemResp,"OK"))
										{
											atCmdState = AT_CGCLASS;
											retryCount = 0;
											break;
										}
										chprintf(g_chp,"Action02 : ERROR - Couldn't Set the apn name..!\r\n");
										break;
				case AT_CGCLASS		:	usfn_SendAtCommand("AT+CGCLASS=\"B\"\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME*4);
										if(strstr(modemResp,"OK"))
										{
											atCmdState = AT_CGACT_1_1;
											retryCount = 0;
											break;
										}
										chprintf(g_chp,"Action02 : ERROR - Couldn't set the GPRS Class..!\r\n");
										break;
				case AT_CGACT_1_1	:	usfn_SendAtCommand("AT+CGACT=1,1\r",modemResp, MODEM_GPRS_CONNECTION_TIME_OUT);
										if(strstr(modemResp,"OK"))
										{
											atCmdState = AT_CGATT_TEST;
											retryCount = 0;
											break;
										}

										usfn_SendAtCommand("AT+CGCLASS?\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);

										chprintf(g_chp,"Action02 : ERROR - Couldn't activate the GPRS..!\r\n%s\r\n", modemResp);
										break;
				case AT_CGATT_TEST	:	usfn_SendAtCommand("AT+CGATT?\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
										if((strstr(modemResp,"OK")) && (strstr(modemResp,"1")))
										{
											atCmdState = AT_CGDCONT_TEST_IP;
											retryCount = 0;
											break;
										}
										chprintf(g_chp,"Action02 : ERROR - GPRS is not attached..!\r\n");
										break;
				case AT_CGDCONT_TEST_IP	:usfn_SendAtCommand("AT+CGDCONT?\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
										if(!((strstr(modemResp,"OK")) && (strstr(modemResp,"\"0.0.0.0\""))))
										{
											atCmdState = AT_CGREG;
											retryCount = 0;
											break;
										}
										chprintf(g_chp,"Action02 : ERROR - Modem didn't receive the IP address..!\r\n");
										break;
				case AT_CGREG		:	usfn_SendAtCommand("AT+CGREG?\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
										if(strstr(modemResp,"OK"))
										{
											if(strstr(modemResp,"0,0"))
											{
												chprintf(g_chp,"Action02: GPRS is not registered, ME is not currently searching a new operator.\r\n");
												atCmdState = AT_CGREG;
											}
											else if(strstr(modemResp,"0,1"))
											{
												chprintf(g_chp,"Action02: GPRS is registered, home network.\r\n");
												atCmdState = AT_KCNXCFG;
											}
											else if(strstr(modemResp,"0,2"))
											{
												chprintf(g_chp,"Action02: GPRS is not registered, but ME is currently searching a new operator.\r\n");
												atCmdState = AT_CGREG;
											}
											else if(strstr(modemResp,"0,3"))
											{
												chprintf(g_chp,"Action02: GPRS registration denied.\r\n");
												atCmdState = AT_CGREG;
											}
											else if(strstr(modemResp,"0,4"))
											{
												chprintf(g_chp,"Action02: GPRS is not registered unknown.\r\n");
												atCmdState = AT_CGREG;
											}
											else if(strstr(modemResp,"0,5"))
											{
												chprintf(g_chp,"Action02: GPRS is registered, roaming.\r\n");
												atCmdState = AT_KCNXCFG;
											}
											break;
										}
										chprintf(g_chp,"Action02 : ERROR - Couldn't register the GPRS..!\r\n");
										break;
				case AT_KCNXCFG		:	sprintf(modemATCmd,"AT+KCNXCFG=1,\"GPRS\",\"%s\"\r", g_simCloudDetails_t.apnName1);
										usfn_SendAtCommand(modemATCmd,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
										if(strstr(modemResp,"OK"))
										{
											atCmdState = AT_KTCPCFG_TIME;
											retryCount = 0;
											break;
										}
										chprintf(g_chp,"Action02 : ERROR - Couldn't configure the GPRS..!\r\n");
										break;
				case AT_KTCPCFG_TIME:	sprintf(modemATCmd,"AT+KTCPDEL=%d\r", g_lcrGlobalData.timeServerSession);
										usfn_SendAtCommand(modemATCmd,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
										if((strstr(modemResp,"OK")) || (strstr(modemResp,"ERROR")))
										{
											sprintf(modemATCmd,"AT+KTCPCFG=1,0,\"%s\",%d\r", g_simCloudDetails_t.timeServerIP,g_simCloudDetails_t.timeServerPort);
											usfn_SendAtCommand(modemATCmd,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
											if(strstr(modemResp,"OK"))
											{
												strtok(modemResp," ");
												charPtr	= strtok(NULL,"\r");
												g_lcrGlobalData.timeServerSession = (uint8_t)atoi(charPtr);
												chprintf(g_chp,"Action02 : TIME Server session ID - %d.\r\n",g_lcrGlobalData.timeServerSession);
												atCmdState = AT_KTCPCFG_CLOUD;
												retryCount = 0;
												break;
											}
										}
										chprintf(g_chp,"Action02 : ERROR - Couldn't configure the TIME Server..!\r\n");
										break;
				case AT_KTCPCFG_CLOUD:	sprintf(modemATCmd,"AT+KTCPDEL=%d\r", g_lcrGlobalData.cloudServerSession);
										usfn_SendAtCommand(modemATCmd,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
										if((strstr(modemResp,"OK")) || (strstr(modemResp,"ERROR")))
										{
											sprintf(modemATCmd,"AT+KTCPCFG=1,0,\"%s\",%d\r", g_simCloudDetails_t.cloudServerIP,g_simCloudDetails_t.cloudServerPort);
											usfn_SendAtCommand(modemATCmd,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
											if(strstr(modemResp,"OK"))
											{
												strtok(modemResp," ");
												charPtr	= strtok(NULL,"\r");
												g_lcrGlobalData.cloudServerSession = (uint8_t)atoi(charPtr);
												chprintf(g_chp,"Action02 : Cloud Server session ID - %d.\r\n",g_lcrGlobalData.cloudServerSession);
#if! LCR_CONF_MODEM_4G_IS_PRESENT
												atCmdState = AT_CFUN_4_0;
#else
												atCmdState = GPRS_INIT_END;
												retryCount = 0;
#endif
												break;
											}
										}
										chprintf(g_chp,"Action02 : ERROR - Couldn't configure the Cloud Server..!\r\n");
										break;
				case AT_KFTPCFG:		sprintf(modemATCmd,"AT+ KFTPCFGDEL=%d\r", g_lcrGlobalData.ftpServerSession);
										usfn_SendAtCommand(modemATCmd,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
										if((strstr(modemResp,"OK")) || (strstr(modemResp,"ERROR")))
										{
											sprintf(modemATCmd,"AT+KFTPCFG=1,\"%s\",\"%s\",\"%s\",21,0\r\r", g_simCloudDetails_t.ftpIP,g_simCloudDetails_t.ftpUserName,g_simCloudDetails_t.ftpPassword);
											usfn_SendAtCommand(modemATCmd,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
											if(strstr(modemResp,"OK"))
											{
												strtok(modemResp," ");
												charPtr	= strtok(NULL,"\r");
												g_lcrGlobalData.ftpServerSession = (uint8_t)atoi(charPtr);
												chprintf(g_chp,"Action02 : FTP Server session ID - %d.\r\n",g_lcrGlobalData.ftpServerSession);
												retryCount = 0;
												break;
											}
										}
										chprintf(g_chp,"Action02 : ERROR - Couldn't configure the FTP Server..!\r\n");
										break;
#if! LCR_CONF_MODEM_4G_IS_PRESENT
				case AT_CFUN_4_0	:	usfn_SendAtCommand("AT+CFUN=4,0\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME*8);
										chprintf(g_chp,"Action02 : Entering Filght mode.\r\n");
										if(strstr(modemResp,"OK"))
										{
											atCmdState = AT_CFUN_1_0;
											retryCount = 0;
											break;
										}
										chprintf(g_chp,"Action02 : ERROR - Couldn't enter flight mode..!\r\n");
										break;
				case AT_CFUN_1_0	:	usfn_SendAtCommand("AT+CFUN=1,0\r",modemResp, MODEM_CMD_RESPONSE_WAIT_TIME*6);
										chprintf(g_chp,"Action02 : Exiting Filght mode.\r\n");
										if(strstr(modemResp,"OK"))
										{
											atCmdState = GPRS_INIT_END;
											retryCount = 0;
											break;
										}
										chprintf(g_chp,"Action02 : ERROR - Couldn't exit flight mode..!\r\n");
										break;
#endif //LCR_CONF_MODEM_4G_IS_PRESENT
			}
#if LCR_CONF_MODEM_RESPONSE_ENABLE
			chprintf(g_chp, "Action02 : AT response - \r\n%s\r\n", modemResp);
			chThdSleepMilliseconds(200);
#endif
			usfn_CheckSMS();
			memset(modemATCmd,0x00,sizeof(modemATCmd));
			memset(modemResp,0x00,sizeof(modemResp));
			charPtr = NULL;
			if(atCmdState == GPRS_INIT_END)
			{
				g_lcrEvent = LCR_EVENT_GPRS_READY;
				chprintf(g_chp, "Action02 : GPRS registration End\r\n");
				chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
				return;
			}
		}
	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
}


/*------------------------------------------------------------------------
*Function 	: 	action03.
*Description: 	In case of SIM error. SIM switch is done.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action03(void)
{
	chprintf(g_chp,"Action03 : Entry.\r\n");
	g_lcrEvent = LCR_EVENT_SIM_ERROR;

	if(g_lcrGlobalFlags.simSelect)
	{
		SELECT_SIM1();
		g_lcrEvent = LCR_EVENT_MODEM_INITIALIZATION_DONE;
		chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
		return;
	}
	//Select SIM slot 2.
	SELECT_SIM2();
	usfn_TurnONModem();
	//Check communication after modem restart.
	if(FALSE == usfn_CheckModemCommunication())
	{
		//Configure the modem.
		if(FALSE == usfn_ConfigModemSettings())
		{
			if(usfn_ModemSimInit(g_lcrGlobalFlags.simSelect))
			{
				chprintf(g_chp,"Action03 : ERROR - Couldn't initialize the SIM2.\r\n");
				SELECT_SIM1();
				g_lcrEvent = LCR_EVENT_MODEM_INITIALIZATION_DONE;
			}
			else
			{
				chprintf(g_chp,"Action03 : SUCCESS - Initialized the SIM2.\r\n");
				g_lcrEvent = LCR_EVENT_SIM_READY;
			}
		}
	}
	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);

}


/*------------------------------------------------------------------------
*Function 	: 	action04.
*Description: 	Gets time from the timer server.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action04(void)
{
	chprintf(g_chp,"Action04 : Entry.\r\n");
	g_lcrEvent = LCR_EVENT_TIME_DATE_SET_ERROR;
	if(FALSE == usfn_GetTimeAndDate())
	{
		g_lcrEvent = LCR_EVENT_TIME_DATE_SET;
		g_lcrGlobalFlags.dateTimeSet = TRUE;
	}
	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);

}


/*------------------------------------------------------------------------
*Function 	: 	action05.
*Description: 	Re-initiates GPRS connection. If retry counts exeeded then moves to
				another SIM slot.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action05(void)
{
	chprintf(g_chp,"Action05 : Entry.\r\n");
	g_lcrEvent = LCR_EVENT_GPRS_READY;
	char modemAtCmd[50]	= "";
	char modemResp[MODEM_UART_BUFFER_SIZE]	= "";

	g_modemRetryCount_t.serverConnect++;

	sprintf(modemAtCmd,"AT+KTCPCLOSE=%u,%u\r", g_lcrGlobalData.timeServerSession,1);
	usfn_SendAtCommand(modemAtCmd,modemResp, MODEM_CMD_RESPONSE_WAIT_TIME);
	chprintf(g_chp,"action05 : Timer TCP close - %s\r\n", modemResp);

	if(g_modemRetryCount_t.serverConnect >= MODEM_CONNECTING_SERVER_RETRY_COUNT)
	{
		g_modemRetryCount_t.serverConnect = 0;
		usfn_ChangeLcrState(LCR_STATE_GPRS_INITIALIZATION);
		g_lcrEvent = LCR_EVENT_GPRS_ERROR;
	}
	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
}


/*------------------------------------------------------------------------
*Function 	: 	action06.
*Description: 	Mankes a connection to cloud.(This doesn't mean connection
				is accomplished)
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action06(void)
{
	chprintf(g_chp,"Action06 : Entry.\r\n");
	g_lcrEvent = LCR_EVENT_CLOUD_ERROR;

	if(FALSE == usfn_ConnectToCloud())
	{
		//Start modem status check timer.
		if(FALSE == chVTIsArmed(&g_modemStatusCheckTimer))
			chVTSet(&g_modemStatusCheckTimer, TIME_S2I(MODEM_STATUS_CHECK_TIME), g_modemStatusCheckTimerCallBack, NULL);

		//Start tcp backup timer. This runs until device changes to CLOUD_CONNECTED state.
		if(FALSE == chVTIsArmed(&g_storeAndForwardTimer))
		{
			chprintf(g_chp,"Action06 : Starting Store&Forward timer.\r\n");
			chVTSet(&g_storeAndForwardTimer, TIME_S2I(g_simCloudDetails_t.periTime), g_storeAndForwardTimerCallBack, NULL);
		}
	}
	else
	{
		chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
	}
}


/*------------------------------------------------------------------------
*Function 	: 	action07.
*Description: 	Send power up message when device conects to server for the
				first time.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action07(void)
{
	chprintf(g_chp,"Action07 : Entry.\r\n");
	if(FALSE == g_lcrGlobalFlags.lcrPowerOn)
	{
		g_lcrGlobalFlags.lcrPowerOn = TRUE;
		g_lcrEvent = LCR_EVENT_UPDATE_POWER_UP_MSG;
		chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
	}
}


/*------------------------------------------------------------------------
*Function 	: 	action08.
*Description: 	Sends regular packet to the server.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action08(void)
{
	chprintf(g_chp,"Action08 : Entry.\r\n");
	chVTReset(&g_modemStatusCheckTimer);
	if(usfn_CollectDataAndPushToServer(SEND_TCP_DATA_REGULAR_FLAG))
	{
		g_lcrEvent = LCR_EVENT_CLOUD_ERROR;
		chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
	}
	chVTSet(&g_modemStatusCheckTimer, TIME_S2I(MODEM_STATUS_CHECK_TIME), g_modemStatusCheckTimerCallBack, NULL);
}


/*------------------------------------------------------------------------
*Function 	: 	action09.
*Description: 	Sends heart beat to server.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action09(void)
{
	chprintf(g_chp,"Action09 : Entry.\r\n");
	chVTReset(&g_modemStatusCheckTimer);
	if(g_lcrGlobalData.noOfModbusConfigs)
	{
		if(usfn_CollectDataAndPushToServer(SEND_TCP_DATA_HEART_BEAT_FLAG))
		{
			g_lcrEvent = LCR_EVENT_CLOUD_ERROR;
			chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
			return;
		}
	}
	chVTSet(&g_modemStatusCheckTimer, TIME_S2I(MODEM_STATUS_CHECK_TIME), g_modemStatusCheckTimerCallBack, NULL);
}


/*------------------------------------------------------------------------
*Function 	: 	action10.
*Description: 	Sends alarm data to server.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action10(void)
{
	chprintf(g_chp,"Action10 : Entry.\r\n");
	chVTReset(&g_modemStatusCheckTimer);
	if(usfn_CollectDataAndPushToServer(SEND_TCP_DATA_ALARM_FLAG))
	{
		g_lcrEvent = LCR_EVENT_CLOUD_ERROR;
		chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
	}
	chVTSet(&g_modemStatusCheckTimer, TIME_S2I(MODEM_STATUS_CHECK_TIME), g_modemStatusCheckTimerCallBack, NULL);

}


/*------------------------------------------------------------------------
*Function 	: 	actio11.
*Description: 	Checks modem status.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action11(void)
{
	chprintf(g_chp,"Action11 : Entry.\r\n");
	modemHealth_e modemHealth	= MODEM_STATUS_INVALID;

	modemHealth = usfn_CheckModemStatus();
	usfn_CheckSMS();

	if((MODEM_STATUS_INVALID == modemHealth) || (MODEM_STATUS_TCP_CPNNECTED == modemHealth))
		return;
	else
	{
		switch(modemHealth)
		{
			case MODEM_STATUS_TCP_ERROR		:	g_lcrEvent = LCR_EVENT_CLOUD_ERROR;
												break;
			case MODEM_STATUS_TCP_READY		:	g_lcrEvent = LCR_EVENT_CLOUD_READY;
												break;
			case MODEM_STATUS_DEVICE_RESTART:	g_lcrEvent = LCR_EVENT_RESTART;
												break;
		}
	}
	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
}


/*------------------------------------------------------------------------
*Function 	: 	action12.
*Description: 	Checks SMS reception.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action12(void)
{
	chprintf(g_chp,"Action12 : Entry.\r\n");
	usfn_CheckSMS();
}


/*------------------------------------------------------------------------
*Function 	: 	action14.
*Description: 	Triggers GPRS initialization.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action14(void)
{
	g_lcrAlarmStructure.errGprs 	= TRUE;
	g_lcrAlarmStructure.errCloud 	= TRUE;
	chprintf(g_chp,"Action14 : Entry.\r\n");
	usfn_ChangeLcrState(LCR_STATE_GPRS_INITIALIZATION);
	g_lcrEvent = LCR_EVENT_SIM_READY;
	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
}


/*------------------------------------------------------------------------
*Function 	: 	action15.
*Description: 	Checks whether device need to connect time server or
				regular server.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action15(void)
{
	g_lcrAlarmStructure.errGprs 	= FALSE;
	GPRS_LED_ON();
	chprintf(g_chp,"Action15 : Entry.\r\n");
	if(g_lcrGlobalFlags.dateTimeSet)
	{
		usfn_ChangeLcrState(LCR_STATE_CLOUD_CONNECTING);
		g_lcrEvent = LCR_EVENT_TIME_DATE_SET;
	}
	else
	{
		usfn_ChangeLcrState(LCR_STATE_TIME_DATE_SETTING);
		g_lcrEvent = LCR_EVENT_GPRS_READY;
	}
	g_modemRetryCount_t.gprsConnect = 0;
	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
}


/*------------------------------------------------------------------------
*Function 	: 	action16.
*Description: 	Changes state of after time&date set.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action16(void)
{
	chprintf(g_chp,"Action16 : Entry.\r\n");
	usfn_ChangeLcrState(LCR_STATE_CLOUD_CONNECTING);
	g_lcrEvent = LCR_EVENT_TIME_DATE_SET;
	g_modemRetryCount_t.serverConnect = 0;
	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
}


/*------------------------------------------------------------------------
*Function 	: 	action17.
*Description: 	Retries GPRS connection.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action17(void)
{
	g_lcrAlarmStructure.errGprs 	= TRUE;
	GPRS_LED_OFF();
	chprintf(g_chp,"Action17 : Entry.\r\n");
	g_modemRetryCount_t.gprsConnect++;
	g_lcrEvent = LCR_EVENT_SIM_READY;
	if(g_modemRetryCount_t.gprsConnect >= MODEM_CONNECTING_GPRS_RETRY_COUNT)
	{
		g_modemRetryCount_t.gprsConnect=0;
		usfn_ChangeLcrState(LCR_STATE_SIM_INITIALIZATION);
		g_lcrEvent = LCR_EVENT_SIM_ERROR;
	}
	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);

}


/*------------------------------------------------------------------------
*Function 	: 	action18.
*Description: 	When there is cloud error. GPRS initialization triggered.
				Timer to backup data is started and data sending to server
				stopped.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action18(void)
{
	g_lcrAlarmStructure.errCloud 	= TRUE;
	SERVER_LED_OFF();
	chprintf(g_chp,"Action18 : Entry.\r\n");
	usfn_ChangeLcrState(LCR_STATE_GPRS_INITIALIZATION);
	g_lcrEvent = LCR_EVENT_GPRS_ERROR;

	//Disbale all data sending timers as the device is offline.
	if(TRUE == chVTIsArmed(&g_modemStatusCheckTimer))
		chVTReset(&g_modemStatusCheckTimer);

	if(TRUE == chVTIsArmed(&g_periTimer))
		chVTReset(&g_periTimer);

	if(TRUE == chVTIsArmed(&g_alarmTimer))
		chVTReset(&g_alarmTimer);

	if(TRUE == chVTIsArmed(&g_hearBeatTimer))
		chVTReset(&g_hearBeatTimer);

	if(TRUE == chVTIsArmed(&g_sendTcpBackUpDataTimer))
		chVTReset(&g_sendTcpBackUpDataTimer);

	if(TRUE == chVTIsArmed(&g_serverTimeReSynchTimer))
		chVTReset(&g_serverTimeReSynchTimer);


	//Start tcp data backup timer.
	if(FALSE == chVTIsArmed(&g_storeAndForwardTimer))
	{
		chprintf(g_chp,"Action18 : Starting Store And Forward Timer.\r\n");
		chVTSet(&g_storeAndForwardTimer, TIME_S2I(g_simCloudDetails_t.periTime), g_storeAndForwardTimerCallBack, NULL);
	}

	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);

}


/*------------------------------------------------------------------------
*Function 	: 	action20.
*Description: 	Sends powerup message to the server.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action20(void)
{
	chprintf(g_chp,"Action20 : Entry.\r\n");
	if(usfn_CollectDataAndPushToServer(SEND_TCP_POWER_UP_MSG))
	{
		g_lcrEvent = LCR_EVENT_CLOUD_ERROR;
		chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
	}
}



/*------------------------------------------------------------------------
*Function 	: 	action21.
*Description: 	Sends acknoledgement to the server when modbus confiuration
				is received.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action21(void)
{
	chprintf(g_chp,"Action21 : Entry.\r\n");
	usfn_CollectDataAndPushToServer(SEND_TCP_MODBUS_CFG_ACK_FLAG);
	g_lcrEvent = LCR_EVENT_RESTART;
	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);
}


/*------------------------------------------------------------------------
*Function 	: 	action22.
*Description: 	When cloud is connected, start data sending timers.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action22(void)
{
	chprintf(g_chp,"Action22 : Entry.\r\n");
	g_lcrAlarmStructure.errCloud 	= FALSE;
	usfn_ChangeLcrState(LCR_STATE_CLOUD_CONNECTED);
	g_lcrEvent = LCR_EVENT_CLOUD_READY;
	SERVER_LED_ON();
	//Start data sending timers.
	if(FALSE == chVTIsArmed(&g_periTimer))
		chVTSet(&g_periTimer, TIME_S2I(g_simCloudDetails_t.periTime), g_periTimerCallBack, NULL);

	if(FALSE == chVTIsArmed(&g_alarmTimer))
		chVTSet(&g_alarmTimer, TIME_S2I(TCP_DATA_PERIOD_ALARM), g_alarmTimerCallBack, NULL);

	if(FALSE == chVTIsArmed(&g_hearBeatTimer))
		chVTSet(&g_hearBeatTimer, TIME_S2I(TCP_DATA_PERIOD_HEART_BEAT), g_hearBeatTimerCallBack, NULL);

	if(FALSE == chVTIsArmed(&g_serverTimeReSynchTimer))
		chVTSet(&g_serverTimeReSynchTimer, TIME_S2I(LCR_TIME_RESYNCH_PERIOD), g_serverTimeReSynchTimerCallBack, NULL);


	//If backup data exists, then start tcp backup data sending timer.
	if(TRUE == g_lcrGlobalFlags.tcpBackupDataExists)
	{
		if(FALSE == chVTIsArmed(&g_sendTcpBackUpDataTimer))
			chVTSet(&g_sendTcpBackUpDataTimer, TIME_S2I(TCP_BACKUP_DATA_SEND_PERIOD), g_sendTcpBackUpDataTimerCallBack, NULL);
	}

	//Stop if tcp backup timer if enabled.
	if(TRUE == chVTIsArmed(&g_storeAndForwardTimer))
		chVTReset(&g_storeAndForwardTimer);

	chMBPostTimeout(&eventManagerMailBox, (msg_t) &g_lcrEvent, TIME_INFINITE);

}


/*------------------------------------------------------------------------
*Function 	: 	action23.
*Description: 	Sends stored data in the SD card to the cloud.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action23(void)
{
	chprintf(g_chp,"Action23 : Entry.\r\n");
	tcpData_t tcpData={0};
	uint16_t tcpDataLen = 0;
	//Get data from the SD card.
	if(! usfn_getAPacketFromTheTcpBackupFile(&tcpData, &tcpDataLen))
	{
		if(usfn_PushDataToServer(&tcpData, tcpDataLen))
		{
			chprintf(g_chp,"Action23 : ERROR - While pushing backup data.\r\n");
		}
		else
		{
			g_cursorPosInTcpBackupFile = g_cursorPosInTcpBackupFile + tcpDataLen+2;
			chprintf(g_chp, "action23 : File Size : %u bytes, CursorPos : %u.\r\n",usfn_SdCardCheckFileExistance(TCP_DATA_BACKUP_FILE_NAME),g_cursorPosInTcpBackupFile);
			if(g_cursorPosInTcpBackupFile >= usfn_SdCardCheckFileExistance(TCP_DATA_BACKUP_FILE_NAME))
			{
				chprintf(g_chp, "action23 : All the backup data sent to server. Deleting the backup file.\r\n");
				usfn_SdCardDeleteFile(TCP_DATA_BACKUP_FILE_NAME);
				usfn_SdCardDeleteFile(TCP_DATA_BACKUP_CURSOR_POS_FILE_NAME);
				g_lcrGlobalFlags.tcpBackupDataExists = FALSE;
				g_cursorPosInTcpBackupFile = 0;
				if(TRUE == chVTIsArmed(&g_sendTcpBackUpDataTimer))
					chVTReset(&g_sendTcpBackUpDataTimer);
			}
		}
	}
	else
		chprintf(g_chp,"Action23 : ERROR - While getting tcp packet from the SD card.\r\n");

}

/*------------------------------------------------------------------------
*Function 	: 	action24.
*Description: 	Resynch the device time.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action24(void)
{
	chprintf(g_chp,"Action24 : Entry.\r\n");
	if(FALSE == usfn_GetTimeAndDate())
		chprintf(g_chp,"\r\nAction24 : Successfully synched the time.\r\n");
	else
		chprintf(g_chp,"\r\nAction24 : Couldn't synch the time.\r\n");

}

/*------------------------------------------------------------------------
*Function 	: 	action100.
*Description: 	Restart the device.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
static void action100(void)
{
	chprintf(g_chp,"Action100 : Entry.\r\n");

	cJSON *lcrConfigJson = cJSON_CreateObject();
	char *jSonStr=NULL;
	usfn_SdCardDeleteFile(TCP_DATA_BACKUP_CURSOR_POS_FILE_NAME);
	cJSON_AddNumberToObject(lcrConfigJson, TCP_DATA_BACKUP_CURSOR_JSON, (uint32_t)g_cursorPosInTcpBackupFile);
	jSonStr = cJSON_Print(lcrConfigJson);
	if(FALSE  == usfn_SdCardWriteFile(TCP_DATA_BACKUP_CURSOR_POS_FILE_NAME,jSonStr))
		chprintf(g_chp,"action100 : Cursor position of the TCP backup file successfully stored.\r\n%s\r\n",jSonStr);
	else
		chprintf(g_chp,"action100 : ERROR - Couldn't store the cursor position of TCP backup file.\r\n");
	chThdSleepSeconds(1);
	NVIC_SystemReset();
}
//----------------------------------------------------------------------------------------------------
void (*stateFuncExe[LCR_EVENT_END][LCR_STATE_END])(void) =
{
//						00-INVALID	01-LCR		02-SIM		03-GPRS		04-TIMEDATE	05-CLOUD	06-CLOUD	07-FTP
//									_INIT		_INIT		_INIT		_INIT		_CONING		_CONNECTED
/* 00 INVALID */		{noAction,	noAction,	noAction,	noAction,	noAction,	noAction,	noAction,	noAction},
/* 01 MODEM_INIT_DONE */{noAction,	action00,	action01,	noAction,	noAction,	noAction,	noAction,	noAction},
/* 02 SIM_ERROR */		{noAction,	noAction,	action03,	noAction,	noAction,	noAction,	noAction,	noAction},
/* 03 SIM_READY */		{noAction,	noAction,	action14,	action02,	noAction,	noAction,	noAction,	noAction},
/* 04 GPRS_ERROR */		{noAction,	noAction,	noAction,	action17,	noAction,	noAction,	noAction,	noAction},
/* 05 GPRS_READY */		{noAction,	noAction,	noAction,	action15,	action04,	noAction,	noAction,	noAction},
/* 06 TIME_DATE_ERROR */{noAction,	noAction,	noAction,	noAction,	action05,	noAction,	noAction,	noAction},
/* 07 TIME_DATE_SET */	{noAction,	noAction,	noAction,	noAction,	action16,	action06,	noAction,	noAction},
/* 08 CLOUD_ERROR */	{noAction,	noAction,	noAction,	noAction,	noAction,	action18,	action18,	noAction},
/* 09 CLOUD_READY */	{noAction,	noAction,	noAction,	noAction,	noAction,	action22,	action07,	noAction},
/* 10 PERI_TIME */		{noAction,	noAction,	noAction,	action17,	noAction,	noAction,	action08,	noAction},
/* 11 HEART_BEAT */		{noAction,	noAction,	noAction,	noAction,	noAction,	noAction,	action09,	noAction},
/* 12 ALARM */			{noAction,	noAction,	noAction,	noAction,	noAction,	noAction,	action10,	noAction},
/* 13 SIM_SWITCH */		{noAction,	noAction,	noAction,	noAction,	noAction,	noAction,	noAction,	noAction},
/* 14 POWER_UP */		{noAction,	noAction,	noAction,	noAction,	noAction,	noAction,	action20,	noAction},
/* 15 MODBUS_CFG_ACK */	{noAction,	noAction,	noAction,	noAction,	noAction,	noAction,	action21,	noAction},
/* 16 MODEM_STATUS */	{noAction,	noAction,	noAction,	noAction,	action12,	action11,	action11,	noAction},
/* 17 SEND ST&FW */		{noAction,	noAction,	noAction,	noAction,	noAction,	noAction,	action23,	noAction},
/* 18 RESYNCH_TIME*/	{noAction,	noAction,	noAction,	noAction,	noAction,	noAction,	action24,	noAction},
/* 19 RESTART */		{noAction,	noAction,	action100,	action100,	action100,	action100,	action100,	noAction}
};

//----------------------------------------------------------------------------------------------------
/*------------------------------------------------------------------------
*Function 	: 	eventManagerHandle.
*Description: 	Eventmanager's thread.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
THD_FUNCTION(eventManagerHandle, p)
{

	(void)p;

	lcrEvent *q_lcrEvent = NULL;

	g_lcrState = LCR_STATE_INITIALIZING;

	chMtxObjectInit(&g_mutex_lcrStateMachine);

	chMBObjectInit(&eventManagerMailBox, eventManagerMsgsQueue, EVENT_MANAGER_QUEUE_LEN);

	(*stateFuncExe[LCR_EVENT_MODEM_INITIALIZATION_DONE][g_lcrState])();

	while(1)
	{
		chMBFetchTimeout(&eventManagerMailBox, (msg_t *)&q_lcrEvent, TIME_INFINITE);

		chprintf(g_chp,"\r\nEVENT MANAGER SIZE : %u.\r\n",chMBGetUsedCountI(&eventManagerMailBox));

		chprintf(g_chp,"usfn_EventManager : state - %u; event - %u\r\n", g_lcrState, (lcrEvent)*q_lcrEvent);
		if((g_lcrState >= LCR_STATE_END) || ((lcrEvent)*q_lcrEvent >= LCR_EVENT_END))
		{
			chprintf(g_chp,"usfn_EventManager : ERROR - Invalid event\r\n");
			break;
		}

		(*stateFuncExe[(uint8_t)*q_lcrEvent][g_lcrState])();
	}
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_EventManager.
*Description: 	Starts event manager thread.
*Arguments	: 	None.
*Return	 	: 	FALSE	-	On success.
				TRUE	- 	On error.
*------------------------------------------------------------------------*/
bool usfn_EventManager(void)
{
	chprintf(g_chp,"usfn_EventManager : Entry\r\n");


	if((g_eventManagerThd = chThdCreateStatic(eventManagerThdWorkigArea, sizeof(eventManagerThdWorkigArea), EVENT_MANAGER_HANDLE_PRIORITY, eventManagerHandle, NULL)))
	{
		return FALSE;
	}

	return TRUE;
}
