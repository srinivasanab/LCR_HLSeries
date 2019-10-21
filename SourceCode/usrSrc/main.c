/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "stdint.h"
#include "stdio.h"

#include "digitalIO.h"
#include "eventManager.h"
#include "global.h"
#include "initialization.h"
#include "testSpace.h"
#include "version.h"


/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();


	//Initializing serial port for debugging (UART1)
	sdStart(DEBUG_PORT, NULL);
	g_chp = (BaseSequentialStream *)DEBUG_PORT;

	chprintf(g_chp,"\r\n_____________________________________________________________\r\n\r\n");
	chprintf(g_chp,"		LCR_Application (Ver:%d.%d.%d)\r\n", LCR_VERSION_MAJOR, LCR_VERSION_MINOR, LCR_VERSION_REVISION);
	chprintf(g_chp,"_____________________________________________________________\r\n");

#if 1 // If this made 0, program moves to test function. Where test codes can be written and tested.
	//Initialize board peripherals.
	if(FALSE == usfn_BoardInitialization())
	{
		// Read LcrConfig data and initialize the firmware.
		if(FALSE == usfn_AppInitialization())
		{
			//Start the Event manager.
			if(FALSE == usfn_EventManager())
			{
				//event manager started....
			}
			else
				chprintf(g_chp,"main : ERROR - Couldn't start event the manager.\r\n");
		}
		else
			chprintf(g_chp,"main : ERROR - usfn_AppInitialization.\r\n");
	}
	else
		chprintf(g_chp,"main : ERROR - usfn_BoardInitialization.\r\n");

  // Idle thread.
  while (TRUE)
  {
	  chThdSleepMilliseconds(500);
  }
  chprintf(g_chp,"main : ERROR - PROGRAM ENDED..!!\r\n");

#else
	usfn_JumpToTestSpace();
#endif

}
