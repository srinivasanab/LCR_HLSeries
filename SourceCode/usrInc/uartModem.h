/*
 * uarts.h
 *
 *  Created on: Jun 30, 2019
 *      Author: Arjun Dongre
 */

#ifndef USRINC_UARTMODEM_H_
#define USRINC_UARTMODEM_H_



#define MODEM_BAUDRATE			115200	// Baudrate to the modem.
#define MODEM_UART_BUFFER_SIZE	200		// Buffer size to store AT command response.

//Global functions.
bool usfn_ModemUartInitialization(void);
void usfn_SendAtCommand(char*, char*, uint32_t);

#endif /* USRINC_UARTMODEM_H_ */
