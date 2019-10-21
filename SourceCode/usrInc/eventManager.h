/*
 * eventManager.h
 *
 *  Created on: Jun 30, 2019
 *      Author: Arjun Dongre
 */

#ifndef USRINC_EVENTMANAGER_H_
#define USRINC_EVENTMANAGER_H_

//Prioroty of the event manager thread.
#define EVENT_MANAGER_HANDLE_PRIORITY				3
//Stack size for the event manager thread.
#define EVENT_MANAGER_HANDLE_STACK_SIZE				30*1024 //4kB

//Size of buffer to store incoming events to the event manager thread.
#define EVENT_MANAGER_QUEUE_LEN	50

//Enus defines the state of device.
typedef enum {
	LCR_STATE_INVALID,				//Invalid state
	LCR_STATE_INITIALIZING,			//Initializing threads and timers.
	LCR_STATE_SIM_INITIALIZATION,	//SIM switch and initialization.
	LCR_STATE_GPRS_INITIALIZATION,	//GPRS initialization.
	LCR_STATE_TIME_DATE_SETTING,	//Getting time and date.
	LCR_STATE_CLOUD_CONNECTING,		//Cloud connecting.
	LCR_STATE_CLOUD_CONNECTED,		//Device is online.
	LCR_STATE_FTP_DOWNLOADING,		//DOTA state.
	LCR_STATE_END					// End.
}lcrState;

//Enum defines the events received.
typedef enum {
	LCR_EVENT_INVALID,						//Invalid event.
	LCR_EVENT_MODEM_INITIALIZATION_DONE,	//event when modem initialization is done.
	LCR_EVENT_SIM_ERROR,					//SIM error event.
	LCR_EVENT_SIM_READY,					//SIM registered event.
	LCR_EVENT_GPRS_ERROR,					//GPRS error event.
	LCR_EVENT_GPRS_READY,					//GPRS activated event.
	LCR_EVENT_TIME_DATE_SET_ERROR,			//Error while getting date & time event.
	LCR_EVENT_TIME_DATE_SET,				//Date and time set event.
	LCR_EVENT_CLOUD_ERROR,					//Error while connecting to cloud event.
	LCR_EVENT_CLOUD_READY,					//Cloud connected event.
	LCR_EVENT_PERI_TIME,					//Event to send regular data.
	LCR_EVENT_UPDATE_HEART_BEAT,			//Event to send regular data.
	LCR_EVENT_UPDATE_ALARM,					//Event to send regular data.
	LCR_EVENT_UPDATE_SIM_SWITCH_OVER,		//Event to send sim switch over data.
	LCR_EVENT_UPDATE_POWER_UP_MSG,			//Event to send power up message data.
	LCR_EVENT_UPDATE_MODBUS_CFG_ACK,		//Event to send acknoledgement to the received data.
	LCR_EVENT_MODEM_STATUS_CHECK,			//Event to check modem's tcp connection health.
	LCR_EVENT_SEND_STORE_AND_FORWARD,		//Event to send stored tcp data.
	LCR_EVENT_RESYNCH_DEVICE_TIME,			//Event to resynch the time.
	LCR_EVENT_RESTART,						//Event to restart the device.
	LCR_EVENT_END							//End of events.
}lcrEvent;


//typedef struct _lcrStateMachine
//{
//	lcrState	state;
//	lcrEvent	event;
//}lcrStateMachine;


//Globals
extern mailbox_t 				eventManagerMailBox;

bool usfn_EventManager(void);
void usfn_ChangeLcrState(lcrState);

#endif /* USRINC_EVENTMANAGER_H_ */
