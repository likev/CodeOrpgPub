/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/11/28 15:27:40 $
 * $Id: rpg_event_services.c,v 1.16 2012/11/28 15:27:40 steves Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */

#include <rpg.h>

/* Maximum Number of Registered Events. */
#define MAX_EVENTS 	     20

/* Maximum size of the Event Queue. */
#define MAX_QUEUE_SIZE       100

/* Error Code */
#define ES_EVENT_QUEUE_FULL -1

/* Define structure for event registration. */
typedef struct {

   en_t code[MAX_EVENTS+1];			/* Event code */
   int  internal_or_external[MAX_EVENTS+1];
   void (*service_routine[MAX_EVENTS+1])();
   int queued_parameter[MAX_EVENTS+1];		/* Parameter passed to service
						   routine upon event notification */
   LB_id_t msg_id[MAX_EVENTS+1];	        /* Message ID (Note: only applicable 
						   for UN.) */
   int  num_events;				/* Number of registered events */

} Evt_list;

/* Define structure for event queue node. */
typedef struct {

   int code;				/* Event code */
   char *msg;				/* Message associated with event */
   size_t msglen;			/* Message length */
   void (*service_routine)();           /* Event service routine */
   int queued_parameter;		/* Parameter associated with event */
   LB_id_t msg_id;			/* Message ID (Note: Only for UN). */

} Evt_queue_node;

/* File scope variables. */
static int Task_type;			/* Is this task TASK_EVENT_BASED? */
static int Evt_initialized = 0;		/* Set when events are registered. */
static int Evt_wait_for_event = 0;	/* If set, indicates task is event-based. */
static int Evt_wait_for_event_timeout = 0; /* If set, indicates wait for event timeout. */
static Evt_list Event_list;		/* Structure of all events registered by
                                           this task. */
static Evt_queue_node Event_queue[MAX_QUEUE_SIZE+1];
					/* Contains a list of all outstanding events. */
static int Event_queue_head;		/* The head of the outstanding event list, i.e.,
                                           the next event to be serviced. */
static int Event_queue_tail;		/* The tail of the event list, .i.e., where the 
                                           next incoming event will be placed. */

/* Local Functions. */
static int Register_UN( int *data_id, LB_id_t *msg_id, 
                           void (*service_routine)() );
static int Register_event( int *event_code, 
                           void (*service_routine)(),
                           int *queued_parameter,
                           int internal_or_external );
void UN_handler( int data_id, LB_id_t msg_id, int msg_info, void *arg );
void Timeout_handler( malrm_id_t *alarm_id );
void Event_handler( en_t evtcd, void *msg, size_t msglen );
void Initialize_event_list( );
int Service_event_queue();
static void Check_for_internal_events();
static void Special_clutprod_event_processing( char *msg, int msglen );

/* Public Functions. */

/********************************************************************

   Description: This function is used to control the processing of
                RPG algorithm/product task which are activated based
                on events rather than the availability of task inputs.

                The event queue is examined periodically.  If an event
                is on the queue, the event service routine associated
                with the event is called.  The queued parameter 
                associated with the registering of this event is 
                passed as an argument.

   Return:     There is no return value defined.

********************************************************************/
void RPG_wait_for_event(){

   int events_serviced, block_state, i;

   if( Evt_initialized == 0 )
      PS_task_abort( "Must Register For Events To Wait For Events\n" );

   /* Check if the EVT_WAIT_FOR_EVENT_TIMEOUT event is registered.
      If so, set the timeout now. */
   for( i = 0; i < Event_list.num_events; i++ ){

      if( EVT_WAIT_FOR_EVENT_TIMEOUT == Event_list.code[i] ){

         int flag = -1;  /* Indicates interval timer. */
         int status = 0, count = Event_list.queued_parameter[i]*1000;

         /* If first time through, must register the event. */
         if( Evt_wait_for_event == 0 ){

            LE_send_msg( GL_INFO, "Registering EVT_WAIT_FOR_EVENT_TIMEOUT timer\n" );
            RPG_reg_timer( &Event_list.code[i], Timeout_handler );

         }

         /* First cancel the timer if already set, then re-set the
            timer. */
         a3cm41( &Event_list.code[i], &count, &flag, &status );
         a3cm40( &Event_list.code[i], &count, &flag, &status );

         if( status < 0 )
            LE_send_msg( GL_INFO, "Unable to Register EVT_WAIT_FOR_EVENT_TIMEOUT\n" );
                    
         break;

      }

   }

   /* Set flag indicating the this task wishes to be event based by
      virtue of calling this module. */
   Evt_wait_for_event = 1;

   /* Wait for an event to occur. */
   while(1){

      /* Check for any internal generated events. */
      Check_for_internal_events();

      /* Set the NTF blocking. This temporarily prevents any external 
         events from being delivered.  We do this because if any 
         internal events occur, we enqueue event in the event list
         and any external event occurring at the same time may cause
         corruption of this queue. */
      block_state = LB_NTF_control( LB_NTF_BLOCK ); 
      if( block_state != 0 )
         LE_send_msg( GL_ERROR, "Setting NTF Blocking .... Already Set!\n" );

      /* Check the event queue and service any events on the queue. */
      events_serviced = Service_event_queue();

      /* Reset the NTF blocking. */
      block_state = LB_NTF_control( LB_NTF_UNBLOCK );
      if( block_state != 1 )
         LE_send_msg( GL_ERROR, "Resetting NTF Blocking .... Already Reset!\n" );

      /* If any events serviced this pass, return. */
      if( events_serviced )
         break;

      /* Sleep for several seconds then check event queue again. */
      sleep(2);
      
   }  

/* End of RPG_wait_for_event() */
}

/********************************************************************

   Description: This function registers a User Notification.
                The data_id and msg_id are sent as arguments to 
                the service routine.  

   Input:      data_id - The data id which this process wishes to
                            be informed when updates occur.
               msg_id - The message id of the updated message.
               service_routine - Routine called when the specified
                                 event occurs.
   Return:     Returns 0 on success.  Negative return value indicates
               error.

********************************************************************/
int RPG_UN_register( fint *data_id, LB_id_t *msg_id, 
                     void (*service_routine)() ){

   return( Register_UN( data_id, msg_id, service_routine ) );

} /* End RPG_UN_register() */

/********************************************************************

   Description: This function registers an event corresponding to
                event_code.  The queue_parameter is sent as an 
                argument to the event service routine.  These events
                are assumed generated externally.

   Input:      event_code - The event id which this process wishes to
                            be informed when it occurs.
               service_routine - Routine called when the specified
                                 event occurs.
               queued_parameter - parameter passed to event service
                                  routine when event is serviced.
   Return:     Returns 0 on success.  Negative return value indicates
               error.

********************************************************************/
int RPG_reg_for_external_event( fint *event_code, 
                                void (*service_routine)(),
                                fint *queued_parameter ){

   /* Register the external event. */
   return( Register_event( event_code, service_routine, queued_parameter,
                           EVT_EXTERNAL_EVENT ) );

/* RPG_reg_for_external_event() */
}

/********************************************************************

   Description: This function registers an event corresponding to
                event_code.  The queue_parameter is sent as an 
                argument to the event service routine.  These events
                are assumed generated internally.

   Input:      event_code - The event id which this process wishes to
                            be informed when it occurs.
               service_routine - Routine called when the specified
                                 event occurs.
               queued_parameter - parameter passed to event service
                                  routine when event is serviced.
   Return:     Returns 0 on success.  Negative return value indicates 
               error.

********************************************************************/
int RPG_reg_for_internal_event( fint *event_code, 
                                void (*service_routine)(),
                                fint *queued_parameter ){

   /* Register the internal event. */
   return( Register_event( event_code, service_routine, queued_parameter,
                           EVT_INTERNAL_EVENT ) );

/* RPG_reg_for_external_event() */
}

/* Private Functions. */

/********************************************************************

   Description:
      Checks if the passed event has been registered.

   Inputs:
      event_code - event ID to check.

   Outputs:

   Returns:
      1 if event registered, 0 otherwise.

   Notes:

*********************************************************************/
int ES_event_registered( int event_code ){

   int i;

   /* Check if this event code has been registered. */
   for( i = 0; i < Event_list.num_events; i++ ){

      /* If event registered, return true. */
      if( event_code == Event_list.code[i] )
         return (1);

   }

   /* Event not registered. Return false. */
   return (0);

/* End of ES_event_registered() */
}

/********************************************************************

   Description: This function registers an event corresponding to
                event_code.  The queue_parameter is sent as an 
                argument to the event service routine.

   Input:      event_code - The event id which this process wishes to
                            be informed when it occurs.
               service_routine - Routine called when the specified
                                 event occurs.
               queued_parameter - parameter passed to event service
                                  routine when event is serviced.
               internal_or_external - indicates whether this event
                                      is internally or externally
                                      generated.
   Return:     Returns 0 on success.  If negative, the return value 
               is the value returned from call to EN_register.

********************************************************************/
static int Register_event( int *event_code, 
                           void (*service_routine)(),
                           int *queued_parameter,
                           int internal_or_external ){

   en_t evtcd;
   int ret, i;
   
   /* Check if event list has been initialized.  If not, do so. */
   if( Evt_initialized == 0 )
      Initialize_event_list();

   /* Set event code. */
   evtcd = (en_t) *event_code;

   /* Check if this event code has already been registered. */
   for( i = 0; i < Event_list.num_events; i++ ){

      if( evtcd == Event_list.code[i] )
         break;

   }

   /* Event has not been registered yet so register it now. */
   if( i >= Event_list.num_events ){

      /* Too many events registered. */
      if( i >= MAX_EVENTS )
         LE_send_msg( GL_INFO, "Too Many Events Registered.\n" );
 
      else{
       
         /* Register the specified event with the event notification daemon.  
            Report any error in registration. */
         if( internal_or_external == EVT_EXTERNAL_EVENT ){

            ret = EN_register( evtcd, (void *) Event_handler );

            if( ret < 0 ){

               LE_send_msg( GL_EN(ret), "Unable to Register For Event %d\n",
                            evtcd );
               return ret;

            }

         }
         else if( evtcd == EVT_WAIT_FOR_EVENT_TIMEOUT ){

            /* Need to initialize MISC_systime(). */
            MISC_systime( (int *) NULL );

         }
  
         /* Track this event.  Save the tracked event code and service routine. */
         Event_list.code[i] = evtcd;
         Event_list.internal_or_external[i] = internal_or_external;
         Event_list.service_routine[i] = service_routine;
         Event_list.queued_parameter[i] = *queued_parameter;
         Event_list.msg_id[i] = 0;
         Event_list.num_events++;

      }

   }

   /* Event has already been registered.  Can only change the service routine. */
   else
      Event_list.service_routine[i] = service_routine;

   return 0;

/* End of Register_event() */
}

/********************************************************************

   Description: This function registers a UN. The fd and msg_id are
                sent as arguments to the event service routine.

   Input:      
               service_routine - Routine called when the specified
                                 UN occurs.
               data_id - data ID passed to UN service routine when
                         event is serviced.
               msg_id - message ID passed to UN service routine when
                        event is serviced.

   Return:     Returns 0 on success.  If negative, the return value 
               is the value returned from call to ORPGDA_UN_register.

********************************************************************/
static int Register_UN( int *data_id, LB_id_t *msg_id, 
                        void (*service_routine)() ){

   en_t evtcd;
   int ret, i;
   
   /* Check if event list has been initialized.  If not, do so. */
   if( Evt_initialized == 0 )
      Initialize_event_list();

   /* Set event code. */
   evtcd = (en_t) EVT_USER_NOTIFICATION;

   /* Check if this event code/data_id/msg_id combination have already
      been registered. */
   for( i = 0; i < Event_list.num_events; i++ ){

      if( (evtcd == Event_list.code[i])
                 &&
          (Event_list.queued_parameter[i] == *data_id)
                 &&
          (Event_list.msg_id[i] == *msg_id) )
         break;

   }

   /* Event has not been registered yet so register it now. */
   if( i >= Event_list.num_events ){

      /* Too many events registered. */
      if( i >= MAX_EVENTS )
         LE_send_msg( GL_INFO, "Too Many Events Registered.\n" );
 
      else{
       
         /* Register the specified User Notification.  Report any
            error in registration.  Note:  Since there is no way
            to know whether or not access to the LB is read and/or 
            write, open the LB for write.  If this fails, fall back
            to read-only.   Opening for write can prevent the 
            notification from being lost if the first access is read
            and later the access is write. */
         if( (ret = ORPGDA_open( *data_id, LB_WRITE | LB_READ )) < 0 )
            ret = ORPGDA_open( *data_id, LB_READ );
         
         if( (ret < 0) || (ret = ORPGDA_UN_register( *data_id, *msg_id, UN_handler )) < 0 ){
     
            LE_send_msg( GL_EN(ret), "Unable to Register For User Notification (%d %d)\n",
                         *data_id, *msg_id );
            return ret;

         }

         /* Track this event.  Save the tracked event code and service routine. */
         Event_list.code[i] = evtcd;
         Event_list.internal_or_external[i] = EVT_EXTERNAL_EVENT;
         Event_list.service_routine[i] = service_routine;
         Event_list.queued_parameter[i] = *data_id;
         Event_list.msg_id[i] = *msg_id;
         Event_list.num_events++;

      }

   }

   /* Event has already been registered.  Can only change the service routine. */
   else
      Event_list.service_routine[i] = service_routine;

   return 0;

/* End of Register_UN() */
}

/********************************************************************

   Description: This function initializes data associated with event
                notification registration and setting up the event 
                queue.  This function is only called once.

   Return:     There is no return value defined.  Global variable
               Evy_initialized is set prior to module exit.

********************************************************************/
void Initialize_event_list( ){

   int i;

   /* Initialize the number of events registered. */
   Event_list.num_events = 0;

   /* Set event queue head and tail. */
   Event_queue_head = 0;
   Event_queue_tail = -1;

   /* malloc space for event messages. */
   for( i = 0; i <= MAX_QUEUE_SIZE; i++ ){

      Event_queue[i].msg = (char *) malloc( EN_MAX_AN_MSG );
      if( Event_queue[i].msg == NULL )
         PS_task_abort( "malloc Failed for %d bytes\n", EN_MAX_AN_MSG );

      Event_queue[i].msglen = 0;

   }

   /* Set flag indicating event list has been initialized. */
   Evt_initialized = 1;

/* End of Initialize_event_list( ) */
}


/********************************************************************

   Description:
      This function initializes the Event Services module. 

********************************************************************/
void ES_initialize(){

   /* Get the task type. */
   Task_type = INIT_task_type();
   LE_send_msg( GL_INFO, "Task Type Registered as %d\n", Task_type );

/* End of ES_initialize() */
}

/********************************************************************

   Description: This function is the event handler for any registered
                event that is posted.  This function adds the event
                data to the event queue tail node. 

                If the event queue is full, the event is lost.

   Input:      evtcd - The event id associated with the event.
               msg - An optional message associated with the event.  
                     Currently, this message is saved but the 
                     interface does not provide a mechanism to set
                     this data.  By default this pointer is NULL.
               msglen - Length of the associaed message.  By default,
                        the length is 0.
   Return:     There are no return values defined for this function.

********************************************************************/
void Event_handler( en_t evtcd, void *msg, size_t msglen ){

   int block_state, i;

   /* If error occurred, post message and return. */
   if( evtcd == EN_POST_ERR_EVTCD ){

      LE_send_msg( GL_INFO, "Error in Event Notification.  Err = %d\n",
                   evtcd );
      return;
   
   }
   
   /* Block any external events from interrupting updating the event queue. 
      This is really only necessary if function was called owing to internal
      event being generated. */
   block_state = LB_NTF_control( LB_NTF_BLOCK );
   if( block_state != 0 )
      LE_send_msg( GL_ERROR, "Setting NTF Blocking .... Already Set!\n" );

   /* Check the circular queue to see if it is full.  If the queue 
      is full, the event will be ignored. */
   if( (((Event_queue_tail+1) % MAX_QUEUE_SIZE) + 1) == Event_queue_head ){

      LE_send_msg( GL_INFO, "Event Queue Is Full. Event %d Lost.\n",
                   evtcd );

      /* Unblock external events. */
      block_state = LB_NTF_control( LB_NTF_UNBLOCK );
      if( block_state != 1 )
         LE_send_msg( GL_ERROR, "Resetting NTF Blocking .... Already Reset!\n" );

      return;

   }

   /* Loop through the event register list.  When a match is found with the 
      desired event, save its event service routine, its event id, and 
      queued parameter.  Also save the message if one is associated with
      the event. */
   for( i = 0; i < Event_list.num_events; i++ ){

      if ( evtcd == Event_list.code[i] ){

         /* Increment the event queue tail and place this event at this 
            queue position. */
         Event_queue_tail = (Event_queue_tail % MAX_QUEUE_SIZE) + 1;

         /* Add message associate with this event. */
         if( msg != NULL ){

            /* Clip the message if longer than maximum allowed. */
            if( msglen > EN_MAX_AN_MSG ){

                LE_send_msg( GL_INFO, "Event Message Too Large: %d\n", msglen );
                msglen = EN_MAX_AN_MSG;

            }

            /* The event queue message buffer should never be a NULL pointer.
               Checking doesn't hurt anything.   Not checking will cause a
               segmentation violation if the pointer is NULL and we try to
               copy to it. */
            if( Event_queue[Event_queue_tail].msg != NULL ){

               memcpy( Event_queue[Event_queue_tail].msg, (char *) msg, msglen );
               Event_queue[Event_queue_tail].msglen = msglen;

            }
            else 
               Event_queue[Event_queue_tail].msglen = 0;

         }
         else
            Event_queue[Event_queue_tail].msglen = 0;

         Event_queue[Event_queue_tail].service_routine = Event_list.service_routine[i];
         Event_queue[Event_queue_tail].queued_parameter = Event_list.queued_parameter[i];
         Event_queue[Event_queue_tail].code = Event_list.code[i];

         break;

      }

   } 

   /* If this task is not event based, service this event immediately
      and any other events that may be queued. */
   if( (i < Event_list.num_events) && (!Evt_wait_for_event) )
      Service_event_queue();

   /* Unblock external events. */
   block_state = LB_NTF_control( LB_NTF_UNBLOCK );
   if( block_state != 1 )
      LE_send_msg( GL_ERROR, "Resetting NTF Blocking .... Already Reset!\n" );
 
   /* Exit module. */
   return;

/* End of Event_handler( ) */
}
     
/********************************************************************

   Description: This function is the handler for User Notification.
                This function adds the event data to the event queue
                tail node. 

                If the event queue is full, the event is lost.

   Input:      data_id - The data id associated with the UN.
               msg_id - The message ID associated with the UN.
               msg_info - see LB man page for details.
               arg - see LB man page for details.

   Return:     There are no return values defined for this function.

********************************************************************/
void UN_handler( int data_id, LB_id_t msg_id, int msg_info, void *arg ){

   int block_state, i;

   /* Block any external events from interrupting updating the event queue. 
      This is really only necessary if function was called owing to internal
      event being generated. */
   block_state = LB_NTF_control( LB_NTF_BLOCK );
   if( block_state != 0 )
      LE_send_msg( GL_ERROR, "Setting NTF Blocking .... Already Set!\n" );

   /* Check the circular queue to see if it is full.  If the queue 
      is full, the event will be ignored. */
   if( (((Event_queue_tail+1) % MAX_QUEUE_SIZE) + 1) == Event_queue_head ){

      LE_send_msg( GL_INFO, "Event Queue Is Full. Event EVT_USER_NOTIFCATION Lost.\n" );

      /* Unblock external events. */
      block_state = LB_NTF_control( LB_NTF_UNBLOCK );
      if( block_state != 1 )
         LE_send_msg( GL_ERROR, "Resetting NTF Blocking .... Already Reset!\n" );

      return;

   }

   /* Loop through the event register list.  When a match is found with the 
      desired event, save its event service routine, its event id, and 
      queued parameter. */
   for( i = 0; i < Event_list.num_events; i++ ){

      if ( (EVT_USER_NOTIFICATION == Event_list.code[i]) 
                  &&
           (data_id == ORPGDA_lbfd( Event_list.queued_parameter[i]))
                  &&
           ((msg_id == Event_list.msg_id[i]) || (Event_list.msg_id[i] == LB_ANY)) ){

         /* Increment the event queue tail and place this event at this 
            queue position. */
         Event_queue_tail = (Event_queue_tail % MAX_QUEUE_SIZE) + 1;

         Event_queue[Event_queue_tail].msglen = 0;
         Event_queue[Event_queue_tail].service_routine = Event_list.service_routine[i];
         Event_queue[Event_queue_tail].queued_parameter = Event_list.queued_parameter[i];
         Event_queue[Event_queue_tail].msg_id = msg_id;
         Event_queue[Event_queue_tail].code = Event_list.code[i];

         break;

      }

   } 

   /* If this task is not event based, service this event immediately
      and any other events that may be queued. */
   if( !Evt_wait_for_event )
      Service_event_queue();

   /* Unblock external events. */
   block_state = LB_NTF_control( LB_NTF_UNBLOCK );
   if( block_state != 1 )
      LE_send_msg( GL_ERROR, "Resetting NTF Blocking .... Already Reset!\n" );
 
   /* Exit module. */
   return;

/* End of UN_handler( ) */
}

/*******************************************************************
  
   Description:
      This function called when the EVT_WAIT_FOR_EVENT_TIMEOUT event 
      occurs.  The flag Evt_wait_for_event_timeout flag is set. 

*******************************************************************/
void Timeout_handler( malrm_id_t *alarm_id ){

   Evt_wait_for_event_timeout = 1;

/* End of Timeout_handler() */
}

/********************************************************************

   Description: This function is used to control the processing of
                events registered by the task.

                The event queue is examined.  If an event is on the 
                queue, the event service routine associated
                with the event is called.  The queued parameter 
                associated with the registering of this event is 
                passed as an argument.

   Return:      Returns the number of events serviced.

********************************************************************/
int Service_event_queue(){

   static int *queued_parameter;
   static LB_id_t *msg_id;
   int events_serviced;

   /* Initialize the number of events serviced this pass. */
   events_serviced = 0;

   while( (Event_queue_tail % MAX_QUEUE_SIZE) + 1 != Event_queue_head ){

      /* Event has been found in the event queue.  Check if adaptation
         data updates are required. */
#ifdef RPG_LIBRARY
      ADP_update_adaptation_by_event( Event_queue[Event_queue_head].code );
#endif
      /* Check if ITC updates are required. */
      ITC_update_by_event( Event_queue[Event_queue_head].code );

      /* Now service the event by calling event service routine. */
      queued_parameter = (int *) &Event_queue[Event_queue_head].queued_parameter; 
      msg_id = (LB_id_t *) &Event_queue[Event_queue_head].msg_id; 

      if( Event_queue[Event_queue_head].code == EVT_CFCPROD_REPLAY_PRODUCT_REQUEST )
         Special_clutprod_event_processing( Event_queue[Event_queue_head].msg,
                                            Event_queue[Event_queue_head].msglen );

      else if( Event_queue[Event_queue_head].code == ORPGEVT_END_OF_VOLUME ){

         if( Event_queue[Event_queue_head].service_routine != NULL )
            Event_queue[Event_queue_head].service_routine( 
                        Event_queue[Event_queue_head].queued_parameter,
                        Event_queue[Event_queue_head].msg,
                        Event_queue[Event_queue_head].msglen,
                        Evt_wait_for_event );

      }
      else if( Event_queue[Event_queue_head].code == EVT_USER_NOTIFICATION ){

         if( Event_queue[Event_queue_head].service_routine != NULL ){

#ifdef RPG_LIBRARY
            Event_queue[Event_queue_head].service_routine( queued_parameter, msg_id );
#else
#ifdef RPGC_LIBRARY
            Event_queue[Event_queue_head].service_routine( *queued_parameter, *msg_id );
#endif
#endif

         }

      }
      else{

         if( Event_queue[Event_queue_head].service_routine != NULL ){

#ifdef RPG_LIBRARY
            Event_queue[Event_queue_head].service_routine( queued_parameter );
#else
#ifdef RPGC_LIBRARY
            Event_queue[Event_queue_head].service_routine( *queued_parameter );
#endif
#endif
         }

      }

      /* Set message length to 0. */
      if( Event_queue[Event_queue_head].msglen > 0 )
         Event_queue[Event_queue_head].msglen = 0; 

      /* Increment the event queue head for next item in queue. */
      Event_queue_head = (Event_queue_head % MAX_QUEUE_SIZE) + 1;

      /* Increment the number of events serviced this pass. */
      events_serviced++;

   }

   return( events_serviced );

/* End of Service_event_queue() */
}

/**********************************************************************

   Description:
      Module checks for internally defined events.

   Inputs:

   Outputs:

   Returns:

   Notes:

**********************************************************************/
static void Check_for_internal_events(){

   int i, input_avail, wait_for = WAIT_ANY_INPUT;

   /* Do For All registered events. */
   for( i = 0; i < Event_list.num_events; i++ ){

      /* Process only if internally defined event. */
      if( Event_list.internal_or_external[i] == EVT_INTERNAL_EVENT ){

         switch( Event_list.code[i] ){

            case EVT_ANY_INPUT_AVAILABLE:
            {

               WA_wait_for_any_data( &wait_for, &input_avail, 
                                     EVT_NO_WAIT_FOR_DATA );

               /* If input available, add event to event list. */
               if( input_avail == INPUT_AVAILABLE )
                  Event_handler( Event_list.code[i], NULL, 0 );

               break;
            }

            case EVT_CFCPROD_REPLAY_PRODUCT_REQUEST:
            {

               Prod_request *prod_req;
               int num_reqs;

               prod_req = PRQ_check_for_replay_requests( &num_reqs );
               if( num_reqs > 0 ){

                  Event_handler( Event_list.code[i], (char *) prod_req, 
                                 (int) num_reqs*sizeof(Prod_request) );

               }
               break;

            }

            case EVT_WAIT_FOR_EVENT_TIMEOUT:
            {

               if( Evt_wait_for_event_timeout ){

                  int flag = -1;       /* Indicates interval timer. */
                  int status = 0, count = Event_list.queued_parameter[i]*1000;

                  Evt_wait_for_event_timeout = 0;

                  /* Set the timeout value again. */
                  if( Task_type != TASK_EVENT_BASED )
                     a3cm40( &Event_list.code[i], &count, &flag, &status );

                  /* Add this event to event list. */
                  Event_handler( Event_list.code[i], NULL, 0 );

               }
               break;

            }

            default:
            {
 
               PS_task_abort( "Unsupported Internal Event Registered\n" );
               break;

            }

         /* End of "switch" statement. */
         }

      /* End of "for" loop. */
      }

   }

/* End of Check_for_internal_events() */
}

/**********************************************************************

   Description:
      Processing module for the cfcprod replay product request.

   Inputs:
      msg - pointer to buffer containing product request(s)
      msglen - length of product request message.

   Outputs:

   Returns:

   Notes:
      The queued parameter returned to the application is the registered
      queued parameter plus the request bit map.  See RPG/APUP ICD for
      more details.

***********************************************************************/
static void Special_clutprod_event_processing( char *msg, int msglen ){

   Prod_request *req = (Prod_request *) msg;
   int num_req = msglen / sizeof(Prod_request);
   static int queued_parameter;
   unsigned int vol_seq_num = ORPGVST_get_volume_number();

   /* Do For All requests. */
   while( (num_req > 0) && (req->pid == CFCPROD) ){

      /* Send to application. */
      queued_parameter = Event_queue[Event_queue_head].queued_parameter;
      queued_parameter += req->param_1;
      Event_queue[Event_queue_head].service_routine( &queued_parameter );

      /* Go to next request. */
      num_req--;
      req++;

   /* End of "while" loop. */
   }

   /* Clear abort product list. */
   AP_init_prod_list( vol_seq_num );

/* End of Special_clutprod_event_processing. */
}
