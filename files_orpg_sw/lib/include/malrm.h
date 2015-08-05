/**************************************************************************

      Module: malrm.h

 Description: Multiple Alarm Services (MALRM) library public include file.

 Assumptions:
 **************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/09/14 14:37:39 $
 * $Id: malrm.h,v 1.3 2005/09/14 14:37:39 jing Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef MALRM_H
#define MALRM_H

/*
 * System Include Files/Local Include Files
 */
#include <time.h>
#include <signal.h>

/*
 * Type Definitions
 */
typedef unsigned int malrm_id_t ; /* integer type used for MALRM alarm IDs   */


/*
 * MALRM library routine prototypes
 */
#ifdef __cplusplus
extern "C"
{
#endif
int MALRM_cancel(malrm_id_t alarmid, unsigned int *remaining_secs) ;
int MALRM_deregister(malrm_id_t alarmid) ;
int MALRM_register(malrm_id_t alarmid,void (*callback)(malrm_id_t));
int MALRM_set(malrm_id_t alarmid,
              time_t start_time,
              unsigned int interval_secs) ;
int malrmlr_reg_alarm(malrm_id_t alarmid, void (*callback)(malrm_id_t));
#ifdef __cplusplus
}
#endif

/*
 * Definitions
 */
#define MALRM_MAX_ALARMS		8
#define MALRM_ALARM_SIG			SIGALRM
#define MALRM_START_TIME_NOW		(time_t) 0
#define MALRM_ONESHOT_NTVL		(unsigned int) 0
#define MALRM_MIN_NTVL_SECS		(unsigned int) 60
#define MALRM_HOUR_NTVL_SECS		(unsigned int) 3600
#define MALRM_DAY_NTVL_SECS		(unsigned int) 86400

/*
 * Error return values
 */
#define MALRM_BAD_ALARMID		-500
#define MALRM_REGISTER_FAILED  		-501
#define MALRM_SUSPECT_PTR		-502
#define MALRM_DEREGISTER_FAILED 	-503
#define MALRM_DUPL_REG			-504
#define MALRM_NVLD_DEREG		-505
#define MALRM_TOO_MANY_ALARMS	  	-507
#define MALRM_ALARM_NOT_REGISTERED  	-509
#define MALRM_BAD_START_TIME  		-510
#define MALRM_ALARM_NOT_SET  		-511

#define MALRM_SIGACTION_FAILED		-550
#define MALRM_SIGEMPTYSET_FAILED	-551



/*** DO NOT REMOVE ***/
#endif
/*** DO NOT REMOVE ***/
