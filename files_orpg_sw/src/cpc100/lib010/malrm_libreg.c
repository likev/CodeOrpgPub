/**************************************************************************
  
      Module:  malrm_libreg.c
  
 Description:
	This file provides the source for the Multiple Alarms Services
	(MALRM) library routines associated with handling client alarm
	registration.  In particular, this code maintains the Client
	Alarm Registry.

	Functions that are public are defined in alphabetical order at
	the top of this file and are identified by a prefix of "malrmlr_".

	Functions that are private to this file are defined in alphabetical
	order, following the definition of the public functions.

        Note that a Unit Test (UT) driver has been provided at the bottom of
        this file.  To compile this source file into a UT binary, compile the
        file with UTMAIN defined and link-in the required object files.
        As problems are discovered, and/or as new test cases are defined (in
	the SDF), this UT driver must be updated.

Interruptible System Calls:
	The following system calls that appear in this file may return an
	error value with errno set to EINTR:

 Notes:
        Zero is not a valid alarm ID.
        Zero is not a valid time_t value.

 Assumptions:
  
 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/06/08 20:45:34 $
 * $Id: malrm_libreg.c,v 1.11 2009/06/08 20:45:34 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/*
 * System Include Files/Local Include Files
 *
 */
#include <config.h>
#include <limits.h>         /* UINT_MAX */
#include <malrm.h>
#include <misc.h>

#define MALRM_LIBREG
#include <malrm_globals.h>
#undef MALRM_LIBREG

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 *
 * maximum alarm seconds was suggested by "POSIX Programmer's Guide" for
 *  ... portability
 */
#define MIN_ALARM_SECS	1
#define MAX_ALARM_SECS	65535
#define MODIFY_TIME_T_ADD 1
#define MODIFY_TIME_T_SUB 2


/*
 * Static Globals
 *
 *    Client Alarm Registry
 */
typedef struct {
   malrm_id_t id ;             /* alarm ID (zero means "unregistered"     */
   time_t next_alarm_time ;
   unsigned int interval_secs ; /* zero means "one-shot"                  */
   void (*(callback))(malrm_id_t) ;     
				/* pointer to MALRM callback routines      */
} alarmreg_entry_t ;

static struct {
   int num_alarms ;
   alarmreg_entry_t alarm_array[MALRM_MAX_ALARMS] ;
} Client_reg ;


/*
 * Static Function Prototypes
 */
static void Client_callback( alarmreg_entry_t *entry ) ;
static alarmreg_entry_t *find_alarmreg_entry(malrm_id_t alarmid) ;
static void modify_time_t(time_t *thetime, int flag, unsigned int secs) ;


/**************************************************************************
 Description: Invoke the registered alarm callback routines as required
       Input: current POSIX time
      Output: alarm ids are passed to callback routines
     Returns: void
       Notes:
 **************************************************************************/
void malrmlr_callbacks(time_t current_time)
{
   register int i ;
   double t_diff ;

   assert(current_time) ;

   for (i=0; i < MALRM_MAX_ALARMS; ++i) {

      if ((Client_reg.alarm_array[i].id)
                     &&
          (Client_reg.alarm_array[i].next_alarm_time)) {
         
     
         t_diff = difftime(Client_reg.alarm_array[i].next_alarm_time,
                           current_time) ;
         if (t_diff <= 0) {
            Client_callback( &Client_reg.alarm_array[i] );
         }

      } /*endif the alarm is registered and set*/

   } /*endfor every possible alarm*/

   return ;

/*END of malrmlr_callbacks()*/
}


/**************************************************************************
 Description: Cancel a given alarm
       Input: alarm ID
              current time
              pointer to storage for number of remaining alarm seconds
                 (can be NULL)
      Output: number of remaining alarm seconds
     Returns: zero upon success; otherwise one of the following negative
              error codes:

		MALRM_ALARM_NOT_REGISTERED
		MALRM_ALARM_NOT_SET

 **************************************************************************/
int malrmlr_cancel_alarm(malrm_id_t alarmid,
                         time_t current_time,
                         unsigned int *remaining_secs)
{
   int retval = 0 ;
   alarmreg_entry_t *ptr ;

   assert(alarmid) ;
   assert(current_time) ;

   ptr = find_alarmreg_entry(alarmid) ;
   if (ptr == NULL) {
      return(MALRM_ALARM_NOT_REGISTERED) ;
   }

   if (! ptr->next_alarm_time) {
      retval = MALRM_ALARM_NOT_SET ;
   }

   if (remaining_secs != NULL) {
      /*
       * Automatic variables ...
       */
      double t_diff ;

      t_diff = difftime(ptr->next_alarm_time, current_time) ;
      if (t_diff >= 0) {
         *remaining_secs = (unsigned int) t_diff ;
      }
      else {
         *remaining_secs = 0 ;
      }
   }

   ptr->next_alarm_time = 0 ;
   ptr->interval_secs = 0 ;

   return(retval) ;

/*END of malrmlr_cancel_alarm()*/
}


/**************************************************************************
 Description: Update the Client Alarm Registry to deregister the callback
              routine for a given alarm.
       Input: alarm ID, current POSIX time
      Output: none
     Returns: number of remaining alarm callback routine registrations
              upon success; otherwise one of the following negative error
              codes:

		MALRM_ALARM_NOT_REGISTERED

 **************************************************************************/
int malrmlr_dereg_alarm(malrm_id_t alarmid, time_t current_time)
{
   register int i ;
   int retval = 0 ;

   assert(alarmid) ;
   assert(current_time) ;

   /*
    * Locate the specified alarm ID in the Client Alarm Registry ...
    */
   for (i=0; i < MALRM_MAX_ALARMS; ++i) {
      if (Client_reg.alarm_array[i].id == alarmid) {
         break ;
      }
   }

   if (i < MALRM_MAX_ALARMS) {

      current_time = MISC_systime(NULL) ;
      (void) malrmlr_cancel_alarm(alarmid, current_time, NULL) ;

      Client_reg.alarm_array[i].id = 0 ;
      Client_reg.alarm_array[i].callback = NULL ;
      if (Client_reg.num_alarms > 0) {
         --(Client_reg.num_alarms) ;
      }
      else {
         Client_reg.num_alarms = 0 ;
      }

      retval = Client_reg.num_alarms ;

   }
   else {
      /*
       * Unable to locate the specified alarm ID in the Client Alarm Registry
       */
      retval = MALRM_ALARM_NOT_REGISTERED ;
   }

   return(retval) ;

/*END of malrmlr_dereg_alarm()*/
}


/**************************************************************************
 Description: Determine the next alarm number of elapsed seconds
       Input: current POSIX time
      Output: none
     Returns: number of elapsed seconds to be specified in next call to
              alarm()
       Notes: Returning a value of zero cancels all alarm() requests.  If
              we have an alarm that needs to be fired "immediately", we must
              return a value of 1 second.
 **************************************************************************/
unsigned int malrmlr_next_alarm_secs(time_t current_time)
{
   register int i ;
   unsigned int least_secs = UINT_MAX ;
   time_t nearest_time ;
   unsigned int num_secs  = 0 ;     /* value of zero cancels all requests */
   double t_diff ;

   assert(current_time) ;

   if (Client_reg.num_alarms == 0) {
      return(num_secs) ;
   }

   for (i=0; i < MALRM_MAX_ALARMS; ++i) {

      if ((Client_reg.alarm_array[i].id)
                      &&
          (Client_reg.alarm_array[i].next_alarm_time)) {

         t_diff = difftime(Client_reg.alarm_array[i].next_alarm_time,
                           current_time) ;   

         /* The next alarm time is less than the current for this alarm id,
            so call the client's callback routine. */
         if( t_diff < 0 ){

            Client_callback( &Client_reg.alarm_array[i] ) ;

            /* If this is a one shot alarm, process the next alarm id. */
            if( Client_reg.alarm_array[i].next_alarm_time == (time_t) 0 )
               continue;   

            /* If client has an interval timer and the next alarm time
               happens to be less than the current time, make next alarm time
               happen in the future. */
            if( Client_reg.alarm_array[i].interval_secs ){

               t_diff = difftime(Client_reg.alarm_array[i].next_alarm_time,
                                 current_time) ;   
               while( t_diff <= (double) 0.0 ){

                  modify_time_t( &Client_reg.alarm_array[i].next_alarm_time,
                                 MODIFY_TIME_T_ADD,
                                 Client_reg.alarm_array[i].interval_secs );
                  t_diff = difftime(Client_reg.alarm_array[i].next_alarm_time,
                                    current_time) ;   
      
               }

            }

         }

         if ((unsigned int) t_diff < least_secs) {
            nearest_time = Client_reg.alarm_array[i].next_alarm_time ;
            least_secs = (unsigned int) t_diff ;
         }

      } /*endif alarm registered and set*/
      
   } /*endfor every possible alarm*/

   if (least_secs != UINT_MAX) {
      /*
       * We found a set alarm ...
       */
      if (least_secs) {
         /*
          * For portability ...
          */
         if (least_secs <= MAX_ALARM_SECS) {
            num_secs = least_secs ;
         }
         else {
            num_secs = MAX_ALARM_SECS ;
         }
      }
      else {
         num_secs = MIN_ALARM_SECS ;
      }
   }

   return(num_secs) ;

/*END of malrmlr_next_alarm_secs()*/
}


/**************************************************************************
 Description: Update the Client Alarm Registry to register an alarm
              callback routine for a given alarm ID.
       Input: alarm ID, pointer to alarm callback routine
      Output: none
     Returns:
		zero upon successfully registering MALRM callback routine
			for a given alarm ID; otherwise, one of the
			following:

                MALRM_DUPL_REG
                MALRM_TOO_MANY_ALARMS

 **************************************************************************/
int malrmlr_reg_alarm(malrm_id_t alarmid, void (*callback)(malrm_id_t))
{
   register int i ;
   int retval ;

   assert(alarmid) ;
   assert(callback) ;

   /*
    * Determine if a callback routine has already been registered
    * for this alarm ID ...
    */
   for (i=0; i < MALRM_MAX_ALARMS; ++i) {
      if (Client_reg.alarm_array[i].id == alarmid) {
         return(MALRM_DUPL_REG) ;
      }
   } /*endfor possible alarm IDs*/

   /*
    * Determine if we can accomodate registration for this alarm ID ...
    */
   retval = 0;			/* not needed - turn off gcc warning */
   for (i=0; i < MALRM_MAX_ALARMS; ++i) {
      if (Client_reg.alarm_array[i].id == 0) {
         Client_reg.alarm_array[i].id = alarmid ;
         Client_reg.alarm_array[i].callback = callback ;

         ++(Client_reg.num_alarms) ;

         if (Client_reg.num_alarms <= 0) {
            Client_reg.num_alarms = 1 ;
         }

         retval = Client_reg.num_alarms ;

         break ;         /* LEAVE THE FOR LOOP ...                  */
      }

   } /*endfor possible alarm IDs*/

   if (i >= MALRM_MAX_ALARMS) {
      retval = MALRM_TOO_MANY_ALARMS ;
   }

   return(retval) ;

/*END of malrmlr_reg_alarm()*/
}


/**************************************************************************
 Description: Update the Client Alarm Registry to set an alarm
       Input: alarm id, start (POSIX) time, time interval (seconds)
      Output: none
     Returns:
              zero upon successfully setting the alarm; otherwise,
              one of the following negative error codes:

		MALRM_ALARM_NOT_REGISTERED

              An interval of zero seconds indicates a "one-shot" alarm

 **************************************************************************/
int malrmlr_set_alarm(malrm_id_t alarmid,
                      time_t start_time,
                      unsigned int interval_secs)
{

   register int i ;
   int retval = 0 ;

   assert(alarmid) ;
   assert(start_time) ;

   for (i=0; i < MALRM_MAX_ALARMS; ++i) {
      if (Client_reg.alarm_array[i].id == alarmid) {
         break ;
      }
   }

   if (i >= MALRM_MAX_ALARMS) {
      return(MALRM_ALARM_NOT_REGISTERED) ;
   }

  
   /*
    * Update this entry of the Client Alarm Registry ...
    */
   Client_reg.alarm_array[i].next_alarm_time = start_time ;
   Client_reg.alarm_array[i].interval_secs = interval_secs ;

   return(retval) ;

/*END of malrmlr_set_alarm()*/
}


/**************************************************************************
 Description: Invoke the registered alarm client callback routine as
              required
       Input: alarmreg_entry for this alarm
      Output: alarm id is passed to callback routine
     Returns: void
       Notes:
 **************************************************************************/
static void Client_callback( alarmreg_entry_t *entry )
{

   /* 
    * Call client's callback routine. 
    */
   entry->callback(entry->id) ;
    
   /*
    * Update the Next Alarm Time element ...
    */
    if (entry->interval_secs) {
       modify_time_t(&(entry->next_alarm_time),
                     MODIFY_TIME_T_ADD,
                     entry->interval_secs) ;
    }
    else {
       entry->next_alarm_time = 0 ; 
    }

   return ;

/*END of Client_callback()*/
}


/**************************************************************************
 Description: Given an alarm ID, find the matching Client Alarm Registry
              entry and return a pointer to that entry.
       Input: alarm ID
      Output: none
     Returns: a pointer to the correct entry upon success; NULL otherwise
       Notes: This is a short routine, but it helped unclutter several
              other routines.
 **************************************************************************/
static alarmreg_entry_t *find_alarmreg_entry(malrm_id_t alarmid)
{
   alarmreg_entry_t *cur_p = NULL ;
   register int i ;

   assert(alarmid) ;

   for (i=0; i < MALRM_MAX_ALARMS; ++i) {
      if (Client_reg.alarm_array[i].id == alarmid) {
         break ;
      }
   }

   if (i < MALRM_MAX_ALARMS) {
      cur_p = &(Client_reg.alarm_array[i]) ;
   }

   return(cur_p) ;

/*END of find_alarmreg_entry()*/
}


/**************************************************************************
 Description: Modify a time_t value
       Input: pointer to time_t to be modified
              operation flag
              number of seconds
      Output: time_t is modified
     Returns: void
       Notes: We use double because that is what difftime returns ... seems
              prudent
 **************************************************************************/
static void modify_time_t(time_t *thetime, int flag, unsigned int secs)
{

   double tmp1 ;
   double tmp2 ;
   double tmp3 ;

   assert(*thetime) ;

   tmp1 = (double) *thetime ;
   tmp2 = (double) secs ;

   if (flag == MODIFY_TIME_T_ADD) {
      tmp3 = tmp1 + tmp2 ;
      if ((time_t) tmp3 > 0) {
         *thetime = (time_t) tmp3 ;
      }
   }
   else if (flag == MODIFY_TIME_T_SUB) {
      tmp3 = tmp1 - tmp2 ;
      if ((time_t) tmp3 > 0) {
         *thetime = (time_t) tmp3 ;
      }
   }
   
   return ;

/*END of modify_time_t()*/
}

