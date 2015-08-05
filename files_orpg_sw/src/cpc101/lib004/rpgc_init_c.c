/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/11/28 15:27:41 $
 * $Id: rpgc_init_c.c,v 1.28 2012/11/28 15:27:41 steves Exp $
 * $Revision: 1.28 $
 * $State: Exp $
 */

#include <rpgc.h>

#include <orpgerr.h>
#include <infr.h>
#include <missing_proto.h>

/* Properties of this task. */
int Task_type;           				/* TASK_ELEVATION_BASED, TASK_VOLUME_BASED,
                                   			   TASK_EVENT_BASED, etc. */
/* For command line argument processing. */
#define ARG_SIZE 					128
#define N_ARGS  					20

static int Argc = 0;                    		/* The argc variable */
static char *Argv [N_ARGS];             		/* The argv array */
static int Task_terminating = 0;        		/* Set if task is terminating. */

static int Task_input_stream = PGM_REALTIME_STREAM; 	/* Whether task is realtime or
                                           		   replay.  Default is realtime. */
static char Task_name[ORPG_TASKNAME_SIZ];       	/* Contains the rpg task name. */

static int Log_services_init = 0;       		/* Indicates whether log error
                                           	   	   services has been initialized
                                           	   	   or not. */

static int Log_file_nmsgs = 1000;        		/* Number of messages in the task's
                                           	   	   log file. */

typedef struct End_of_vol_info{

   char task_name[ORPG_TASKNAME_SIZ];   		/* Task name. */

   int vol_aborted;                     		/* Flag, if set, indicates volume did not
                                           		   complete. */
   unsigned int vol_seq_num;            		/* Volume scan sequence number. */

   unsigned int expected_vol_dur;       		/* Expected volume scan duration, in
                                           		   seconds. */

} End_of_vol_info_t;

static End_of_vol_info_t Eov_info;
static int End_of_vol_flag = 0;

/* For user-defined command line processing. */
static char RPGC_options[ARG_SIZE];			/* Command line additional options. */
static int Initialized = 0;				/* Module initialization flag. */

static RPGC_options_callback_t RPGC_options_callback;

/* Storage for log message. */
static char buf[ LE_MAX_MSG_LENGTH ];  

/* Supports the use of GL_ERROR when calling RPGC_log_msg(). */
static char *INIT_src_name = NULL;
static int INIT_line_num = 0;

/* Local Function Prototypes. */
static int Parse_args (void);
static int Term_handler( int signal, int dont_care );
void EOV_event_handler( int code, void *event_data, size_t data_size,
                        int task_event_based );
static void INIT_set_arguments( int argc, char *argv[] );


/****************************************************************
   Description:
      C/C++ interface to allow registering inputs and outputs
      with a single function call.

   Inputs:
      argc - number of command line arguments
      argv - command line arguments 

   Returns:
      If any problem occurs in registering the inputs and
      outputs, then the PS_task_abort is called.  Otherwise,
      always returns 0.

   Notes:
      Optional inputs can still be specified after this call.

****************************************************************/
int RPGC_reg_io( int argc, char *argv[] ){

   int ret;

   /* Register all inputs. */
   ret = RPGC_reg_inputs( argc, argv );
   if( ret < 0 )
      PS_task_abort( "Unable to RPGC_reg_inputs()\n" );

   /* Register all outputs. */
   ret = RPGC_reg_outputs( argc, argv );
   if( ret < 0 )
      PS_task_abort( "Unable to RPGC_reg_outputs()\n" );

   return (0);
 
/* End of RPGC_reg_io() */
}

/****************************************************************
   Description:
      C/C++ interface for initializing log-error services.

   Inputs:
      argc - number of command line arguments.
      argv - command line arguments.

   Returns:
      Always returns 0.

   Notes:
      Allows this service to be initialized before the 
      RPGC_task_init call. 

****************************************************************/
int RPGC_init_log_services( int argc, char *argv[] ){

   int retval;

   /* If log services already initialized, just return. */
   if( Log_services_init )
      return 0;

   /* Set the command line arguments. */
   INIT_set_arguments( argc, argv );

   /* Parse command line options. */
   Parse_args();

   /* Initialize the LE service ... instance of '-1' indicates this
      is NOT a multiple-instance task (we assume all legacy tasks are
      single-instance tasks) ... */
   ORPGMISC_init( Argc, Argv, Log_file_nmsgs, 0, -1, 0);

   /* Get my task name (not necessarily my executable name) */
   retval = ORPGTAT_get_my_task_name( (char *) &Task_name[0],
                                      ORPG_TASKNAME_SIZ );

   if( retval < 0 )
      PS_task_abort( "ORPGTAT_get_my_task_name() Failed\n" );

   Log_services_init = 1;

   return (0);

/* End of RPGC_init_log_services() */
}

/****************************************************************
   Description:
      C/C++ interface for retrieving the input data stream (either
      realtime or replay).

   Inputs:
      argc - number of command line arguments.
      argv - command line arguments.

   Returns:
      Returns the stream on success, or -1 on error.

   Notes:
      The input data stream can be set in 2 ways:  The input_stream
      key-word can be set in the task_attr_table or the name
      of the task can differentiate between replay and realtime...
      that is, replay tasks start with "replay_". 

****************************************************************/
int RPGC_get_input_stream( int argc, char *argv[] ){

    int retval;
    Orpgtat_entry_t *task_entry = NULL;
    char task_name[ORPG_TASKNAME_SIZ];

   /* This call is required to initialize RPG infrastructure data. */
   RPGC_init_log_services( argc, argv );

   /* Get my task name (not necessarily my executable name) */
   if( (retval = ORPGTAT_get_my_task_name( (char *) &task_name[0],
                                           ORPG_TASKNAME_SIZ )) < 0 )
      return retval;


    /* Determine if this task is realtime or replay. */
    task_entry = ORPGTAT_get_entry( (char *) &task_name[0] );
    if( task_entry != NULL ){

       char *substr = strstr( &task_entry->task_name[0], "replay" );

       /* If the task_name contains the substring replay, then it is a
          replay task.  If the TAT defines the data stream, use it.  Otherwise, 
          by default, it is a realtime task. */
       if( (substr == NULL) && (task_entry->data_stream != ORPGTAT_REPLAY_STREAM) ){

          LE_send_msg( GL_INFO, "Task %s Input Data Stream Is Real-time\n",
                       task_entry->task_name );
          Task_input_stream = PGM_REALTIME_STREAM;

       }
       else if( (substr != NULL) || (task_entry->data_stream == ORPGTAT_REPLAY_STREAM) ){

          LE_send_msg( GL_INFO, "Task %s Input Data Stream Is Replay\n",
                       task_entry->task_name );
          Task_input_stream = PGM_REPLAY_STREAM;

       }
       else{

          LE_send_msg( GL_INFO, "Task %s Input Data Stream Is Real-time (By Default)\n",
                       task_entry->task_name );
          Task_input_stream = PGM_REALTIME_STREAM;

       }

    }
    else{

       LE_send_msg( GL_INFO, "No Task Table Entry For %s\n", Task_name );
       return -1;

    }

    /* Return the task input stream. */
    return( Task_input_stream );

/* End of RPGC_get_input_stream(). */
} 

/********************************************************************
			
   Description: 
      This is a simplified version of LE_send_msg(). See LE manpage. 

   Input:  
      code - either GL_INFO, GL_ERROR, or GL_STATUS
      format - a format string used for generating the message. The 
               format is used as the same way as that used in printf.
      ... - the variable list.

********************************************************************/
void RPGC_log_msg( int code, const char *format, ... ){

    va_list args;

    /* Create text string from format and variable arguments. */
    va_start( args, format );
    vsprintf( buf, format, args );
    va_end( args );

    /* Pass string to LE library. */
    LE_send_msg( code, buf );

    return;
}

/****************************************************************
   Description:
      C/C++ interface for RPG_task_init_c.

   Inputs:
      what_based - either VOLUME_BASED, ELEVATION_BASED, etc.
      argc - number of command line arguments.
      argv - command line arguments.

   Returns:
      Returns value from RPG_task_init_c.  See rpg infrastructure
      library man page for more details.

****************************************************************/
int RPGC_task_init( int what_based, int argc, char *argv[] ){

    int event_code, queued_parameter ;
    Orpgtat_entry_t *task_entry = NULL;;

    INIT_set_arguments( argc, argv );

    /* Initialize the command line arguments module.  NOTE: This
       needs to be called before RPGC_init_log_services. */
    CO_initialize();

    /* Initialize the LE service ... instance of '-1' indicates this
       is NOT a multiple-instance task (we assume all legacy tasks are
       single-instance tasks) ... */
    RPGC_init_log_services( argc, argv );

    /* Set up task type */
    if( (what_based < TASK_ELEVATION_BASED)
                     ||
        (what_based > TASK_EVENT_BASED)){

       PS_task_abort( "Invalid Task Timing Specified (%d)\n",
                      what_based );

    }
    else
       Task_type = what_based;

    /* Determine if this task is realtime or replay. */
    task_entry = ORPGTAT_get_entry( (char *) &(Task_name[0]) );
    if( task_entry != NULL ){

       char *substr = strstr( &task_entry->task_name[0], "replay" );

       /* If the task_name contains the substring replay, then it is a
          replay task.  If the TAT defines the data stream, use it.  Otherwise, 
          by default, it is a realtime task. */
       if( (substr == NULL) && (task_entry->data_stream != ORPGTAT_REPLAY_STREAM) ){

          LE_send_msg( GL_INFO, "Task %s Input Data Stream Is Real-time\n",
                       task_entry->task_name );
          Task_input_stream = PGM_REALTIME_STREAM;

       }
       else if( (substr != NULL) || (task_entry->data_stream == ORPGTAT_REPLAY_STREAM) ){

          LE_send_msg( GL_INFO, "Task %s Input Data Stream Is Replay\n",
                       task_entry->task_name );
          Task_input_stream = PGM_REPLAY_STREAM;

       }
       else{

          LE_send_msg( GL_INFO, "Task %s Input Data Stream Is Real-time (By Default)\n",
                       task_entry->task_name );
          Task_input_stream = PGM_REALTIME_STREAM;

       }

    }
    else
       PS_task_abort( "No Task Table Entry For %s\n", Task_name );

    /* Initialize other supporting modules */
    IB_initialize( task_entry );
    OB_initialize( task_entry );

    /* This must be called after OB_initialize(). */
    PRQ_initialize();

    /* We call VI_initialize to initialize ORPGDAT_ADAPTATION for
       write permission.   This is to prevent a possible task failure
       later. */
    VI_initialize();
    ADP_initialize();
    ADE_initialize();
    AP_initialize();
    SS_initialize();
    ITC_initialize();
    ES_initialize();

    /* This should always be the last module to be initialized. */
    WA_initialize( task_entry );

    if( task_entry != NULL )
       free( task_entry );

    /* Register termination signal handler */
    ORPGTASK_reg_term_handler( Term_handler );

    /* Register for ORPGEVT_END_OF_VOLUME" event. */
    event_code = ORPGEVT_END_OF_VOLUME;
    queued_parameter = ORPGEVT_END_OF_VOLUME;
    if( RPGC_reg_for_external_event( event_code, EOV_event_handler,
                                     queued_parameter ) < 0 ){

       LE_send_msg( GL_INFO, "Task %s Unable To Report CPU Utilization\n",
                    (char *) &Task_name[0] );

    }

    /* Inform the operator that this task is active. */
    LE_send_msg( GL_INFO, "Task %s Is Active\n", (char *) &Task_name[0] );

    return (0);

/* End of RPGC_task_init( ) */
}

/****************************************************************

   Description:
      C/C++ interface for conveniently registering inputs, 
      outputs, then task initialization.

   Inputs:
      what_based - either VOLUME_BASED, ELEVATION_BASED, etc.
      argc - number of command line arguments.
      argv - command line arguments.

   Outputs:

   Returns:
      Returns value from RPG_task_init_c.  See rpg infrastructure
      library man page for more details.  On error, returns
      -1.

   Notes:
      This function is only intended for use by algorithms not
      requiring adaptation data registration, or optional inputs,
      or scan summary registration, or moment registration.

*****************************************************************/
int RPGC_reg_and_init( int what_based, int argc, char *argv[] ){

   int ret;

   /* Initialize the log services before anything else. */
   RPGC_init_log_services( argc, argv );

   /* Register all inputs and outputs. */
   if( (ret = RPGC_reg_io( argc, argv )) >= 0 ){

      /* Do task initialization. */
      return( RPGC_task_init( what_based, argc, argv ) );

   }

   return(-1);

/* End of RPGC_reg_and_init(). */
} 

/****************************************************************

   Description:
      C/C++ interface for preparing a task to use pthreads.  
      This function only needs to be called when using pthreads.
      It must be the first function called in the main routine.
      See Notes for more details.

   Inputs:

   Outputs:

   Returns:
      Returns void.

   Notes:
      EN_control (EN_SET_SIGNAL, (int) new_signal) directs the EN to use signal
      "new_signal". This function can only be called when there is no EN
      registered.  That is to be safe this function should be the first
      function called before any other functions are called.

      "new_signal" takes the value of EN_NTF_NO_SIGNAL, which sets the
      synchronous notification mode. In this mode, there is no signal to
      interrupt the application. All incoming events are queued until EN_control
      (EN_WAIT, ms) is called. EN_control (EN_WAIT, ms) causes all pending events
      to be delivered and returns immediately. If there is no pending event,
      EN_control (EN_WAIT, ms) blocks the application until an event comes. It
      then causes the event to be delivered and returns. The blocking times out
      after "ms" milliseconds.   The RPGC library calls EN_control( EN_WAIT, ... )
      in several places and therefore fully supports this functionality.

      The infrastructure library (libinfr) is not specifically designed to support
      pthreads.   Therefore use of pthreads without calling this function can 
      cause task failures.

*****************************************************************/
void RPGC_pthreads_init( ){

   EN_control( EN_SET_SIGNAL, EN_NTF_NO_SIGNAL );

/* End of RPGC_pthreads_init(). */
}

/*******************************************************************

   Description:
      Kills the RPG software if RPG is non-operational.  Otherwise
      does nothing.


   Notes:
      This function is intended to be used for testing purposes
      only.

*******************************************************************/
void RPGC_kill_rpg(){

   if( ORPGMISC_is_operational() )
      return;

   ORPGMGR_cleanup();

/* End of RPGC_kill_rpg(). */
}

/*******************************************************************

   Description:
       This function moves the command line arguments from a C/C++
       process to local storage Argc and Argv.

   Inputs:
       argc - number of command line arguments.
       argv - the command line arguments.

   Notes:
       Needed by the RPGC library.

*******************************************************************/
static void INIT_set_arguments( int argc, char *argv[] ){

   int i;

   /* Set the number of command line arguments. */
   Argc = argc;

   /* Transfers the command line arguments. */
   for( i = 0; i < argc; i++ )
      Argv[i] = argv[i];

/* End of INIT_set_arguments */
}

/********************************************************************

    Description:
       This function returns the task type.

    Return:
       The task type.

********************************************************************/
int INIT_task_type (void){

   return (Task_type);

/*END of INIT_task_type ()*/
}

/********************************************************************

    Description:
       This function returns the task's name.

    Return:
       The task name.

********************************************************************/
char* INIT_task_name (void){

   return( (char *) &Task_name[0] );

/*END of INIT_task_name ()*/
}

/*****************************************************************

    Description:
       This function returns the value of Task_terminating.

*****************************************************************/
int INIT_task_terminating(){

   return( Task_terminating );

}

/*****************************************************************

    Description:
       This function parses the command line options.

    Returns:
       Return value is never used.

*****************************************************************/
static int Parse_args (void){

    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    char options[ARG_SIZE];

    strcpy( (char *) &(Task_name[0]), Argv[0] );

    /* Set up command line options. */
    memset( options, 0, ARG_SIZE );
    strcat( options, "T:l:h" );
    if( (strlen( RPGC_options ) > 0) 
                  && 
        ((strlen("T:l:h") + strlen( RPGC_options )) <= ARG_SIZE) )
       strcat( options, RPGC_options );

    err = 0;
    while ((c = getopt (Argc, Argv, options)) != EOF) {
        switch (c) {

            case 'T':
               break;

            case 'l':
               Log_file_nmsgs = atoi( optarg );
               fprintf( stdout, "log file number messages %d\n", Log_file_nmsgs );
               if( Log_file_nmsgs < 0 || Log_file_nmsgs > 7500 )
                  Log_file_nmsgs = 500;
               break;

            case 'h':
            case '?':
                err = 1;
		break;

            default:
                /* Process customized command line options. */
                CO_process_custom_options( c );
                break;

        }
    }

    if (err == 1) {              /* Print usage message */
        printf ("Usage: %s [options]\n", Argv [0]);
        printf ("       -T Task Name\n" );
        printf ("       -l Log file Number LE Messages (%d)\n", Log_file_nmsgs);
        printf ("       -h (print usage)\n");
        exit (0);
    }

    return (0);

/*END of Parse_args()*/
}

/*************************************************************************

   Description:
      This function returns the task's input stream (realtime or replay)

*************************************************************************/
int INIT_get_task_input_stream(){

   return( Task_input_stream );

}

/************************************************************************

   Description:
      This function services the END_OF_VOLUME event, if event received.

************************************************************************/
int INIT_process_eov_event(){

   if( End_of_vol_flag ){

      End_of_vol_flag = 0;

      /* Report cpu statistics for the "vol_scan_num" volume. */
      OB_report_cpu_stats( Eov_info.task_name, Eov_info.vol_seq_num,
                           Eov_info.vol_aborted, Eov_info.expected_vol_dur );

   }

   return (0);

/* End of INIT_process_eov_event. */
}

/*************************************************************************

   Description:
      This is the task termination signal handler.  Calls abort outputs
      with reason PGM_TASK_FAILURE.

   Inputs:
      signal - Signal type.
      flag - Either GL_ABNORMAL_SIG or GL_NORMAL_SIG.

   Returns:
      There is no return value define for this function.

*************************************************************************/
static int Term_handler( int signal, int flag ){

   int reason, vol_scan_num = 0;
   unsigned int vol_stat_vol_seq_num = 0, vol_seq_num = 0;

   /* Set the "Task_terminating" flag. */
   Task_terminating = 1;

   /* Get the volume sequence number of the last input processed. */
   vol_seq_num = PS_get_current_vol_num( &vol_scan_num );

   /* Get the volume sequence number from volume status data. */
   PS_get_vol_stat_vol_num( &vol_stat_vol_seq_num );

   /* If the volume status data has a later volume sequence number
      and this task is waiting for activation, we want to terminate
      for the later volume. */
   if( (vol_stat_vol_seq_num > vol_seq_num)
                        &&
       WA_waiting_for_activation() )
      AP_set_aborted_volume( vol_stat_vol_seq_num );

   else
      AP_set_aborted_volume( vol_seq_num );

   if( flag == ORPGTASK_EXIT_ABNORMAL_SIG )
      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS,
                   "Task %s Terminating ... Abnormal Signal (%d) Caught\n",
                   (char *) &Task_name[0], signal );

   else if( flag == ORPGTASK_EXIT_NORMAL_SIG )
      LE_send_msg( GL_INFO,
                   "Task %s Terminating ... Normal Signal (%d) Caught\n",
                   (char *) &Task_name[0], signal );

   /* Use appropriate failure reason code. */
   reason = AP_get_abort_reason();

   /* Abort task outputs. */
   if( reason == PGM_TASK_SELF_TERMINATED )
      AP_abort_outputs( PGM_TASK_SELF_TERMINATED );

   else
      AP_abort_outputs( PGM_TASK_FAILURE );

   return(0);

/* End of Term_handler() */
}

/**********************************************************************

   Description:
      Module services the "END_OF_VOLUME" scan event.

   Inputs:
      code - the is the event code associated with the event.
      event_data - pointer to data associated with the event.  See
                   orpgevt.h for more details.
      data_size - size, in bytes, of the event data.
      task_event_based - if set, the task is event based so event should
                         be serviced immediately.

   Outputs:

   Returns:

   Notes:
      This module supports CPU gathering statistics for algorithms.

**********************************************************************/
void EOV_event_handler( int code, void *event_data, size_t data_size,
                        int task_event_based ){

   orpgevt_end_of_volume_t *report_cpu = event_data;

   /* Set end of volume flag. */
   End_of_vol_flag = 1;

   /* Save data associated with this event. */
   strcpy( Eov_info.task_name, Task_name );
   Eov_info.vol_aborted = report_cpu->vol_aborted;
   Eov_info.vol_seq_num = report_cpu->vol_seq_num;
   Eov_info.expected_vol_dur = report_cpu->expected_vol_dur;

   if( task_event_based ){

      /* Clear the end of volume flag. */
      End_of_vol_flag = 0;

      /* Report cpu statistics for the "vol_scan_num" volume. */
      OB_report_cpu_stats( Eov_info.task_name, Eov_info.vol_seq_num,
                           Eov_info.vol_aborted,
                           Eov_info.expected_vol_dur );

   }

/* End of EOV_event_handler() */
}

/**************************************************************************

   Description:
      Sets up internal data for parsing the command line arguments. 
      The command line arguments are processed via call to RPGC_task_init().

   Inputs:
      additional_options - user-defined command line arguments.
      callback - callback routine to service the user-defined
                 command line arguments.

   Returns:
      0 on success, -1 on error.

   Notes:
      This function must be called in the application before any call
      to RPGC_init_log_services.  Since RPGC_init_log_services can 
      be called explicitly and implicitly via multiple registration
      functions, it is best to call this function before any other
      RPGC function.

**************************************************************************/
int RPGC_reg_custom_options( const char *additional_options,
	                     RPGC_options_callback_t callback ){

    /* Check to ensure the arguments and callback function are defined. */
    if( (additional_options == NULL) || (strlen(additional_options) > ARG_SIZE)
                    ||
        (callback == NULL) ){

        return (-1);

    }

    /* Make local copy of argument list. */
    memset( RPGC_options, 0, ARG_SIZE );
    strcat( RPGC_options, additional_options );

    /* Save the command line argument parser address. */
    RPGC_options_callback = callback;

    /* Set the module initialized flag. */
    Initialized = 1;

    return 0;

/* End of RPGC_set_custom_options() */
}

/**************************************************************************

   Description:
      Read the command line options.   A user-defined routine is called
      for each command line option.

   Inputs:
      input - the command line argument.

   Returns:
      0 on success, or negative error code on failure. 

**************************************************************************/
int CO_process_custom_options( int input ){

    int ret;
    extern int optind;

    /* Ensure the command line arguments have been defined. */
    if( (strlen( RPGC_options ) <= 0) || (RPGC_options_callback == NULL) )
        return -1;

    /* Inform the user. */
    LE_send_msg( GL_INFO, "Parsing User-Defined Command Line Arguments.\n" );

    /* Process all command line arguments. */
    if( (ret = RPGC_options_callback( input, optarg ) ) < 0 )
       return ret;
       
    return 0;

/* End of CO_process_custom_options() */
}

/**************************************************************************

   Description:
      Initializes the Command Line Options module.

**************************************************************************/
void CO_initialize(){

    /* If not already initialized, initialize this module. */
    if( !Initialized ){

        memset( RPGC_options, 0, ARG_SIZE );
        RPGC_options_callback = NULL;

    }

    Initialized = 1;

/* End of CO_initialize() */
}

/*************************************************************************

   Description:
      Overrrides the definition of GL_ERROR.   Algorithms should not
      write to the error log.

   Inputs:
      file = file name.
      line_num - line number.

   Returns:
      Always returns 0.

*************************************************************************/
int INIT_file_line( char *file, int line_num ){

   INIT_src_name = file;
   INIT_line_num = line_num;
   return GL_INFO;

/* End of INIT_file_line. */
}
