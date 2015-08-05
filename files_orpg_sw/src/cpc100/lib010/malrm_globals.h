/**************************************************************************

      Module:  malrm_globals.h

 Description:
	This include file provides definitions and declarations of global
	constants and variables.  The contents of this file may result in
	compile-time allocation of storage.

	Only file-scope globals may be defined in any other Multiple Alarm
	Services (MALRM) library source file.

 Assumptions:

 **************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/09/14 15:41:35 $
 * $Id: malrm_globals.h,v 1.5 2005/09/14 15:41:35 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef MALRM_GLOBALS_H
#define MALRM_GLOBALS_H


/*
 * System Include Files/Local Include Files
 * Truly global header files ...
 * Those header files required to define global variables ...
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include <malrm.h>
#include <malrm_def.h>

/*
 *  All routines may use the system error number ...
 */

#if defined MALRM_LIB || defined MALRM_LIBREG
/*
 * Function Prototypes:
 * malrmlr_ (malrm_libreg.c)
 * 	Invoke registered alarm callback routines
 *      Cancel an alarm
 *	Deregister an alarm
 *      Determine next system alarm() number of seconds
 *	Register an alarm
 *      Set an alarm
 */

#ifdef __cplusplus
extern "C"
{
#endif

void malrmlr_callbacks(time_t current_time) ;
int malrmlr_cancel_alarm(malrm_id_t alarmid,
                         time_t current_time,
                         unsigned int *remaining_secs) ;
int malrmlr_dereg_alarm(malrm_id_t alarmid,
                        time_t current_time) ;
unsigned int malrmlr_next_alarm_secs(time_t current_time) ;
int malrmlr_set_alarm(malrm_id_t alarmid,
                      time_t start_time,
                      unsigned int interval_secs) ;
#ifdef __cplusplus
}
#endif

#endif

#ifdef MALRM_LIB
#endif

#ifdef MALRM_LIBREG
#endif




#endif /*DO NOT DELETE*/
