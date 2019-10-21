/*
 * global.c
 *
 *  Created on: Jun 30, 2019
 *      Author: Arjun Dongre
 */
#include "hal.h"
#include "global.h"



//Declare & initialize gobal variables
BaseSequentialStream *g_chp=NULL;
gsmModemDetails 	g_gsmModemDetails_t={0};
simCloudDetails 	g_simCloudDetails_t={0};
lcrGlobalData_t		g_lcrGlobalData={0};
lcrAlarmStructure   g_lcrAlarmStructure={0};
lcrGlobalFlags_t 	g_lcrGlobalFlags={0};




