/**************************************************************************
  
      Module:  malrm_lib.c
  
 Description:
	This file provides four Multiple Alarm Services (MALRM)
    	library routines: MALRM_cancel(), MALRM_deregister(),
        MALRM_register(), and MALRM_set().

	The scope of all other routines defined within this file is limited
        to this file.  The private functions are defined in alphabetical
        order, following the definitions of the API functions (which also
        appear in alphabetical order).

	Note that a Unit Test (UT) driver has been provided at the bottom of
	this file.  To compile this source file into a UT binary, compile the
	file with UTMAIN defined and link-in the required object files
	(currently only malrm_libreg.o is required).  As problems are
	discovered, and/or as new test cases are defined (in the SDF), this
	UT driver must be updated.

Interruptible System Calls:
	none

 Memory Allocation:

	none

 Notes:
        Zero is not a valid alarm ID.

	We protect the MALRM database by enclosing the body of each API
	function between calls to alarm().  The first call cancels all
	alarm() requests, and the final call resets the alarm().
  
 Assumptions:
  	(1) the signal corresponding to MALRM_ALARM_SIG is reserved for
		the use of the MALRM library
  
 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/06/08 20:45:33 $
 * $Id: malrm_lib.c,v 1.9 2009/06/08 20:45:33 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/*
 * System Include Files/Local Include Files
 *
 */
#include <config.h>
#include <unistd.h>
#include <malrm.h>
#include <misc.h>

#define MALRM_LIB
#include <malrm_globals.h>
#undef MALRMEN_LIB 

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */

/*
 * Static Globals
 */

/*
 * Static Function Prototypes
 */
static void alarm_handler(void) ;


/**************************************************************************
 Description: Cancel a given (registered) MALRM alarm
       Input: alarm id, pointer to storage for remaining number of seconds
      Output:
              remaining seconds placed in storage pointed to by second arg
     Returns: zero upon success, or one of the following negative error codes:

                MALRM_BAD_ALARMID
		MALRM_ALARM_NOT_REGISTERED 	(malrmlr_cancel())
		MALRM_ALARM_NOT_SET 		(malrmlr_cancel())

       Notes:
 **************************************************************************/
int MALRM_cancel(malrm_id_t alarmid, unsigned int *remaining_secs)
{

   time_t current_time ;
   int retval ;

   if (alarmid == 0) {
      return(MALRM_BAD_ALARMID) ;
   }

   /*
    * Cancel the previous system alarm ...
    */
   (void) alarm(0) ;

   current_time = MISC_systime(NULL) ;
   retval = malrmlr_cancel_alarm(alarmid, current_time, remaining_secs) ;

   /*
    * Schedule the next system alarm ...
    */
   (void) alarm(malrmlr_next_alarm_secs(current_time)) ;

   return(retval) ;

/*END of MALRM_cancel()*/
}


/**************************************************************************
 Description: Deregister an MALRM callback routine for a given alarm
       Input: alarm id, pointer to MALRM callback routine
      Output:
     Returns: the number of remaining registered alarms, or one of the
              following negative numbers:

                MALRM_BAD_ALARMID
		MALRM_ALARM_NOT_REGISTERED (malrmlr_dereg_alarm())
		MALRM_SIGACTION_FAILED
		MALRM_SIGEMPTYSET_FAILED

       Notes:
 **************************************************************************/
int MALRM_deregister(malrm_id_t alarmid)
{
   time_t current_time ;
   int retval ;

   if (alarmid == 0) {
      return(MALRM_BAD_ALARMID) ;
   }

   /*
    * Cancel the previous system alarm ...
    */
   (void) alarm(0) ;

   current_time = MISC_systime(NULL) ;
   retval = malrmlr_dereg_alarm(alarmid, current_time) ;
   if (retval == 0) {

      /*
       * It is necessary to deregister the MALRM library signal handler ...
       */
      struct sigaction  sigact ;

      sigact.sa_handler = SIG_DFL ;

      retval = sigemptyset(&sigact.sa_mask) ;
      if (retval == -1) {
         return(MALRM_SIGEMPTYSET_FAILED) ;
      }

      sigact.sa_flags = (int) 0 ;

      retval = sigaction((int)  MALRM_ALARM_SIG,
     		(const struct sigaction *) &sigact , 
   		(struct sigaction *) NULL ) ;
      if (retval != 0) {
         return(MALRM_SIGACTION_FAILED) ;
      }

   } /*endif this was the last alarm callback routine registration */

   /*
    * Schedule the next system alarm ...
    */
   (void) alarm(malrmlr_next_alarm_secs(current_time)) ;


   return(retval) ;


/*END of MALRM_deregister()*/
}


/**************************************************************************
 Description: Register an alarm
       Input: alarm id, pointer to MALRM callback routine
      Output:
     Returns: the number of registered alarms, or one of the following
              negative error codes:

                MALRM_BAD_ALARMID
                MALRM_DUPL_REG			(malrmlr_reg_alarm())
		MALRM_SIGACTION_FAILED
		MALRM_SIGEMPTYSET_FAILED
                MALRM_SUSPECT_PTR
                MALRM_TOO_MANY_ALARMS		(malrmlr_reg_alarm())

       Notes:
 **************************************************************************/
int MALRM_register(malrm_id_t alarmid,void (*callback)(malrm_id_t))
{
   time_t current_time ;
   int retval ;
   int retval2 ;


   if (alarmid == 0) {
      return(MALRM_BAD_ALARMID) ;
   }

   if (callback == NULL) {
      return(MALRM_SUSPECT_PTR) ;
   }

   /*
    * Cancel the previous system alarm ...
    */
   (void) alarm(0) ;

   current_time = MISC_systime(NULL) ;

   retval = malrmlr_reg_alarm(alarmid,callback) ;
   if (retval == 1) {

      /*
       * It is necessary to register the MALRM library signal handler ...
       */
      struct sigaction  sigact ;

      sigact.sa_handler = (void (*)(int)) alarm_handler ;

      retval2 = sigemptyset(&sigact.sa_mask) ;
      if (retval2 == -1) {
         return(MALRM_SIGEMPTYSET_FAILED) ;
      }

      sigact.sa_flags = (int) 0 ;

      retval2 = sigaction((int)  MALRM_ALARM_SIG,
     		(const struct sigaction *) &sigact , 
   		(struct sigaction *) NULL ) ;
      if (retval2 != 0) {
         return(MALRM_SIGACTION_FAILED) ;
      }

   } /*endif this is the first alarm callback routine registration */


   /*
    * Schedule the next system alarm ...
    */
   (void) alarm(malrmlr_next_alarm_secs(current_time)) ;

   return(retval) ;

/*END of MALRM_register()*/
}


/**************************************************************************
 Description: Set a given (registered) MALRM alarm
       Input: alarm id
              alarm flags
              alarm start time
              alarm interval time (zeroes mean one-shot)
      Output:
     Returns: zero upon success, or one of the following negative error codes:

                MALRM_ALARM_NOT_REGISTERD (malrmlr_set_alarm())
                MALRM_BAD_ALARMID
                MALRM_BAD_START_TIME

       Notes:
 **************************************************************************/
int MALRM_set(malrm_id_t alarmid,
              time_t start_time,
              unsigned int interval_secs)
{

   int retval ;
   time_t current_time ;
   double t_diff ;

   if (alarmid == 0) {
         return(MALRM_BAD_ALARMID) ;
   }

   current_time = MISC_systime(NULL) ;

   /*
    * Ensure that the start time is reasonable ...
    */
   if (start_time == MALRM_START_TIME_NOW) {
      start_time = current_time ;
   }
   else {
      t_diff = difftime(start_time, current_time) ;
      if (t_diff < 0) {
         return(MALRM_BAD_START_TIME) ;
      }
   }

   /*
    * Cancel the previous system alarm ...
    */
   (void) alarm(0) ;

   retval = malrmlr_set_alarm(alarmid, start_time, interval_secs) ;

   /*
    * Schedule the next system alarm ...
    */
   (void) alarm(malrmlr_next_alarm_secs(current_time)) ;

   return(retval) ;

/*END of MALRM_set()*/
}



/**************************************************************************
 Description: Handle signal MALRM_ALARM_SIG delivered to this signal handler
              by the system.
       Input: void
      Output: alarm data to registered MALRM callback routine
     Returns: void
       Notes: ALL ROUTINES CALLED BY THIS SIGNAL HANDLER MUST THEMSELVES
              BE WRITTEN IN ACCORDANCE WITH GOOD SIGNAL-HANDLER DESIGN
              PRINCIPLES (e.g., use only reentrant functions!)

              errno is saved and restored, since this may be important
              to an interrupted system call.

 **************************************************************************/
static void alarm_handler()
{
   time_t current_time ;
   int errnosave ;

   errnosave = errno ;         /* for sake of any interrupted system call */

   current_time = MISC_systime(NULL) ;

   /*
    * Invoke the registered alarm callback routines as appropriate ... 
    */
   malrmlr_callbacks(current_time) ;

   /*
    * Schedule the next system alarm ...
    */
   (void) alarm(malrmlr_next_alarm_secs(current_time)) ;

   errno = errnosave ;

   return ;

/*END of alarm_handler()*/
}

