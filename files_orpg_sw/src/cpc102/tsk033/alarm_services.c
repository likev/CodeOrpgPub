/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/17 21:40:07 $
 * $Id: alarm_services.c,v 1.1 2010/03/17 21:40:07 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/*\////////////////////////////////////////////////////////////////////////

   This is essentially the MALRM library without using interrupts (i.e.,
   alarm()).  The use of these functions rely on the user to periodically
   call ALARM_check_for_alarm().  The more frequently the user calls this
   function, the more "timely" the alarms.  

   This suite of functions is well-suited for applications which do not
   require absolute time precision for raising alarms.  

   To use these functions, the user must register the alarm ID, set the
   alarm, then periodically call ALARM_check_for_alarm().   Ideally the
   function ALARM_check_for_alarm() would be embedded in a main loop 
   where the loop is executed on the order of 1 to every few seconds. 

////////////////////////////////////////////////////////////////////////\*/

/* System Include Files/Local Include Files */
#include <alarm_services.h>

/* Static Global Variables. */
static time_t Next_alarm_time;
static Client_reg_t Client_reg;

/* Static Function Prototypes. */
static void Alarm_callbacks( time_t current_time );
static int Alarm_cancel_alarm( malrm_id_t alarmid, time_t current_time,
                               unsigned int *remaining_secs );
static int Alarm_dereg_alarm( malrm_id_t alarmid, time_t current_time );
static time_t Alarm_next_alarm( time_t current_time );
static int Alarm_reg_alarm( malrm_id_t alarmid, void (*callback)(malrm_id_t) );
static int Alarm_set_alarm( malrm_id_t alarmid, time_t start_time,
                            unsigned int interval_secs );
static void Client_callback( Alarmreg_entry_t *entry );
static Alarmreg_entry_t *Find_alarmreg_entry( malrm_id_t alarmid );
static void Modify_time_t( time_t *thetime, int flag, unsigned int secs );

/*\/////////////////////////////////////////////////////////////////////////

   Description: 
      Cancel a given (registered) alarm

   Input: 
      alarm id - pointer to storage for remaining number of seconds

   Output:
      remaining_secs - remaining seconds until the alarm would have been
                       raised.

   Returns: 
      0 upon success, or one of the following negative error codes:

                MALRM_BAD_ALARMID
		MALRM_ALARM_NOT_REGISTERED 	
		MALRM_ALARM_NOT_SET 		

////////////////////////////////////////////////////////////////////////\*/
int ALARM_cancel( malrm_id_t alarmid, unsigned int *remaining_secs ){

   time_t current_time;
   int retval;

   if( alarmid == 0 ) 
      return( MALRM_BAD_ALARMID );

   /* Cancel the previous system alarm ... */
   current_time = MISC_systime( (int *) NULL );
   retval = Alarm_cancel_alarm( alarmid, current_time, remaining_secs );

   /* Schedule the next alarm ... */
   Next_alarm_time = Alarm_next_alarm( current_time );

   return(retval);

/* End of ALARM_cancel() */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      Deregister an ALARM callback routine for a given alarm

   Input: 
      alarm id - pointer to MALRM callback routine

   Output:

   Returns: 
      The number of remaining registered alarms, or one of the
      following negative numbers:

        MALRM_BAD_ALARMID
	MALRM_ALARM_NOT_REGISTERED 

/////////////////////////////////////////////////////////////////////////\*/
int ALARM_deregister( malrm_id_t alarmid ){

   time_t current_time;
   int retval;

   if( alarmid == 0 ) 
      return( MALRM_BAD_ALARMID );

   /* Deregister the alarm ... */
   current_time = MISC_systime( (int *) NULL );
   retval = Alarm_dereg_alarm( alarmid, current_time );

   /* Schedule the next system alarm ... */
   Next_alarm_time = Alarm_next_alarm( current_time );

   return(retval);

/* End of ALARM_deregister() */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      Register an alarm.

   Input: 
      alarm id - alarm id.
      callback - pointer to callback function.

   Output:

   Returns: 
      The number of registered alarms, or one of the following negative 
      error codes:

        MALRM_BAD_ALARMID
        MALRM_DUPL_REG			
        MALRM_SUSPECT_PTR
        MALRM_TOO_MANY_ALARMS		

////////////////////////////////////////////////////////////////////////\*/
int ALARM_register( malrm_id_t alarmid, void (*callback)(malrm_id_t) ){

   time_t current_time ;
   int retval ;

   if( alarmid == 0 ) 
      return( MALRM_BAD_ALARMID );
   
   if( callback == NULL ) 
      return( MALRM_SUSPECT_PTR );

   current_time = MISC_systime( (int *) NULL );

   retval = Alarm_reg_alarm( alarmid, callback );

   /* Schedule the next system alarm ... */
   Next_alarm_time = Alarm_next_alarm( current_time );

   return(retval);

/* End of ALARM_register() */
}

/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

   Description: 
      Set a given (registered) alarm

   Input: 
      alarm id - alarm ID.
      start_time - alarm start time (UTC or MALRM_START_TIME_NOW).
      interval_secs - alarm interval time (zeroes mean one-shot)

   Returns: 
      0 upon success, or one of the following negative error codes:

         MALRM_ALARM_NOT_REGISTERD 
         MALRM_BAD_ALARMID
         MALRM_BAD_START_TIME

////////////////////////////////////////////////////////////////////////\*/
int ALARM_set( malrm_id_t alarmid, time_t start_time, 
               unsigned int interval_secs ){

   int retval;
   time_t current_time;
   double t_diff;

   if( alarmid == 0 ) 
      return( MALRM_BAD_ALARMID );

   current_time = MISC_systime( (int *) NULL );

   /* Ensure that the start time is reasonable ... */
   if( start_time == MALRM_START_TIME_NOW ) 
      start_time = current_time;

   else{

      t_diff = difftime( start_time, current_time );
      if( t_diff < 0 ) 
         return(MALRM_BAD_START_TIME);
       
   }

   retval = Alarm_set_alarm( alarmid, start_time, interval_secs );

   /* Schedule the next system alarm ... */
   Next_alarm_time = Alarm_next_alarm( current_time );

   return(retval);

/* End of ALARM_set() */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
     Checks if an alarm should be raised.   If an alarm should be raised,
     calls registered callback functions, then sets the next time an
     alarm should be raised.

////////////////////////////////////////////////////////////////////////\*/
void ALARM_check_for_alarm(){

   time_t current_time ;

   current_time = MISC_systime( (int *) NULL );

   /* If the Next_alarm_time is less than the current time, service
      the alarm. */
   if( (Next_alarm_time > 0) && (Next_alarm_time <= current_time) ){

      /* Invoke the registered alarm callback routines as appropriate ... */
      Alarm_callbacks( current_time );

      /* Schedule the next system alarm ... */
      Next_alarm_time = Alarm_next_alarm( current_time );

   }

   return ;

/* End of ALARM_check_for_alarm() */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      Invoke the registered alarm callback routines as required

   Input: 
      current_time - current time, UTC.

////////////////////////////////////////////////////////////////////////\*/
static void Alarm_callbacks( time_t current_time ){

   int i;
   double t_diff;

   for( i = 0; i < MALRM_MAX_ALARMS; ++i ){

      if( (Client_reg.alarm_array[i].id != 0)
                     &&
          (Client_reg.alarm_array[i].next_alarm_time > 0) ){
         
         t_diff = difftime( Client_reg.alarm_array[i].next_alarm_time,
                            current_time ) ;

         if( t_diff <= 0 ) 
            Client_callback( &Client_reg.alarm_array[i] );

      } /* endif the alarm is registered and set */

   } /* endfor every possible alarm */

   return;

/* End of malrmlr_callbacks() */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      Cancel a given alarm

   Input: 
      alarm_id - alarm ID
      current_time - current time, in UTC.
      remaining_secs - pointer to storage for number of remaining alarm seconds
                 (can be NULL).

   Outdddput: 
      remaining_secs - number of remaining alarm seconds.

   Returns: 
      0 upon success; otherwise one of the following negative error codes:

	MALRM_ALARM_NOT_REGISTERED
	MALRM_ALARM_NOT_SET

////////////////////////////////////////////////////////////////////////\*/
static int Alarm_cancel_alarm( malrm_id_t alarmid, time_t current_time,
                               unsigned int *remaining_secs ){

   int retval = 0;
   Alarmreg_entry_t *ptr;

   assert(alarmid);
   assert(current_time);

   ptr = Find_alarmreg_entry( alarmid );
   if( ptr == NULL ) 
      return( MALRM_ALARM_NOT_REGISTERED );

   if( ptr->next_alarm_time == 0 ) 
      retval = MALRM_ALARM_NOT_SET;

   if( remaining_secs != NULL ){

      double t_diff;

      t_diff = difftime( ptr->next_alarm_time, current_time );
      if( t_diff >= 0 ) 
         *remaining_secs = (unsigned int) t_diff;

      else 
         *remaining_secs = 0;
      
   }

   ptr->next_alarm_time = 0;
   ptr->interval_secs = 0;

   return(retval);

/*END of Alarm_cancel_alarm()*/
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      Update the Client Alarm Registry to deregister the callback routine 
      for a given alarm.

   Input: 
      alarm_id - alarm ID.
      current_time - current time, UTC.

   Returns: 
      Number of remaining alarm callback routine registrations
      upon success; otherwise one of the following negative error
      codes:

	MALRM_ALARM_NOT_REGISTERED

////////////////////////////////////////////////////////////////////////\*/
static int Alarm_dereg_alarm( malrm_id_t alarmid, time_t current_time ){

   register int i ;
   int retval = 0 ;

   assert(alarmid);
   assert(current_time);

   /* Locate the specified alarm ID in the Client Alarm Registry ... */
   for( i = 0; i < MALRM_MAX_ALARMS; ++i ){
 
      if( Client_reg.alarm_array[i].id == alarmid )
         break ;

   }

   if( i < MALRM_MAX_ALARMS ){

      current_time = MISC_systime( (int *) NULL );
      (void) Alarm_cancel_alarm( alarmid, current_time, NULL );

      Client_reg.alarm_array[i].id = 0;
      Client_reg.alarm_array[i].callback = NULL;
      if( Client_reg.num_alarms > 0 ) 
         --( Client_reg.num_alarms );
       
      else 
         Client_reg.num_alarms = 0;
       

      retval = Client_reg.num_alarms;

   }
   else{

      /* Unable to locate the specified alarm ID in the Client Alarm Registry. */
      retval = MALRM_ALARM_NOT_REGISTERED;
   }

   return(retval);

/* END of Alarm_dereg_alarm() */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      Determine the next alarm time.

   Input: 
      current_time - current time, UTC.

   Returns: 
      Time, UTC, when the next alarm should be raised.  A return value
      of 0 indicates no alarms are currently active. 

////////////////////////////////////////////////////////////////////////\*/
static time_t Alarm_next_alarm( time_t current_time ){

   int i ;
   unsigned int least_secs = UINT_MAX, num_secs = 0;
   time_t next_time;
   double t_diff ;

   assert(current_time);

   /* Initialize the return value to no alarms active.  If an alarm
      is found to be active, the return value will specify the time,
      in UTC, when the alarm should be raised. */
   next_time = 0;

   if( Client_reg.num_alarms == 0 ) 
      return(next_time);

   for( i = 0; i < MALRM_MAX_ALARMS; ++i ){

      if( (Client_reg.alarm_array[i].id != 0)
                      &&
          (Client_reg.alarm_array[i].next_alarm_time > 0) ){

         t_diff = difftime( Client_reg.alarm_array[i].next_alarm_time,
                            current_time );   

         /* The next alarm time is less than the current for this alarm id. */
         if( t_diff < 0 ){

            /* If client has an interval timer and the next alarm time
               happens to be less than the current time, make next alarm time
               happen in the future. */
            if( Client_reg.alarm_array[i].interval_secs > 0 ){

               t_diff = difftime( Client_reg.alarm_array[i].next_alarm_time,
                                  current_time );   
               while( t_diff <= (double) 0.0 ){

                  Modify_time_t( &Client_reg.alarm_array[i].next_alarm_time,
                                 MODIFY_TIME_T_ADD,
                                 Client_reg.alarm_array[i].interval_secs );
                  t_diff = difftime( Client_reg.alarm_array[i].next_alarm_time,
                                     current_time );   
      
               }

            }
            else{

               /*Note:  t_diff needs to be set to 0 if it difftime returns a 
                        negative value.  Otherwise least_secs might be set to 
                        UINT_MAX.  This causes next_time to be set to 0.  Should 
                        this happen, then the timer expiration will never be raised. 

                        If t_diff less than 0, we want to service this alarm right 
                        away anyway. */ 
               t_diff = 0.0;

            }

         }

         /* Find the smallest time interval. */
         if( (unsigned int) t_diff < least_secs )
            least_secs = (unsigned int) t_diff;

      } /* endif alarm registered and set */
      
   } /* end for every possible alarm */

   /* If the smallest time interval is valid, compute the
      next alarm time. */
   if( least_secs != UINT_MAX ){

      /* We found a set alarm ... */
      if( least_secs != 0 ){

         /* For portability ... */
         if( least_secs <= MAX_ALARM_SECS ) 
            num_secs = least_secs;
          
         else 
            num_secs = MAX_ALARM_SECS;
          
      }
      else 
         num_secs = MIN_ALARM_SECS;
       

      next_time = num_secs + current_time;

   }

   return( next_time );

/* End of Alarm_next_alarm() */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      Update the Client Alarm Registry to register an alarm callback 
      routine for a given alarm ID.

   Input: 
      alarm_id - alarm ID.
      callback - pointer to alarm callback routine.

   Returns:
      0 upon successfully registering MALRM callback routine for a given 
      alarm ID; otherwise, one of the following:

         MALRM_DUPL_REG
         MALRM_TOO_MANY_ALARMS

////////////////////////////////////////////////////////////////////////\*/
static int Alarm_reg_alarm( malrm_id_t alarmid, void (*callback)(malrm_id_t) ){

   int retval, i;

   assert(alarmid);
   assert(callback);

   /* Determine if a callback routine has already been registered
      for this alarm ID ... */
   for( i = 0; i < MALRM_MAX_ALARMS; ++i ){

      if( Client_reg.alarm_array[i].id == alarmid ) 
         return( MALRM_DUPL_REG );
       
   } /*endfor possible alarm IDs*/

   /* Determine if we can accomodate registration for this alarm ID ... */

   retval = 0;	
   for( i = 0; i < MALRM_MAX_ALARMS; ++i ){

      if( Client_reg.alarm_array[i].id == 0 ){

         Client_reg.alarm_array[i].id = alarmid;
         Client_reg.alarm_array[i].callback = callback;

         ++(Client_reg.num_alarms);

         if( Client_reg.num_alarms <= 0 ) 
            Client_reg.num_alarms = 1;
          
         retval = Client_reg.num_alarms;

         break;
         
      }

   } /*endfor possible alarm IDs*/

   if( i >= MALRM_MAX_ALARMS )
      retval = MALRM_TOO_MANY_ALARMS;

   return(retval);

/* End of Alarm_reg_alarm() */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      Update the Client Alarm Registry to set an alarm.

   Input: 
      alarmid - alarm id.
      start_time - start (POSIX) time, time interval (seconds)

   Returns:
      0 upon successfully setting the alarm; otherwise, one of the 
      following negative error codes:

	MALRM_ALARM_NOT_REGISTERED

     An interval of zero seconds indicates a "one-shot" alarm.

////////////////////////////////////////////////////////////////////////\*/
int Alarm_set_alarm( malrm_id_t alarmid, time_t start_time,
                     unsigned int interval_secs )
{

   int retval = 0, i;

   assert(alarmid);
   assert(start_time);

   for( i = 0; i < MALRM_MAX_ALARMS; ++i ){

      if( Client_reg.alarm_array[i].id == alarmid )
         break;

   }

   if( i >= MALRM_MAX_ALARMS ) 
      return( MALRM_ALARM_NOT_REGISTERED );
  
   /* Update this entry of the Client Alarm Registry ... */
   Client_reg.alarm_array[i].next_alarm_time = start_time;
   Client_reg.alarm_array[i].interval_secs = interval_secs;

   return(retval);

/* End of Alarm_set_alarm() */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      Invoke the registered alarm client callback routine as required.

   Input: 
      entry - Alarmreg_entry_t for this alarm

////////////////////////////////////////////////////////////////////////\*/
static void Client_callback( Alarmreg_entry_t *entry ){

   /* Call client's callback routine. */
   entry->callback( entry->id );
    
   /* Update the Next Alarm Time element ... */
   if( entry->interval_secs > 0 ) 
       Modify_time_t( &(entry->next_alarm_time), MODIFY_TIME_T_ADD,
                      entry->interval_secs );
     
    else 
       entry->next_alarm_time = 0; 

   return;

/* End of Client_callback() */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      Given an alarm ID, find the matching Client Alarm Registry
      entry and return a pointer to that entry.

   Input: 
      alarmid - alarm ID

   Returns: 
      A pointer to the correct entry upon success; NULL otherwise.

////////////////////////////////////////////////////////////////////////\*/
static Alarmreg_entry_t *Find_alarmreg_entry( malrm_id_t alarmid ){

   Alarmreg_entry_t *cur_p = NULL;
   int i;

   assert(alarmid);

   for( i = 0; i < MALRM_MAX_ALARMS; ++i ){

      if( Client_reg.alarm_array[i].id == alarmid ) 
         break ;

   }

   if( i < MALRM_MAX_ALARMS ) 
      cur_p = &(Client_reg.alarm_array[i]) ;

   return(cur_p) ;

/* End of Find_alarmreg_entry() */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      Modify a time_t value.

   Input: 
      thetime - pointer to time_t to be modified.
      flag - operation flag.
      secs - number of seconds.

   Output: 
      thetime - time_t is modified

**************************************************************************/
static void Modify_time_t( time_t *thetime, int flag, unsigned int secs ){

   double tmp1 ;
   double tmp2 ;
   double tmp3 ;

   assert(*thetime) ;

   tmp1 = (double) *thetime;
   tmp2 = (double) secs;

   if( flag == MODIFY_TIME_T_ADD ){

      tmp3 = tmp1 + tmp2;
      if( (time_t) tmp3 > 0 ) 
         *thetime = (time_t) tmp3;
      
   }
   else if( flag == MODIFY_TIME_T_SUB ){

      tmp3 = tmp1 - tmp2;
      if( (time_t) tmp3 > 0 )
         *thetime = (time_t) tmp3 ;

   }
   
   return ;

/*END of Modify_time_t()*/
}
