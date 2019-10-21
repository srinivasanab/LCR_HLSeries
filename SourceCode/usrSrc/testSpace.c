#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "stdio.h"
#include "string.h"

#include "digitalIO.h"
#include "global.h"
#include "rs485Communication.h"
#include "testSpace.h"


/*------------------------------------------------------------------------
*Function 	: 	usfn_JumpToTestSpace.
*Description: 	A test space where user can write and test the code.
*Arguments	: 	None.
*Return	 	: 	None.
*------------------------------------------------------------------------*/
void usfn_JumpToTestSpace(void)
{
	RTCDateTime rtcDateTimeHal 	= {0};
	struct tm 	rtcStdTime		= {0};

	while(TRUE)
	{
		  //chThdSleepMilliseconds(1000);
			chThdSleepSeconds(2);
			rtcGetTime(&RTCD1, &rtcDateTimeHal);
			rtcConvertDateTimeToStructTm(&rtcDateTimeHal, &rtcStdTime, NULL);
			chprintf(g_chp, "\r\n : Current Date & Time - %s.\r\n", asctime(&rtcStdTime));
	}
}
