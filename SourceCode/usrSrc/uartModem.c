/*
 * uartModem.c
 *
 *  Created on: Jun 30, 2019
 *      Author: Arjun Dongre
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "string.h"

#include "global.h"
#include "lcrConfig.h"
#include "uartModem.h"


/*------------------------------------------------------------------------
*Function 	: 	usfn_ModemUartInitialization.
*Description: 	Setups UART module for modem communication.
*Arguments	: 	None.
*Return	 	: 	bool - 	On FALSE 	- Success.
						On TRUE		- Failure.
*------------------------------------------------------------------------*/
bool usfn_ModemUartInitialization(void)
{
	SerialConfig g_SerialConfig = {
										MODEM_BAUDRATE,
										0,
										0,
#if LCR_CONF_MODEM_SERIAL_HW_FLOW_CONTROL
										USART_CR3_RTSE|USART_CR3_CTSIE	//Hardware flow control enable
#else
										0
#endif
									};
	sdStart(MODEM_PORT, &g_SerialConfig);
	return false;
}


/*------------------------------------------------------------------------
*Function 	: 	usfn_SendAtCommand.
*Description: 	Sends AT command and receives the response.
*Arguments	: 	*command	-	A pointer to the AT command.
				*response	-	A pointer to response which is returned back.
				delay		-	To wait for response(In milliseconds).
*Return	 	: 	None.
*------------------------------------------------------------------------*/
void usfn_SendAtCommand(char *command, char *response, uint32_t delay)
{
	if(command!=NULL)
		sdWrite(MODEM_PORT, (const uint8_t *)command, (size_t)strlen(command));

	if(response!= NULL)
		sdReadTimeout (MODEM_PORT, (uint8_t *)response, MODEM_UART_BUFFER_SIZE,	TIME_MS2I(delay));

}
