/*
 * digitalIO.c
 *
 *  Created on: Jun 30, 2019
 *      Author: Arjun Dongre
 */
#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "stdbool.h"

#include "digitalIO.h"
#include "global.h"
#include "lcrConfig.h"




#if LCR_CONF_DIGITAL_IO_BOARD_CONNECTED //Enabled only if IO board config is enabled.

//Global variables.
dIoAdcData_t g_dIoAdcData_t = {0};

/*------------------------------------------------------------------------
*Function 	: 	digitalIOBoardHandle
*Description: 	A thread to read Digital Inputs and Output state.
*Arguments	: 	p - pointer to arguments to be passed to the thread.
*------------------------------------------------------------------------*/
THD_FUNCTION(digitalIOBoardHandle, p)
{
	chRegSetThreadName("DIGITAL_IO_BOARD_THREAD");
	(void)p;
	while(1)
	{
		g_dIoAdcData_t.digitalInput1 = palReadPort(DIGITAL_IO_PINS_PORT);	//Read status of IO.
		g_dIoAdcData_t.dIn16		 = palReadPad(IO_CARD_TAMPER_SWITCH4_PIN_PORT,IO_CARD_TAMPER_SWITCH4_PIN);

//		chprintf(g_chp,"digitalIOBoardHandle : %X %X\r\n", g_dIoAdcData_t.digitalInput1, g_dIoAdcData_t.dIn16);
		chThdSleepMilliseconds(500);

	}
}

#endif //LCR_CONF_DIGITAL_IO_BOARD_CONNECTED
