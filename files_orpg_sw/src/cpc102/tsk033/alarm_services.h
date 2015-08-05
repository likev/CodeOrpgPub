/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/17 21:40:07 $
 * $Id: alarm_services.h,v 1.1 2010/03/17 21:40:07 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
#ifndef ALARM_SERVICES_H
#define ALARM_SERVICES_H

#include <config.h>
#include <unistd.h>
#include <malrm.h>
#include <limits.h>      
#include <le.h>
#include <assert.h>
#include <infr.h>

/*
  Macro Definitions
*/
#define MIN_ALARM_SECS          1
#define MAX_ALARM_SECS          65535
#define MODIFY_TIME_T_ADD       1
#define MODIFY_TIME_T_SUB       2

/*
  Structure Definitions.
*/
 /* Client Alarm Registry */
typedef struct {

   malrm_id_t id;                       /* alarm ID (zero means "unregistered") */

   time_t next_alarm_time;

   unsigned int interval_secs;          /* zero means "one-shot" */

   void (*(callback))(malrm_id_t);      /* pointer to MALRM callback routines */

} Alarmreg_entry_t;

typedef struct {

   int num_alarms;			/* Number of registered alarms. */

   Alarmreg_entry_t alarm_array[MALRM_MAX_ALARMS];

} Client_reg_t;


/*
  Function Prototypes
*/

int ALARM_cancel( malrm_id_t alarmid, unsigned int *remaining_secs );
int ALARM_deregister( malrm_id_t alarmid );
int ALARM_register( malrm_id_t alarmid, void (*callback)(malrm_id_t) );
int ALARM_set( malrm_id_t alarmid, time_t start_time, unsigned int interval_secs );
void ALARM_check_for_alarm();

#endif
