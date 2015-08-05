/********************************************************************

	This module contains initialization functions for the 
	old RPG product generaton tasks.

********************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/11/28 15:27:40 $
 * $Id: rpg_init.c,v 1.36 2012/11/28 15:27:40 steves Exp $
 * $Revision: 1.36 $
 * $State: Exp $
 */

#include <rpg.h>


/* properties of this task */
int Task_type;		/* ELEVATION_BASED or VOLUME_BASED */

/* for command line argument processing */
#define ARG_SIZE 64
#define N_ARGS	20

static int Argc = 0;		        /* the argc variable */
static char *Argv [N_ARGS];	        /* the argv array */
static char Arg_buffer [ARG_SIZE * N_ARGS] = "";
				        /* buffer for argv array */
static int Arg_buf_off = 0;	        /* offset of free space in Arg_buffer */

static int Task_terminating = 0;        /* Set if task is terminating. */

static int Task_input_stream = PGM_REALTIME_STREAM; 
                                        /* Whether task is realtime or
                                           replay.  Default is realtime. */
static char Task_name[ORPG_TASKNAME_SIZ];	/* Contains the rpg task name. */

static int Log_services_init = 0;	/* Indicates whether log error
                                           services has been initialized
                                           or not. */

static int Log_file_nmsgs = 1000;       /* Number of messages in the task's 
                                           log file. */

typedef struct End_of_vol_info{

   char task_name[ORPG_TASKNAME_SIZ];	/* Task name. */

   int vol_aborted;   			/* Flag, if set, indicates volume did not
                                           complete. */
   unsigned int vol_seq_num;		/* Volume scan sequence number. */

   unsigned int expected_vol_dur;	/* Expected volume scan duration, in 
					   seconds. */

} End_of_vol_info_t;

static End_of_vol_info_t Eov_info;
static int End_of_vol_flag = 0;


/* local functions */
static int Parse_args (void);
static int Term_handler( int signal, int dont_care );
void EOV_event_handler( int code, void *event_data, size_t data_size,
                        int task_event_based );

/* Public Functions. */

/********************************************************************

   Description:
      Initializes the log error services.  Sets flag to indicate log
      error services have been initialized.

   Inputs:

   Outputs:

   Returns:
      Always returns 0.

   Notes:
********************************************************************/
int RPG_init_log_services_c(){

    int retval;

    /* If log services already initialized, just return. */
    if( Log_services_init )
       return 0;

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

    return 0;

/* End of RPG_init_log_services_c() */
}

/********************************************************************

    Description: 
       This function sets the task type, ELEVATION_BASED or VOLUME_BASED, 
       and initializes the supporting modules.

    Inputs:
       what_based - defines the basis for this task (i.e., is it VOLUME_
                    BASED, ELEVATION_BASED etc.)

    Outputs:

    Return:	Return value is never used.

    Notes:

********************************************************************/
int RPG_task_init_c (int *what_based){

    int event_code, queued_parameter ;
    Orpgtat_entry_t *task_entry;

    /* Initialize the LE service ... instance of '-1' indicates this
       is NOT a multiple-instance task (we assume all legacy tasks are
       single-instance tasks) ... */
    RPG_init_log_services_c();

    /* Set up task type */
    if( (*what_based < TASK_ELEVATION_BASED) 
                     || 
        (*what_based > TASK_EVENT_BASED)){

       PS_task_abort( "Invalid Task Timing Specified (%d)\n",
                      *what_based );

    }
    else
       Task_type = *what_based;

    /* Determine if this task is realtime or replay. */
    task_entry = ORPGTAT_get_entry( (char *) &(Task_name[0]) );
    if( task_entry != NULL ){

       char *substr = strstr( &task_entry->task_name[0], "replay" );

       /* If the task_name contains the substring replay, then it is a
          replay task .... otherwise, it is a realtime task. */
       if( substr == NULL ){

          LE_send_msg( GL_INFO, "Task %s Input Data Stream Is Real-time\n",
                       task_entry->task_name );
          Task_input_stream = PGM_REALTIME_STREAM;

       }
       else{

          LE_send_msg( GL_INFO, "Task %s Input Data Stream Is Replay\n",
                       task_entry->task_name );
          Task_input_stream = PGM_REPLAY_STREAM;

       }

    }
    else
       PS_task_abort( "No Task Table Entry For %s\n", Task_name );

    /* Initialize other supporting modules */
    IB_initialize( task_entry );
    OB_initialize( task_entry );

    /* Must come after OB_initialize() */
    PRQ_initialize();
    VI_initialize();
    WA_initialize( task_entry );
    ADP_initialize();
    ADE_initialize();
    AP_initialize();
    SS_initialize();
    ITC_initialize();
    ES_initialize();

    if( task_entry != NULL )
       free( task_entry );

    /* Register termination signal handler */
    ORPGTASK_reg_term_handler( Term_handler );

    /* Register for ORPGEVT_END_OF_VOLUME" event. */
    event_code = ORPGEVT_END_OF_VOLUME;
    queued_parameter = ORPGEVT_END_OF_VOLUME;
    if( RPG_reg_for_external_event( &event_code, EOV_event_handler, 
                                    &queued_parameter ) < 0 ){

       LE_send_msg( GL_INFO, "Task %s Unable To Report CPU Utilization\n",
                    (char *) &Task_name[0] );

    }

    /* Inform the operator that this task is active. */
    LE_send_msg( GL_INFO, "Task %s Is Active\n", (char *) &Task_name[0] );

    /* Activate message mode. */
    PS_message_mode ();

    return (0);

/*END of RPG_task_init_c()*/
}

/* Private Functions. */

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

/********************************************************************

    Description: 
       This function receives the command line arguments from the 
       FORTRAN main subbroutine and rebuilds the the C Argv list.

    Input:
	argv - one argv string
	len - max length of the argv buffer

    Outputs:

    Returns:	
        return value is never used.

    Notes:

*****************************************************************/
int INIT_process_argv (char *argv, int *len){

    int i;

    /* Do for length of the length. */
    for (i = 0; i < *len; i++){

        /* If value is not alphanumeric, set to NULL. */
	if ((int)argv [i] <= 32){

	    argv [i] = '\0';
	    break;

	}

    }
    argv [*len - 1] = '\0';

    if (Argc >= N_ARGS || Arg_buf_off + *len >= ARG_SIZE * N_ARGS)
	PS_task_abort ("Too many command line options specified\n");

    Argv [Argc] = Arg_buffer  + Arg_buf_off;
    strcpy (Argv [Argc], argv);
    Arg_buf_off += strlen (argv) + 1;
    Argc++;

    return (0);

/*END of INIT_process_argv()*/
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

    strcpy( (char *) &(Task_name[0]), Argv[0] );

    err = 0;
    while ((c = getopt (Argc, Argv, "T:l:h")) != EOF) {
	switch (c) {

            case 'T':
               break;

            case 'l':
               Log_file_nmsgs = atoi( optarg );
               fprintf( stdout, "log file number messages %d\n", Log_file_nmsgs );
               if( Log_file_nmsgs < 0 || Log_file_nmsgs > 5000 )
                  Log_file_nmsgs = 500;
               break;
 
	    case 'h':
	    case '?':
	    default:
		err = 1;
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
void INIT_set_arguments( int argc, char *argv[] ){

   int i;
  
   /* Set the number of command line arguments. */
   Argc = argc;
  
   /* Transfers the command line arguments. */
   for( i = 0; i < argc; i++ )
      Argv[i] = argv[i];
   
/* End of INIT_set_arguments */
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
   RPG_get_abort_reason( &reason );
   
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

