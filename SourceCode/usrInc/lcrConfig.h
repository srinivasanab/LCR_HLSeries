#ifndef USRSRC_LCRCONFIG_H_
#define USRSRC_LCRCONFIG_H_

//File name of the configuration file.
#define LCR_CONFIG_FILE_NAME					"LCRCFG.TXT"



#define LCR_CONF_MODEM_SERIAL_HW_FLOW_CONTROL	TRUE	//Enables hardware flow control with the modem
#define LCR_CONF_MODEM_RESPONSE_ENABLE			TRUE	//Enables modem's response to the AT command on the console.
#define LCR_CONF_RS485_DEBUG_ENABLE				FALSE	//Enables RS485 communication debug logs on the console.
#define LCR_CONF_TCP_COM_DEBUG_ENABLE			TRUE	//Enables tcp debug logs on the console.
#define LCR_CONF_DIGITAL_IO_BOARD_CONNECTED		FALSE	//Configure whether IO board is connected or not.
#define LCR_CONF_MODEM_4G_IS_PRESENT			FALSE	//If Enabled then 4G mode is enabled otherwise 3G mode is used.
#define LCR_CONF_MODEM_SET_2G_MODE				TRUE	//If enabled modem is set to 2G band irrespective of the modem.
#define LCR_CONF_GET_TIME_FROM_OPERATOR			TRUE	//If enabled then time is taken from the modem using CCLK or else from time server.
#define LCR_CONF_TCP_DATA_IN_SINGLE_FRAME		FALSE	//If enabled then all the regular data pushed in the single frame.

#if LCR_CONF_TCP_DATA_IN_SINGLE_FRAME
	#define LCR_CONF_TCP_DATA_ADD_ZEROS_AT_THE_END	FALSE //If enabled then extra zeros padded at the tail of the tcp data.
#endif

#if LCR_CONF_TCP_DATA_IN_SINGLE_FRAME
	#define LCR_CONFIG_FILE_VERSION					2 //For green pole based on single frame
#else
	#define LCR_CONFIG_FILE_VERSION					1 //For vertive or bluestart for multiframes
#endif




#endif
