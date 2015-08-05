/* 
 * RCS info 
 * $Author: steves $ 
 * $Locker:  $ 
 * $Date: 2012/11/28 15:27:40 $ 
 * $Id: rpg_timer_services.c,v 1.6 2012/11/28 15:27:40 steves Exp $ 
 * $Revision: 1.6 $ 
 * $State: Exp $ 
 */ 
#include <rpg.h>

#define MAX_TIMERS      	20

/*
  For tracking timers and callback routines.
*/
typedef struct {
   
   malrm_id_t timer_id;     /* Timer IDs.                    */
   void (*callback)();      /* Callback routines registered. */

} Timer_t;

static Timer_t Reg_timer[ MAX_TIMERS ];  /* Timer tracking data. */
static int Num_timers = 0;               /* Number of timers registered. */
static int Initialized = 0;              /* Timer data init flag. */

/*
  Function prototypes.
*/
void Timer_handler( malrm_id_t parameter );
static void Init_timer_data();


/*\/////////////////////////////////////////////////////////////
//
//  Description:  This function sets a previously registered 
//                and set timer.  The timer expiration time is 
//                the current time plus *count/1000 seconds in 
//                the future.  
//
//                Only a single interval timer is supported.
//
//  Inputs:       parameter - The timer id.
//                count - The timer interval, in milliseconds.
//                flag - Flag value indicating type of timer.
//                       0 = Time of Day
//                      -1 = Interval
//                       Currently, only interval timers are
//                       supported.
//                ier - Error return value.
//
//  Outputs:      ier - The status of the operation. 
//
//  Returns:      There are no return values defined for this 
//                function.
//
//  Globals:
//
//  Notes:
//
/////////////////////////////////////////////////////////////\*/ 
void a3cm40( int *parameter, int *count, int *flag, int *ier ){

   time_t secs, start_time;

   /* 
     If flag has value of zero (timer is specified as Time Of Day,
     abort this task since this feature is not currently supported.
   */
   if( *flag == 0 )
      PS_task_abort( "Time of Day Timers Not Supported\n" );

   /*
     Convert count into seconds since it arrives in units of 
     millseconds. 
   */
   secs = *count/1000; 

   /*
     Get the current POSIX time, then add secs to it to arrive at
     start_time.
   */
   start_time = MISC_systime( (int *) NULL ) + secs;

   /*
     Set the timer.
   */
   *ier = MALRM_set( (malrm_id_t) *parameter, start_time, 
                     MALRM_ONESHOT_NTVL );

}

/*\/////////////////////////////////////////////////////////////
//
//  Description:  This function cancels a previously registered
//                and set timer.  
//
//  Inputs:       parameter - The timer id.
//                count - The timer interval, in milliseconds.
//                flag - Flag value indicating type of timer.
//                       0 = Time of Day
//                      -1 = Interval
//                       Currently, only interval timers are
//                       supported.
//                ier - Error return value.
//
//  Outputs:      ier - The status of the operation. 
//
//  Returns:      There are no return values defined for this 
//                function.
//
//  Globals:
//
//  Notes:
/////////////////////////////////////////////////////////////\*/ 
void a3cm41( int *parameter, int *count, int *flag, int *ier ){

   unsigned int remaining_time;

   /*
     Cancel the timer.
   */
   *ier = MALRM_cancel( (malrm_id_t) *parameter, &remaining_time );

}

/*\/////////////////////////////////////////////////////////////
//
//  Description:  This function registers a timer.  Public 
//                interface.
//
//  Inputs:       parameter - The timer id.
//                callback - Timer expiration service routine.
//
//  Outputs:       
//
//  Returns:      There are no return values defined for this 
//                function.
//
//  Globals:      Reg_timer - Timer registration tracking data.
//                Initialized - Timer registration tracking data
//                              initialized flag.
//
//  Notes:	  If the timer registration fails, the task
//                will abort.
//
/////////////////////////////////////////////////////////////\*/ 
void RPG_reg_timer( fint *parameter, void (*callback)() ){

   /* Check for valid timer ID. */
   if( *parameter == RESERVED_TIMER_ID )
      PS_task_abort( "Can Not Use Reserved Timer ID %d\n",
                     RESERVED_TIMER_ID );


   /* Register the timer. */
   TS_reg_timer( *parameter, callback );

}

/*\/////////////////////////////////////////////////////////////
//
//  Description:  This function registers a timer.  Private
//                interface.
//
//  Inputs:       parameter - The timer id.
//                callback - Timer expiration service routine.
//
//  Outputs:       
//
//  Returns:      There are no return values defined for this 
//                function.
//
//  Globals:      Reg_timer - Timer registration tracking data.
//                Initialized - Timer registration tracking data
//                              initialized flag.
//
//  Notes:	  If the timer registration fails, the task
//                will abort.
//
/////////////////////////////////////////////////////////////\*/ 
void TS_reg_timer( int parameter, void (*callback)() ){

   int ret, i;

   /*
     Initialize timer tracking data.
   */
   if( !Initialized )
      Init_timer_data();

   /*
     Find an empty slot.   
   */
   for (i = 0; i < Num_timers; i++) {
      if (Reg_timer [i].timer_id == (malrm_id_t) 0)
         break;
   }

   /*
     Track this timer id and callback routine.
   */
   if (i < MAX_TIMERS) {

      Reg_timer [i].timer_id = parameter;
      Reg_timer [i].callback = callback;
      if (i >= Num_timers)
         Num_timers = i + 1;

   }

   /*
     Register the timer.
   */
   ret = MALRM_register( (malrm_id_t) parameter, Timer_handler ); 

   if( ret < 0 )
      PS_task_abort( "MALRM_register Error.  Ret = %d\n", ret );

}

/*\/////////////////////////////////////////////////////////////
//
//  Description:  This function is the generic timer expiration
//                handler. This function checks if the timer
//                has been registered, then calls the callback
//                routine associated with the timer_id.  The
//                function called passes the timer id by
//                address to support calling FORTRAN. 
//
//  Inputs:       parameter - The timer id.
//
//  Outputs:       
//
//  Returns:      There are no return values defined for this 
//                function.
//
//  Globals:      Reg_timer - Timer registration tracking data.
//
//  Notes:	  
//
/////////////////////////////////////////////////////////////\*/ 
void Timer_handler( malrm_id_t parameter ){

   int i;

   /*
     Find a match on the parameter.
   */
   for( i = 0; i < Num_timers; i++ ){

      if( Reg_timer [i].timer_id == parameter )
         break;

   }

   /*
     If parameter is valid, call callback routine.
   */
   if( i < Num_timers ){
 
      Reg_timer[i].callback ( &Reg_timer[i].timer_id );
      return;

   }

   /*
     Parameter not recognized.  Report error.
   */
   LE_send_msg( GL_INFO, "Timer ID %d Not Recognized.\n", parameter );
      
}

/*\/////////////////////////////////////////////////////////////
//
//  Description:  This function initializes the timer structures.
//
//  Inputs:       parameter - The timer id.
//
//  Outputs:      Initialized - Flag set prior to return from 
//                              module.  
//
//  Returns:      There are no return values defined for this 
//                function.
//
//  Globals:      Reg_timer - Timer registration tracking data.
//                Initialized - Timer registration tracking data
//                              initialized flag.
//
//  Notes:	  
//
/////////////////////////////////////////////////////////////\*/ 
static void Init_timer_data(){

   int i;

   /*
     Initialize timer tracking data.
   */
   for( i = 0; i < MAX_TIMERS; i++ ){

      Reg_timer [i].timer_id = (malrm_id_t) 0;
      Reg_timer [i].callback = NULL;

   }

   /* Initialize the MISC_systime function(). */
   MISC_systime( (int *) NULL );

   /*
     Set timer tracking data initialized flag.
   */
   Initialized = 1;

}
