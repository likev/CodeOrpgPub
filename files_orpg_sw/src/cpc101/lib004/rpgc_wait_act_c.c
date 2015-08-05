/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/09 22:53:52 $
 * $Id: rpgc_wait_act_c.c,v 1.34 2014/12/09 22:53:52 steves Exp $
 * $Revision: 1.34 $
 * $State: Exp $
 */

#include <rpgc.h>
#include <rpgcs.h>

/* Macro Definitions. */
#define UNDEFINED -1                    /* undefined azi_num and cut_num */

/* Static Global Variables. */
static In_data_type *Inp_list = NULL;   /* Input data type list */
static int N_inps = 0;                  /* Number of input data types */

static Out_data_type *Out_list = NULL;  /* Output data type list */
static int N_outs = 0;                  /* Number of output data types */

static int Task_type;                   /* Type of this task.  Can be
                                           either TASK_TIME_BASED, TASK_ELEVATION_BASED,
                                           or TASK_VOLUME_BASED. */

static int Monitor_only;		/* Whether this task requires scheduled
                                           outputs in order to run. */

static int Alg_control_flg = PBD_ABORT_NO;
                                        /* The algorithm control flag from
                                           basedata header.  Initialized to
                                           indicate no abort condition. */

static unsigned int Aborted_vol_seq_num = 0;
                                        /* Set in conjunction with Alg_control_flg,
                                           indicates the volume with is to be aborted. */

static unsigned int Vol_seq_num_wait_all = 0;
                                        /* The volume sequence number of the
                                           most recently read driving input.
                                           Used only for processes whose
                                           Wait_for = WAIT_DRIVING_INPUT. */
static int Resume_time = RESUME_UNDEFINED;
                                        /* Processing resumption time;
                                           (NEW_VOLUME, NEW_ELEVATION, NEW_DATA or
                                           RESUME_UNDEFINED) */

static int Driving_input_type = UNDEFINED_DATA;
                                        /* Driving input data type.  Can be
                                           either UNDEFINED_DATA, ELEVATION_DATA,
                                           VOLUME_DATA, or RADIAL_DATA. */

static int Bd_status = UNDEFINED;       /* Status of the current radial input.
                                           Can be either GOODBVOL, GOODBEL,
                                           GENDEL, GENDVOL, or INTERMEDIATE.  */

static int Wait_for;                    /* RPG_wait_act argument.  Can be either
                                           WAIT_DRIVING_INPUT or WAIT_ANY_INPUT. */
static int Wait_any_input_index;        /* WAIT_ANY_INPUT index to next
                                           available input. */

static int Input_stream = PGM_REALTIME_STREAM;
                                        /* Input stream for this task.  Currently,
                                           there are two input streams:
                                                PGM_REALTIME_STREAM
                                                PGM_REPLAY_STREAM. */

static int Vol_seq_num_wait_any = 0;    /* Volume sequence number currently being
                                           processed.  Only used for processes whose
                                           Wait_for = WAIT_ANY_INPUT. */

static int Old_vol_seq_num_wait_any = UNDEFINED;
                                        /* Previous volume volume sequence number.
                                           Only used by processes for which Wait_for =
                                           WAIT_ANY_INPUT. */
static int Old_elev_ind_wait_any = UNDEFINED;
                                        /* Elevation index of previous input.  Only used
                                           by processes for which Wait_for =
                                           WAIT_ANY_INPUT. */

static LB_status Status;                /* Input Linear Buffer status structure.
                                           Used by tasks which wait for WAIT_ANY_INPUT. */

static LB_info Info;                    /* Information about Linear Buffer.  Used by tasks
                                           which wait for WAIT_ANY_INPUT. */

static Replay_req_info_t Replay_info;   /* Replay requestion query information. */

enum{ NO = 0, YES = 1 };
static int Waiting_for_activation = YES;
                                        /* Flag, if set, indicates task is waiting for
                                           activation. */

static int Use_suppl_scans = 0; 	/* Use of supplemental scans is not allowed. The
                                           task_attr_table entry for this task provides the
                                           permission. */
  


/* Test variable description used in WA_check_data.  These are used for radial
   sequencing checks. */
typedef struct Rad_test {

   unsigned int vol_test_num;   /* current volume sequence number */
   int rda_cut_num;             /* current scan RDA cut number */
   int rpg_cut_num;             /* current scan RPG cut number */
   int azi_num;                 /* current radial number */
   int end_vol;                 /* end of volume detected */
   int end_elev;                /* end of elevation detected */
   time_t processed_time;       /* time of the data previously
                                   processed */
   int processed_azi_num;       /* azi_num of the data previously
                                   processed */
   int new_elev;
   int new_vol;
   int new_scan;                /* flags for new elevation, volume,
                                   and scan. */

   /* The following information supports normal termination of VCP
      prior to all elevation cuts being scanned (e.g., when AVSET is 
      active. Note: Used for non Radial driving inputs. */
   int vcp;                     /* VCP coverage pattern in use. */
   int n_exp_cuts;              /* Number of expected RDA cuts in VCP. */
   int n_cuts;                  /* Last RDA cut of VCP. */

} Rad_test_t;

typedef struct Vol_test {

   /* The following information supports normal termination of VCP
      prior to all elevation cuts being scanned (e.g., when AVSET is 
      active. Note: Used for non Radial driving inputs. */
   int vcp;                     /* VCP coverage pattern in use. */
   int n_exp_cuts;              /* Number of expected RPG cuts in VCP. */
   int n_cuts[MAXN_OUTS];       /* Last RPG cut a product was generated. */
   int end_vol[MAXN_OUTS];	/* 1 - End of volume, 0 - otherwise. */

} Vol_test_t;

static Rad_test_t Test = 
       { 0, UNDEFINED, UNDEFINED, UNDEFINED, TRUE, FALSE, 0, UNDEFINED, 0, 0, 0,
         UNDEFINED, UNDEFINED, UNDEFINED };

static Vol_test_t Vtest = 
       { UNDEFINED, UNDEFINED,
         {UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED},
         {UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED} };

/* Local functions */
static int Wait_act( int wait_for );
static void Initialize ();
static int Output_requested ();
static int Check_data_prod( int buffer_ind );
static void Wait_for_any_init( int wait_for );
static void Update_support_environment( Base_data_header *bhd,
                                        time_t tm, int new_vol,
                                        Prod_header *phd );
static int Wait_for_replay_request();
static int Check_radial_replay_availability( Prod_request *requests );
static int Set_query_info( Prod_request *requests );
static void Abort_replay_request( Prod_request *request );
static int Check_alg_control( Base_data_header *bhd );
static int Radial_seq_check_pass_1( Rad_test_t *test, Base_data_header *bhd );
static int Radial_seq_check_pass_2( Rad_test_t *test, Base_data_header *bhd );
static void Init_test_vars( Rad_test_t *test, unsigned int vol_num,
                            int end_vol, int end_elev );
static int Check_avset();
static int Replay_find_closest_elev_ind( Prod_request *requests, 
                                         RDA_vcp_info_t *rdacnt );


/***************************************************************
   Description:
      C/C++ interface for waiting for activation for
      WAIT_DRIVING_INPUT.  

   Inputs:
      wait_for - must be WAIT_DRIVING_INPUT.  

   Returns:
      Always returns 0.

***************************************************************/
int RPGC_wait_act( int wait_for ){

   if( wait_for == WAIT_DRIVING_INPUT ){

      /* Wait for activation. */
      return( Wait_act( wait_for ) );

   }
   else
      PS_task_abort("Illegal call to RPG_wait_act( WAIT_ANY_INPUT )\n"); 

   return 0;
 
/* End of RPGC_wait_act( ) */
}

/***************************************************************
   Description:
      C/C++ interface for RPG_wait_for_any_data.  

   Inputs:
      wait_for - either WAIT_DRIVING or WAIT_ANY_INPUT.  

   Returns:
      Returns value from RPG_wait_act call.  See rpg 
      infrastructure library man page for more details.

***************************************************************/
int RPGC_wait_for_any_data( int wait_for ){

   int parm = 0;

   return( WA_wait_for_any_data( wait_for, &parm, EVT_WAIT_FOR_DATA ) );

/* End of  RPGC_wait_for_any_data( int wait_for ) */
}

/********************************************************************
   Description:
      This functions always a task to suspend further processing
      until the task specified by "task_name" is active.  

   Inputs:
      task_name - string representing task name of the task which
                  we are waiting for.  It is assume this string
                  is terminated with '$' character.

   Outputs:

   Returns:
      When it returns, always returns 0.

   Notes:
      This functions does not suspend if:
         1) the input string task name is not terminated with '$'
         2) the task is not found in the Task Attribute Table (TAT)
         3) task status is not available.

********************************************************************/
int RPGC_wait_for_task( char *task_name ){

   int task_state;

   /* Suspend until task status is active. */
   while(1){

      /* Read the task status. */
      if( ORPGMGR_get_task_status( task_name, 0, &task_state ) < 0 )
         return 0;

      /* If task is active, return to caller. */
      if( task_state == MRPG_PS_ACTIVE )
         break;

      /* Sleep a short period of time and try again. */
      EN_control (EN_WAIT, 1000);

   }
   
   return 0;

/* End of RPGC_wait_for_task() */
}

/* Private Functions. */

/********************************************************************

    Description: 
         This function is used for controlling the processing loop 
         of RPG product/algorithm tasks which are data driven or 
         request driven in the case of replay product requests. 
         For real-time products and replay products whose driving 
         input is not radial data or warehoused, this module suspends 
         data processing until the driving input is available and is 
         read.  For replay products whose driving input is radial
         data or the data is warehoused, processing is suspended
         pending a replay product request.  Once a request is 
         received, then the driving input is checked for availability.
         If not available, any requested outputs are aborted and the
         module wait for the next replay request.

         After the driving input has been read, if there is a data 
         sequencing error, the Aborted flag is set.  Module returns 
         if driving input is available, outputs are scheduled for 
         generation, or in the case of RADIAL_DATA driving input, 
         the data is needed (i.e., RELDATA vice COMBBASE).

	 Refer to support.doc for a complete description of the
	 function.

    Inputs:
	 what_for - must be WAIT_DRIVING_INPUT;

    Outputs:

    Returns:
	 Return value is never used.

********************************************************************/
static int Wait_act( int wait_for ){

    int ret, buffer_ind;
    int data_needed;
    int output_requested;
    int input_checked_out;

    /* Save this value as static global variable.  It is used 
       throughout this module. */
    Wait_for = wait_for;

    /* Set the waiting for activation flag. */
    Waiting_for_activation = YES;
    LE_send_msg( GL_INFO, "Waiting For Activation == TRUE\n" );

    /* Until driving input is available. */
    while (1) {

        /* Has the END_OF_VOLUME event been received? */
        INIT_process_eov_event();

        /* Check if previous volume has ended.   If so, were the
           number of elevation cuts in the VCP the same as what was
           expected based on the VCP definition (this would happen, 
           for example, if AVSET were running).  If so, then let's 
           abort volume scan for elevation-based products to clear
           out any product requests for unscanned cuts that will 
           never be satisfied. 

           Note:  Currently the only know reason a VCP would end 
           prematurely and not be unexpected is because of AVSET.  
           Normally the AVSET flag would be checked.   Unfortunately 
           this is not totally reliable when playing back data. */
        if( Input_stream == PGM_REALTIME_STREAM )
            Check_avset();

        /* If this is a replay task, then .... */
        if( Input_stream == PGM_REPLAY_STREAM ){

           /* Is driving input timing RADIAL_DATA or driving input 
              warehoused data?  

              NOTE:  The wait for data flag is not set if either of
                     these two conditions are meet.  Instead of 
                     waiting for the data, we wait for a request. */
           if( !Inp_list[DRIVING].wait_for_data ){

              if( Wait_for_replay_request() < 0 )
                 continue; 

           }

           /* Was the task previously aborted?  */
           if( AP_abort_flag( UNKNOWN ) ){

              /* For replay task consuming radial inputs, we do not care if the 
                 previous volume scan completed successfully or not.  We should
                 not have to abort the previous volume scan in this case since
                 if there had been a request in that volume scan, the data
                 would not have been available and the request would not have 
                 been satisfied.  */
              if( (Resume_time == NEW_DATA)
                               &&
                  (Inp_list[DRIVING].timing == RADIAL_DATA) ){

                 LE_send_msg( GL_INFO, "Setting Test.end_vol == TRUE for Replay Task\n" );
                 Test.end_vol = TRUE;

              }

           }

        }

	/* Read next driving input data; For the real-time data stream,
           this will block until the data is available.  In the case of
           the replay data stream, this returns immediately if read 
           fails for any reason (other than a fatal error) and driving
           input is radial data or warehoused data.  For replay tasks 
           whose driving input is neither of these, then this module will
           block until the data is available. */
	ret = IB_read_driving_input ( &buffer_ind );
 
        if( Input_stream == PGM_REPLAY_STREAM ){

           /* Did IB_read_driving_input return NO_DATA?  If so,
              set abort reason.  The abort flag will be set later since
              WA_check_data returns non-NORMAL status if "buffer_ind" 
              is undefined.  "buffer_ind" is always undefined when
              IB_read_driving_input returns non-NORMAL status. */
           if( ret == NO_DATA )
              AP_set_abort_reason( PGM_REPLAY_DATA_UNAVAILABLE );

           /* Do we need to update the product request information? */
           if( Inp_list[DRIVING].wait_for_data ){

              int num_reqs;

              /* For replay tasks whose driving input is not radial data 
                 and is not warehoused, we need to update the product
                 request information now. */
              PRQ_check_for_replay_requests( &num_reqs );

           }

        }

	/* Check if the input is in sequence or in the case of radial
           input, whether the algorithm control flag is something other 
           than normal; update the current data time and volume sequence
           number, Aborted flag and Resume_time; and update requests;
           Non-NORMAL return from WA_check_data indicates abort 
           condition. */
	input_checked_out = WA_check_data( buffer_ind );
        if( input_checked_out != NORMAL )
           AP_abort_flag( TRUE );

        /* Check if any outputs are requested (i.e., scheduled for 
           generation). */
	output_requested = Output_requested();

        /* Check if this is a monitor only task.  Such tasks do not require
           outputs to be scheduled or have outputs for that matter. */
        output_requested |= Monitor_only;

        /* For replay stream only, report error when request for product is 
           available but the output_requested flag is not set.  This is
           indicative of some error in the request. */
        if( (Input_stream == PGM_REPLAY_STREAM)
                          &&
            (!Inp_list[DRIVING].wait_for_data)
                          &&
            (!output_requested) ){

           LE_send_msg( GL_ERROR, 
                        "Request(s) For Replay Product(s) But No Output Requested?\n" ); 
           LE_send_msg( GL_ERROR, 
                        ">>> Generally Indicates Problem With Request Data <<<\n" ); 

        }

        /* For RADIAL_DATA timing and BASEDATA_ELEV type inputs, check if data
           is needed.  For all other types of inputs, always returns DATA_NEEDED. */
        data_needed = WA_data_filtering( Inp_list + DRIVING,  buffer_ind  ); 
        if( (Input_stream == PGM_REPLAY_STREAM)
                          &&
            (data_needed == DATA_NOT_NEEDED) )
           PS_task_abort( "Replay Request But Data Not Needed?\n" );

        /* Abort condition .... Abort requested outputs if abort 
           flag is set and the algorithm control flag is not 
           PBD_ABORT_NO.  Checking the algorithm control flag 
           prevents multiple task aborts when the task is in the 
           aborted state.   NOTE:  The algorithm control flag is
           only set for processes whose driving input is radial data. */
        if( (AP_abort_flag( UNKNOWN ) == TRUE)
                           && 
            (Alg_control_flg != PBD_ABORT_NO) ){

           /* Get the reason why the task is aborting. */
           int reason = AP_get_abort_reason();

           LE_send_msg( GL_INFO, "Wait For Activation Aborting Outputs With Reason: %d\n",
                        reason );

           /* Abort any outputs scheduled for this process. */
           AP_abort_outputs( reason );

        }

        /* If data is not needed, or output is not requested, or the 
           input is not to be processed because of an abort condition,
           then release the input buffer and read next available driving input. */
	if( (data_needed == DATA_NOT_NEEDED) 
                  ||
	    (!output_requested)
                  ||
	    (input_checked_out != NORMAL) ){

           /* Release the input buffer just allocated and try again. */
           IB_release_input_buffer( DRIVING, buffer_ind ); 
           continue;

        }

	/* The driving input is available and has been read. The data
           is to be processed by this task. 

           Ready to process the data. The driving input LB needs to 
           be seek one step back since it has already been read and 
           needs to be re-read. */
        if( Input_stream == PGM_REALTIME_STREAM 
                         ||
            Inp_list[DRIVING].wait_for_data )
	   ORPGDA_seek (Inp_list[DRIVING].data_id, -1, LB_CURRENT, NULL);

        else if( Input_stream == PGM_REPLAY_STREAM 
                              &&
                 Inp_list[DRIVING].timing == RADIAL_DATA )
           ORPGBDR_set_read_pointer( Inp_list[DRIVING].data_id, LB_CURRENT, -1 ); 

	/* Tells the IB module that a new processing session will 
           start. */
	IB_new_session ();
 
        /* Release the input buffer just allocated.  When the input 
           data is re-read by the task, a new input buffer will be 
           allocated. */
        IB_release_input_buffer( DRIVING, buffer_ind ); 

        /* Set the flag indicating the process is active. */
        Waiting_for_activation = NO;
        LE_send_msg( GL_INFO, "Waiting For Activation == FALSE .. Returning To Application\n" );

	return (0);

    /* End "while" loop. */
    }

    return (0);

/* End of Wait_act() */
}

/********************************************************************

   Description:
      Initializes the WA module.  Determines if this task is
      "Monitor Only".

   Inputs:
      task_entry - TAT entry for this task.

   Outputs:

   Returns:

   Notes:

********************************************************************/
void WA_initialize( Orpgtat_entry_t *task_entry ){

    int output_radial_data = 0;

    /* By default, this task is not "Monitor Only". */
    Monitor_only = 0;

    /* Determine if this task is "Monitor Only".  Such tasks are
       not required to generate output. */
    if( task_entry->type & ORPGTAT_TYPE_MON_ONLY ){

       Monitor_only = 1;
       LE_send_msg( GL_INFO, "This Task Is MONITOR ONLY" );

    }

    Initialize();

    /* Check if use of supplemental scans is allowed. */
    Use_suppl_scans = 0;
    if( task_entry->type & ORPGTAT_TYPE_ALLOW_SUPPL_SCANS ){

       LE_send_msg( GL_INFO, "Use of Supplemental Scans Set By Task Entry\n" );
       Use_suppl_scans = 1;

    }

    output_radial_data = OB_force_use_supplemental_scans();
    if( output_radial_data ){

       LE_send_msg( GL_INFO, "Use of Supplemental Scans Forced By RADIAL_DATA Output\n" );
       Use_suppl_scans = 1;

    }

    if( Use_suppl_scans )
       LE_send_msg( GL_INFO, "Use of Supplemental Scans is Allowed\n" );

/* End of WA_initialize() */
}

/********************************************************************

   Description:
      Returns the value of the Waiting_for_activation flag.

   Inputs:

   Outputs:

   Returns:
      Either YES (not currently active) or NO (currently active).

   Notes:

********************************************************************/
int WA_waiting_for_activation(){

   return( Waiting_for_activation );

/* End of WA_waiting_for_activation. */
}

/********************************************************************

   Description: 
      This function returns the status field of the current radial 
      input.

   Inputs:
      None.

   Outputs:
      None.

   Returns:	
      Status field of the current radial input.  See RDA/RPG 
      ICD for possible values.

   Notes:

********************************************************************/
int WA_radial_status (){

    return (Bd_status);

/* End of WA_radial_status() */
}

/********************************************************************
  
   Description:
      Public interface for WA_supple_scans_allowed().

********************************************************************/
int RPGC_allow_suppl_scans(){

   return( WA_supplemental_scans_allowed() );

/* End of RPGC_allow_suppl_scans(). */
}

/********************************************************************

   Description: 
      This function returns the value of Use_supplemental_scans 

********************************************************************/
int WA_supplemental_scans_allowed (){

    return (Use_suppl_scans);

/* End of WA_supplemental_scans_allowed() */
}

/********************************************************************

   Description: 
      This function returns the argument used in calling RPG_wait_act.

   Inputs:
      None.

   Outputs:
      None.

   Return:	
      TRUE if argument is WAIT_DRIVING_INPUT, FALSE otherwise.

   Notes:

********************************************************************/
int WA_wait_driving (){

    if (Wait_for == WAIT_DRIVING_INPUT)
	return (TRUE);
    else
	return (FALSE);

/* End of WA_wait_all() */
}

/********************************************************************

    Description: 
        This function processes abort information and detects any 
        out-of-sequence data in the driving input stream. It sets the 
        current data time. It also sets/resets the aborted state and 
        reads in user requests. It also sends info to other modules,
        PS, OB, ADP, ADE, and SS. This function processes the radial 
	driving input and calls Check_data_prod to perform the 
	job for other driving inputs.

	This function is called whenever a driving input 
	data is read.

    Inputs:	
        buffer_ind - Index of the input data buffer.

    Outputs:

    Returns:	
        returns NORMAL if data is ok; otherwise a code value 
        if an abort flag is found in the data or the input data 
        is detected to be out of sequence and the processing needs 
        to be aborted.  

    Notes:
	When the data is found to be out-of-sequence, we
	must abort current processing. This is accomplished 
	by returning TERMINATE. This radial, however, may be 
	the first radial of a new section of input data. Thus 
	it has to be processed after abort. Considering this, 
	we back up the read pointer one step in the driving 
	input data stream. We must set aborted state here in 
	order to prevent RPG_wait_act from returning 
	when this function is called from RPG_wait_act.

	When this function is called with the same data, we 
	must skip the in-sequence check if the data is 
	in-sequence. We process this situation by means of
	variable processed_time. Note that this function can
	not be called with any data that is older, in the input 
	stream, than the data proviously processed (i.e. The 
	sequential input LB can not be seek back more than 
	once without a read).

********************************************************************/
int WA_check_data ( int buffer_ind ){

    static int Init_prod_list = FALSE;

    Base_data_header *bhd;
    time_t tm;				/* data time */
    int ret, clear_abort_flag, bad_input;

    static int first_time_initialization = 0;

    /* Non radial-based inputs are checked by Check_data_prod. */
    if (Inp_list [DRIVING].timing != RADIAL_DATA)
	 return (Check_data_prod ( buffer_ind ));

    /* Do first time initialization for replay tasks consuming radial inputs. */
    if( !first_time_initialization ){

       /* For replay task consuming radial inputs, we do not care if the 
          previous volume scan completed successfully or not.  We should
          not have to abort the previous volume scan in this case since
          if there had been a request in that volume scan, the data
          would not have been available and the request would not have 
          been satisfied.  */ 
       if( Input_stream == PGM_REPLAY_STREAM ){

          LE_send_msg( GL_INFO, "Setting Test.end_vol == TRUE for Replay Task\n" );
          Test.end_vol = TRUE;

       }

       first_time_initialization = 1;

    }

    /* If buffer index invalid, return NO_DATA. */
    if( buffer_ind < 0 )
       return (NO_DATA);

    /* Cast input buffer pointer to basedata header structure 
       pointer. */
    bhd = (Base_data_header *) Inp_list [DRIVING].buf[buffer_ind];

    /* Initialize return value of this function to NORMAL and
       Alg_control_flg to PBD_ABORT_NO. */
    ret = NORMAL;	
    Alg_control_flg = PBD_ABORT_NO;

    /* Derive the UNIX time of this radial input.  

      NOTE:  Must substract 1 from Julian date since the reference
             date used by RDA is different than UNIX reference 
             date. */
    tm = (bhd->date - 1) * 86400 + bhd->time / 1000;
    Inp_list[DRIVING].time = tm;

    /* Get the volume scan sequence number of this radial input. 

       NOTE:  The volume sequence number is derived from two values:
              the volume scan number which has values between 1-MAX_SCAN_SUM_VOLS, 
              and the volume scan number quotient, which is the 
              quotient when dividing the volume scan sequence number
              by MAX_SCAN_SUM_VOLS. */
    Vol_seq_num_wait_all = ORPGMISC_vol_seq_num( (int) bhd->vol_num_quotient, 
                                                 (int) bhd->volume_scan_num ) ;

    /* Set the volume sequence number in the VI and PRQ modules. */
    VI_set_vol_seq_num( Vol_seq_num_wait_all );
    PRQ_set_vol_seq_num( Vol_seq_num_wait_all );

    /* If this is the same radial processed as previous pass, update
       the support environment on new elevation start then return NORMAL. 
       On new volume, initialize the abort product list.

       NOTE:  The same radial is typically re-read and re-processed 
              after the initial read either because a sequencing problem
              was detected, the algorithm control flag was not normal,
              or the process is waiting for activation. */
    if( (Test.processed_azi_num == bhd->azi_num) 
                   &&
        (Test.processed_time == tm) ){

       /* Azimuth number and time match.  Now check elevation cut number and
          volume sequence number. */
       if( (Test.rda_cut_num == bhd->elev_num)
                       && 
           (Test.vol_test_num == Vol_seq_num_wait_all) ){

          /* If the waiting for activation flag is set and we have re-read the 
             same radial as last pass, then we want to return TERMINATE.  This
             might happen if the application never read the radial. Returning
             TERMINATE prevents an infinite loop. */
           if( Waiting_for_activation ){

              LE_send_msg( GL_INFO, 
                 "Waiting For Activation And Input Re-read.  Return TERMINATE\n" );
              return( TERMINATE );

           }

           /* On new elevation, update the support environment. */
           if( Test.new_elev )
              Update_support_environment( bhd, tm, Test.new_vol, 
                                          (Prod_header *) NULL );

           /* If flag is true, reinitialize the list of products generated for 
              this volume scan. */
           if( Init_prod_list ){

              Init_prod_list = FALSE;
              AP_init_prod_list( Vol_seq_num_wait_all );

           }

 	   return (ret);

       }

    }

    /* Set new elevation and new volume flags if radial status 
       indicates the start of a new scan (elevation or volume) and
       the test variables indicate the elevation/volume has changed.  
       For radial status, mask upper nibble of byte.  Upper 
       nibble value of 8 indicates BAD. Currently BAD is 
       undefined so all is GOOD. */
    Bd_status = bhd->status & 0xf;
    Test.new_elev = Test.new_vol = Test.new_scan = FALSE;
    if( (Bd_status == GOODBEL) || (Bd_status == GOODBVOL) ){

       /* If Replay data stream, starting new elevation. */
       if( Input_stream != PGM_REALTIME_STREAM ){

	   Test.new_elev = TRUE;
           Test.azi_num = UNDEFINED;

       }
       else{

          /* This is for the REALTIME data stream ...... */

          /* This is for normal start of elevation.  The
             first case would happen at start of volume,
             the second will always occur at start of 
             elevation (and volume). */
          if( (Test.rda_cut_num <= 0)
                         ||
              (Test.rda_cut_num != bhd->elev_num) ){

	     Test.new_elev = TRUE;
             Test.azi_num = UNDEFINED;

          }
          else if( Test.rda_cut_num == bhd->elev_num ){

             /* This is for restart of elevation.  This is 
                necessary so that the azimuth number check 
                does not fail causing tasks to abort. */
             Test.azi_num = UNDEFINED;

          }

       }

        /* For split cuts, both the reflectivity-only and Doppler-only
           cuts will have the same radial status at start of elevation
           scan (i.e, both will be GOODBVOL).  True beginning of volume
           is start of reflectivity-only elevation cut (elevation 
           number 1).   For replay tasks, beginning of volume occurs
           when the volume scan number changes. */
        if( (Input_stream != PGM_REALTIME_STREAM) 
                            && 
            ((Test.vol_test_num != Vol_seq_num_wait_all)
                            ||
             (Bd_status == GOODBVOL && bhd->elev_num == 1)) )
 	   Test.new_vol = TRUE;		

        else {

           /* Input stream must be PGM_REALTIME_STREAM. */
           if( (Test.vol_test_num <= 0)
                         ||
               (Test.vol_test_num != Vol_seq_num_wait_all) )
              Test.new_vol = TRUE;		

        }

        /* If new volume, must also be new elevation.  Clear the 
           product list (for aborting). */
        if( Test.new_vol == TRUE ){

           Test.new_elev = TRUE;
           Init_prod_list = TRUE;

           Test.vcp = bhd->vcp_num;
           Test.n_exp_cuts = RPGCS_get_num_elevations( Test.vcp );

        }

        Test.new_scan = Test.new_elev | Test.new_vol;

    }

    /* Check if the abort flag is set. */
    if( AP_abort_flag( UNKNOWN )){

        /* Abort flag is set.  Determine whether the abort 
           flag can be cleared.  Clearing is based on the resume
           time and whether new data, new elevation, or new volume
           has been detected. */
        clear_abort_flag = 0; 

        switch( Resume_time ){

           case NEW_DATA:
           default:
           {
              clear_abort_flag = 1;

              break;
           }

           case NEW_ELEVATION:
           {
	      if( Test.new_elev ) 
                 clear_abort_flag = 1;

              break;
           }

           case NEW_VOLUME:
           {
	      if( Test.new_vol )
                 clear_abort_flag = 1;

              break;
           }

        }

        /* Clearing abort flag owing to detection of NEW_DATA, NEW_VOLUME,
           or NEW_ELEVATION. */
	if( clear_abort_flag ){

            /* Inform the user. */
            if( Resume_time == NEW_DATA )
               LE_send_msg( GL_INFO, "Processing Resumed For NEW_DATA\n" );
	    else if( Test.new_elev && Resume_time == NEW_ELEVATION) 
               LE_send_msg( GL_INFO, "Processing Resumed For NEW_ELEVATION (new_vol = %d)\n",
                            Test.new_vol );
	    else if( Test.new_vol && Resume_time == NEW_VOLUME )
               LE_send_msg( GL_INFO, "Processing Resumed For NEW_VOLUME\n" );

            /* Clear abort flag. */
	    AP_abort_flag( FALSE );
            Aborted_vol_seq_num = 0;
	    Resume_time = RESUME_UNDEFINED;

	}
        else{

           /* If we are already in an aborted state and the algorithm control
              flag is not normal, then we want to abort. */
           ret = Check_alg_control( bhd );

           /* Set the return value to TERMINATE since the abort flag is
              set and we have not encountered our resume time yet.
              Based on the Resume_time, the end_vol and end_elev flags need
              to be set correctly in the event that the first radial processed
              after resuming has the algorithm control flag set.  This
              allows the radialing sequencing checks to fail. */

           /* For resume time of NEW_ELEVATION and radial status indicates 
              good end of elevation, set the end of elevation flag.  This
              prevents the sequencing checks from complaining. */
           if( (Bd_status == GENDEL) && (Resume_time == NEW_ELEVATION) )
              Init_test_vars( &Test, Vol_seq_num_wait_all, (int) FALSE, (int) TRUE );

           /* If radial status indicates good end of volume, set the end of
              elevation and end of volume flags.  This prevents the sequencing
              checks from complaining. */
           else if( Bd_status == GENDVOL )
              Init_test_vars( &Test, 0, (int) TRUE, (int) TRUE );
          
           else if( Resume_time == NEW_DATA )
	      Init_test_vars( &Test, 0, (int) TRUE, (int) TRUE );

           else if( (Resume_time == NEW_ELEVATION) 
                                 ||
                    (Resume_time == NEW_VOLUME ) )
	       Init_test_vars( &Test, Vol_seq_num_wait_all, (int) FALSE,
                               (int) FALSE );

           return (TERMINATE);

        }

    }

    /* Initialize the bad_input flag to no errors (problems). */
    bad_input = FALSE;

    /* Check algorithm control flag only if this radial is not being 
       reprocessed. */
    if( Test.processed_time != UNDEFINED )
       ret = Check_alg_control( bhd );

    /* At start of new volume, check if unexpected start.  At start of
       new elevation, check elevation scan number sequencing.  We do
       not check for unexpected start of elevation since we allow 
       for elevation restarts. */
    bad_input = Radial_seq_check_pass_1( &Test, bhd );

    /* If not bad input, do more checking. */
    if( !bad_input ){

       /* Update supporting environment on new elevation and "good input". */
       if( Test.new_elev )
          Update_support_environment( bhd, tm, Test.new_vol, (Prod_header *) NULL );

       /* The following code is used to complete sequencing checks on the 
          radial.  */ 
       bad_input = Radial_seq_check_pass_2( &Test, bhd );

    }

    /* Input data error - Reset variables and enter aborted state. */
    if( bad_input ){

        /* Initialize all test variables. */
	Init_test_vars( &Test, 0, (int) TRUE, (int) TRUE );

        /* Set task resume time if not already set. */
	WA_set_resume_time ();

	/* Make sure this driving input data will be re-processed. */
	ORPGDA_seek (Inp_list[DRIVING].data_id, -1, LB_CURRENT, NULL);

        /* Set return code to TERMINATE so this task knows to abort 
           itself. */
	ret = TERMINATE;

        /* Set abort reason code. */
        if( Input_stream == PGM_REALTIME_STREAM )
           AP_set_abort_reason( PGM_INPUT_DATA_ERROR );

        else if( Input_stream == PGM_REPLAY_STREAM )
           AP_set_abort_reason( PGM_REPLAY_DATA_UNAVAILABLE );

        else
           PS_task_abort( "Input Stream Is Undefined (%d)\n", Input_stream );

        /* If waiting for activation and the algorithm control flag is not
           already set, check the algorithm abort flag to see if the
           algorithm control flag is set.   We want to do this to force 
           abort. */
        if( Waiting_for_activation && (Alg_control_flg == PBD_ABORT_NO) ){

           Check_alg_control( bhd );

           /* One of the Radial Sequence Check functions returned Bad Input.  
              Set the Alg_control_flg to force abort. */
           if( Alg_control_flg == PBD_ABORT_NO )
              Alg_control_flg = AP_alg_control( PBD_ABORT_INPUT_DATA_ERROR );

        }

        /* Set the aborted volume sequence number if not already set. */
        if( Aborted_vol_seq_num == 0 ){

           Aborted_vol_seq_num = Vol_seq_num_wait_all;
           AP_set_aborted_volume( Aborted_vol_seq_num );
        
        }

    }
    else{
  
       /* Update input processed time and processed azimuth number. */
       Test.processed_time = tm;
       Test.processed_azi_num = bhd->azi_num;
     
    } 

    /* Reset test variables on end of elevation or volume. */
    if( Bd_status == GENDEL || Bd_status == GENDVOL ){

       Test.azi_num = UNDEFINED;
       Test.end_elev = TRUE;

       /* At end of volume scan ... */
       if( Bd_status == GENDVOL ){

          /* Set the end of volume flag and initialize the
             volume scan test number. */
	  Test.end_vol = TRUE;
          Test.vol_test_num = 0;
          
       }

       /* For non-realtime tasks, initialize the following test 
          variables if task is elevation-based.  This is necessary
          so that subsequent sequencing checks which should pass, do
          pass. */
       if( Input_stream != PGM_REALTIME_STREAM ){

          /* For elevation-based tasks ... */
          if( Task_type == TASK_ELEVATION_BASED ){

             Test.rda_cut_num = UNDEFINED;
             Test.rpg_cut_num = UNDEFINED;
             Test.processed_time = UNDEFINED;
             Test.end_vol = TRUE;
             Test.vol_test_num = 0;
          
          }    

       }

    }

    /* Update product requests and update ITCs. */
    if( Test.new_scan ){

	PRQ_update_requests ( Test.new_vol, bhd->rpg_elev_ind, tm );
	ITC_update ( Test.new_vol );

    }

    if( (!Waiting_for_activation) && (ret != NORMAL) )
       LE_send_msg( GL_INFO,
                    "WA_check_data Ret Bad Status To App (%d)\n", ret );
    else if( Waiting_for_activation && (ret != NORMAL) )
       LE_send_msg( GL_INFO,
                    "WA_check_data Ret Bad Status To Wait_act (%d)\n", ret );

    return (ret);

/* End of WA_check_data() */
}

/********************************************************************

   Description: 
      This function computes the resumption time based on the task 
      type and the input data stream.

   Inputs:

   Outputs:

   Returns:
      The task resumption time.

   Notes:

********************************************************************/
int WA_set_resume_time (){

    /* If Resume_time already defined, just return its current value.
       The Resume_time may have already been defined based on some
       abort condition. */
    if( Resume_time != RESUME_UNDEFINED )
       return( Resume_time );
    
    /* For non-realtime tasks, resume time is NEW_DATA. */
    if( Input_stream != PGM_REALTIME_STREAM )
       Resume_time = NEW_DATA;

    else{
 
       /* Set Resume_time based on Task_type.  Return Resume_time. */
       if (Task_type == TASK_VOLUME_BASED)
          Resume_time = NEW_VOLUME;

       else if (Task_type == TASK_ELEVATION_BASED)
          Resume_time = NEW_ELEVATION;

       else
          Resume_time = NEW_DATA;

    }

    return (Resume_time);

/* End of WA_set_resume_time() */
}

/********************************************************************

   Description: 
      This function performs additional data filtering.  This function, 
      so far, is needed only for basedata and basedata_elev, in which
      case certain elevations are not needed according to the input type.
 
   Inputs:     
      idata - The input data structure
      buffer_ind - The index of the input data buffer

   Outputs:

   Returns: 
      The function returns DATA_NEEDED if the data need to be returned 
      by get_inbuf. Otherwise it returns DATA_NOT_NEEDED.

   Notes:

********************************************************************/
int WA_data_filtering (In_data_type *idata, int buffer_ind){

   Base_data_header *bhd = NULL;
   Compact_basedata_elev *elev_prod = NULL;
   Compact_radial *radial = NULL;
   int alias_id = 0;
   unsigned short cut_type = 0;
   unsigned short has_suppl_scan = 0, is_suppl_scan = 0;

   /* If not valid buffer_ind, return DATA_NOT_NEEDED. */
   if( buffer_ind < 0 )
      return (DATA_NOT_NEEDED);

   /* Get the alias ID in the event that the input type is an alias. */ 
   alias_id = ORPGPAT_get_aliased_prod_id( idata->type );

   /* If input data timing is RADIAL_DATA, compare the msg_type field of 
      the radial with the type mask field provided by the PAT.  */   
   if( idata->timing == RADIAL_DATA ){

      bhd = (Base_data_header *)(idata->buf[buffer_ind]);

      /* For filtering purposes, get the cut type. */
      cut_type = (bhd->msg_type & CUT_TYPE_MASK);
    
      /* Check is this is supplemental cut and if this task
         is allowed to process it. */
      has_suppl_scan = bhd->msg_type & SUPPLEMENTAL_CUT_TYPE;
      is_suppl_scan = bhd->suppl_flags & RDACNT_SUPPL_SCAN;

      /* Radial producers always process supplemental cuts. */
      if( Task_type != TASK_RADIAL_BASED ){

         /* If supplemental scan and task is not authorized to process
            supplemental scans, return data not needed. */
         if( (has_suppl_scan) 
                    && 
             (is_suppl_scan)
                    &&
             (!Use_suppl_scans) )
            return (DATA_NOT_NEEDED);

      }

      if ( cut_type & idata->type_mask )
         return (DATA_NEEDED);

      return (DATA_NOT_NEEDED);

   }

   /* If input data class ID is BASEDATA_ELEV, compare the type field of the 
      product header with the type mask field provided by the PAT. */
   else if( (idata->data_id == BASEDATA_ELEV) 
                            ||
            (ORPGPAT_get_class_id( alias_id ) == BASEDATA_ELEV)  
                            ||
            (idata->data_id == SR_BASEDATA_ELEV) 
                            ||
            (ORPGPAT_get_class_id( alias_id ) == SR_BASEDATA_ELEV) ){ 

      Prod_header *phd = (Prod_header *) idata->buf[buffer_ind];

      /* For replay-stream, we currently do not support this data type. */
      if( Input_stream == PGM_REPLAY_STREAM )
         return(DATA_NOT_NEEDED);

      /* This is a special case.  Check if this is an abort message.  
         If so, then we assume the data is needed because the consumer of
         the data must read the data in order to ABORT. 

         This mechanism serves the same purpose as the Alg_control_flag
         for radial inputs. */
      if( phd->g.len < 0 ){

         LE_send_msg( GL_INFO, "Setting DATA_NEEDED because this is an ABORT message\n" );
         return(DATA_NEEDED);

      }

      /* For the elevation product, check the type field against the 
         type mask define in the PAT. */
      elev_prod = (Compact_basedata_elev *) 
                          (idata->buf[buffer_ind] + sizeof(Prod_header));
      radial = (Compact_radial *) &elev_prod->radial[0];
      
      cut_type = elev_prod->type & CUT_TYPE_MASK;

      /* Check is this is supplemental cut and if this task
         is allowed to process it. */
      has_suppl_scan = elev_prod->type & SUPPLEMENTAL_CUT_TYPE;
      is_suppl_scan = radial->bdh.suppl_flags & RDACNT_SUPPL_SCAN;

      if( (has_suppl_scan)
               &&
          (is_suppl_scan)
               &&
          (!Use_suppl_scans) )
         return (DATA_NOT_NEEDED);

      if( cut_type & idata->type_mask )
         return(DATA_NEEDED);

      return(DATA_NOT_NEEDED);

   }

   return (DATA_NEEDED);

/* End of WA_data_filtering() */
}

/********************************************************************
   Description:
      Returns the index of the next available input for processes
      which request ANY_DATA.

   Inputs:

   Outputs:

   Returns:
      See Description.

   Notes:

********************************************************************/
int WA_get_next_avail_input(){

   return( Wait_any_input_index );

}

/***********************************************************************

   Description:
      Returns pointer to replay request query information.

   Inputs:

   Outputs:

   Returns:
      Pointer to replay request query information.

   Notes:

***********************************************************************/
Replay_req_info_t* WA_get_query_info(){

   return( &Replay_info );

/* End of WA_get_query_info() */
}

/********************************************************************

   Description: 
      This function is used to control the processing of RPG 
      algorithm/product task which are activated based the availability 
      of any of the task inputs.  The task inputs must be non-radial 
      based.

      The input data LB's are also examined periodically.  If the 
      status of an LB has changed, then return with INPUT_AVAILABLE.

   Inputs:      
      wait_for - Must be WAIT_ANY_INPUT.
      input_avail   - Data availability. Return value of this module.
      wait_for_data - Determines if we should wait or not wait for 
                      data availability.  Currently, if process is 
                      event-based, we do not wait but only check.

   Outputs:

   Return: 
      There is no return value defined.

   Notes:
      It is assumed that all inputs from the previous volume scan
      are available before inputs from the current volume scan.  

      Currently, waiting for any data only supports volume-based
      tasks.

********************************************************************/
int WA_wait_for_any_data( int wait_for, int *input_avail,
                          int wait_for_data ){

   int input_updated, ret, i, new_vol = FALSE, new_elev = FALSE;
   Prod_header prod_header;

   static int initialized = 0;
   
   /* First time initialization. */
   if( !initialized ){ 

      Wait_for_any_init( wait_for );

      /* Done with one-time initialization. */
      initialized = 1;

   }

   /* Initialize input available to NO_DATA. */
   *input_avail = NO_DATA;

   /* Return inputs from previous volume scans, if any,
      before we process the "Vol_seq_num_wait_any" volume scan. */
   for( i = 0; i < N_inps; i++ ){
         
      /* Check if this input needs to be read. */
      if( (Inp_list[i].must_read > 0) 
                     &&
          (Inp_list[i].vol_num < Vol_seq_num_wait_any) ){

         *input_avail = INPUT_AVAILABLE;
         Wait_any_input_index = i;
         return (0);

      }

   }

   /* Wait for input data to become available. */
   while(1){

      /* Check if the end of volume event has occurred. */
      INIT_process_eov_event();

      /* Check if previous volume has ended.   If so, were the
         number of elevation cuts in the VCP the same as what was
         expected based on the VCP definition (this would happen, 
         for example, if AVSET were running).  If so, then let's 
         abort volume scan for elevation-based products to clear
         out any product requests for unscanned cuts that will 
         never be satisfied. 

         Note:  Currently the only know reason a VCP would end 
         prematurely and not be unexpected is because of AVSET.  
         Normally the AVSET flag would be checked.   Unfortunately 
         this is not totally reliable when playing back data. */
      if( Input_stream == PGM_REALTIME_STREAM )
          Check_avset();

      /* Check if any of the input types are available for the current
         or later volume scan.  If true, return to calling module. */
      input_updated = FALSE;
      for( i = 0; i < N_inps; i++ ){
         
         /* Check if this input needs to be read. */
         if( Inp_list[i].must_read > 0 ){

            *input_avail = INPUT_AVAILABLE;
            Wait_any_input_index = i;
            return (0);

         }

         /* Check status to see if this LB has been updated. */
         if( (ret = ORPGDA_stat( Inp_list[i].data_id, &Status ) ) != LB_SUCCESS )
            PS_task_abort( "ORPGDA_stat Failed (%d) (data_id: %d)\n", 
                           ret, Inp_list[i].data_id );

         /* LB has been updated. */
         if( Status.updated == LB_TRUE ){

            LB_id_t num_unread_msgs = 0;
            
            /* If the next message read has input stream different than this 
               process, ignore it.  Read the next message.  Only process data
               from same input stream. */
            while(1){

               ret = ORPGDA_read( Inp_list[i].data_id, (char *) &prod_header, 
                                  sizeof(Prod_header), LB_NEXT );

               if( ((ret > 0) || (ret == LB_BUF_TOO_SMALL))
                                && 
                   (prod_header.g.input_stream != Input_stream) ){

                  continue; 
        
               }

               /* If the message has expired, seek to the first unread message 
                  in the LB. */
               else if( ret == LB_EXPIRED ){

                  ret = ORPGDA_seek( Inp_list[i].data_id, 0, LB_FIRST, NULL );
 
                  if( ret == LB_SUCCESS )
                     continue;

               }

               break;

            /* End of "while" loop */
            }

            /* Check for valid return value from ORPGDA_read. */
            if( ret > 0 || ret == LB_BUF_TOO_SMALL ){

               LB_id_t current_msg_id;

               /* This should NEVER HAPPEN!!!! So let me know if it does. */
               if( prod_header.g.input_stream != Input_stream )
                  LE_send_msg( GL_ERROR," Data ID %d Input Stream Not Of Correct Type\n",
                               prod_header.g.input_stream );

               /* Get the message ID of the message just read. */
               current_msg_id = ORPGDA_get_msg_id();

               /* Determine the message ID of the latest message in the LB.
                  For this, we can determine how many messages in the LB are
                  unread.  The "must_read" structure element must then be 
                  incremented by the number of unread messages. */
               ret = ORPGDA_msg_info( Inp_list[i].data_id, LB_LATEST, &Info );
               if( ret == LB_SUCCESS )
                  num_unread_msgs = Info.id - current_msg_id + 1;

               else
                  num_unread_msgs = 1;

               /* Update support environment, requests, and product list on new
                  volume scan. */
               if( (int) prod_header.g.vol_num >= Vol_seq_num_wait_any ){

                  Vol_seq_num_wait_any = (int) prod_header.g.vol_num;

                  /* Set the volume sequence number in the VI and PRQ modules. */
                  VI_set_vol_seq_num( Vol_seq_num_wait_any );
                  PRQ_set_vol_seq_num( Vol_seq_num_wait_any );

                  /* Volume sequence number change ... A new volume scan started. */
                  if( Old_vol_seq_num_wait_any != Vol_seq_num_wait_any ){

                     /* Update support environment and save volume sequence number. */
                     Update_support_environment( (Base_data_header *) NULL,
                                                 0, 1, (Prod_header *) &prod_header ); 
                     Old_vol_seq_num_wait_any = Vol_seq_num_wait_any;

                     /* Update product requests. */
                     PRQ_update_requests( 1, (int) prod_header.g.elev_ind, 
                                          prod_header.g.vol_t );

                     /* Initialize Product List. */
                     AP_init_prod_list( Vol_seq_num_wait_any );

                     /* Set "new_vol" flag. */
                     new_vol = TRUE;

                     /* For elevation-data input, a new volume scan implies a new
                        elevation scan. */
                     if( Inp_list[i].timing == ELEVATION_DATA ){

                        /* Save the elevation index. */
                        Old_elev_ind_wait_any = (int) prod_header.g.elev_ind;
   
                        /* set the "new_elev" flag. */
                        new_elev = TRUE;

                     }

                  }
                  else{

                     /* Update support environment on new input */
                     Update_support_environment( (Base_data_header *) NULL,
                                                 0, 0, (Prod_header *) &prod_header ); 

                     /* Check if the elevation has changed? */
                     if( Inp_list[i].timing == ELEVATION_DATA ){

                        if( (int) prod_header.g.elev_ind != Old_elev_ind_wait_any ){

                           /* Update product requests. */
                           PRQ_update_requests( 0, (int) prod_header.g.elev_ind, 
                                                prod_header.elev_t );

                           Old_elev_ind_wait_any = (int) prod_header.g.elev_ind;
                           new_elev = TRUE;

                        }
                        else
                           new_elev = FALSE;

                     }

                  }

               }
               else{

                  LE_send_msg( GL_INFO, 
                               "Input %d For Volume %d Arrived After Input For Volume %d\n",
                               prod_header.g.prod_id, prod_header.g.vol_num, 
                               Vol_seq_num_wait_any );
           
                  /* Clear new volume flag. */
                  new_vol = FALSE;

               }

               /* Check if abort flag is set. */
               if (AP_abort_flag( UNKNOWN ) ){

                  /* Abort flag is set */
	          if ( (new_vol && (Resume_time == NEW_VOLUME))
                                 ||
                       (Resume_time == NEW_DATA) 
                                 ||
                       (new_elev && (Resume_time == NEW_ELEVATION)) ){

                     /* Inform the user. */
	             if( new_vol && (Resume_time == NEW_VOLUME) )
                        LE_send_msg( GL_INFO, "Clearing Abort flag for NEW_VOLUME\n" );
	             else if( Resume_time == NEW_DATA )
                        LE_send_msg( GL_INFO, "Clearing Abort flag for NEW_DATA\n" );
                     else if( new_elev && (Resume_time == NEW_ELEVATION) )
                        LE_send_msg( GL_INFO, "Clearing Abort flag for NEW_ELEVATION\n" );

                     /* Clear abort flag. */
	             AP_abort_flag( FALSE );

	           }

                }

                /* Reposition file pointer so this input can be re-read. */
                ORPGDA_seek( Inp_list[i].data_id, -1, LB_CURRENT, NULL );

                /* Set flag indicating input availability and increment the 
                   "must_read" structure element by the number of unread messages. */
                input_updated = TRUE;
                Inp_list[i].must_read += num_unread_msgs;
                Wait_any_input_index = -1;

                /* Fill in Inp_list fields for this input. */
                Inp_list[i].elev_time = prod_header.elev_t;
                Inp_list[i].rpg_elev_index = prod_header.g.elev_ind;
                Inp_list[i].vol_time = prod_header.g.vol_t;
                if( Inp_list[i].timing == ELEVATION_DATA )
                   Inp_list[i].time = Inp_list[i].elev_time;
                else
                   Inp_list[i].time = Inp_list[i].vol_time;
                Inp_list[i].vol_num = prod_header.g.vol_num;


             }
             else if( ret != LB_TO_COME )
                PS_task_abort( "ORPGDA_read Failed (%d) (data_id: %d, LB_NEXT)\n",
                               ret, Inp_list[i].data_id );

         }

      /* End of "for" loop. */
      }

      /* Process any inputs which arrived.  Break out of "while" loop. */
      if( input_updated )
         break;

      /* No inputs yet available.  Sleep if must wait for data, then check 
         LB status again.  Else, return NO_DATA. */
      if( wait_for_data == EVT_WAIT_FOR_DATA )
         EN_control (EN_WAIT, 1000);

      else{

         *input_avail = NO_DATA;
         return (0);

      }
      
   /* End of "while" loop. */
   }  

   /* Normal return. */
   *input_avail = INPUT_AVAILABLE;
   return (0);

/* End of WA_wait_for_any_data() */
}

/********************************************************************

   Description:
      This function sets information that may be needed to 
      determine if a VCP terminated early (e.g., AVSET is active) 
      and elevation products need to be aborted for those elevations
      that will never be scanned.

   Inputs:
      outbuf - Pointer to output buffer. 

   Returns:
      Returns 0 on success, -1 on failure.

********************************************************************/
int WA_set_output_info( char *outbuf, int olind ){

   Prod_header *phd = (Prod_header *) outbuf;
   int num_rda_elevs = 0;
   
   static int last_vcp = UNDEFINED;

   /* Shouldn't happen but just in case. */
   if( phd == NULL )
      return -1;

   /* Test for End of Volume. */
   if( phd->bd_status == GENDVOL )
      Vtest.end_vol[olind] = 1;
 
   else
      Vtest.end_vol[olind] = 0;

   /* Gather VCP information.   Need to know the last elevation
      expectedi (based on VCP definition) in the VCP. */
   if( (Vtest.vcp == UNDEFINED)
                  ||
       (Vtest.vcp != last_vcp) ){

      Vtest.vcp = phd->vcp_num;
      num_rda_elevs = RPGCS_get_num_elevations( Vtest.vcp );

      /* We need to subtract 1 in function call since argument assumes 0 indexed. */
      Vtest.n_exp_cuts = RPGCS_get_rpg_elevation_num( Vtest.vcp, num_rda_elevs-1 );

      /* In the unlikely event of an error. */
      if( Vtest.n_exp_cuts <= 0 ){

         Vtest.vcp = UNDEFINED;
         Vtest.n_exp_cuts = UNDEFINED;

      }

      /* Save VCP in order to detect changes in VCP. */
      last_vcp = Vtest.vcp;

   }

   /* Save the elevation of the last product generated. */
   Vtest.n_cuts[olind] = phd->g.elev_ind;

   /* Return Success. */
   return 0;

/* End of WA_set_output_info(). */
}

/********************************************************************

   Description: 
      This function detects any out-of-sequence data in a non-radial 
      based driving input stream. It also sets/resets the aborted 
      state and reads in user requests.

      This function is called whenever a non-radial based driving 
      input data is read.

   Input:	 
      buffer_ind - Index of the input data buffer.

   Output:

   Return:	 
      returns NORMAL if data is ok, or TERMINATE if the data in 
      the driving input is detected to be out of sequence and 
      the processing needs to be aborted.

   Notes:	 
      Refer to notes for WA_check_data.

      For volume and time based inputs we do not apply any 
      sequencing check for the moment.

********************************************************************/
static int Check_data_prod ( int buffer_ind ){

    static int end_vol = TRUE;		/* end of volume detected */
    static int elev_cnt = -1;		/* elevation product cnt in a volume */
    static unsigned int vol_test_num = 0;	
                                        /* current volume number for testing */
    static time_t processed_time = UNDEFINED;	
                                        /* time of the data previously processed */
    static int processed_elev_ind = 0;  /* Elevation index for data previously processed. */
    static unsigned int processed_vol = 0;	
                                        /* volume scan sequence number of the data previously
                                           processed */
    static int new_vol = FALSE;

    Prod_header *phd;
    int ret, bad_input, data_needed, escape;
    time_t tm;

    /* Check the buffer_ind.  If not valid, return. */
    if( buffer_ind < 0 )
       return (NO_DATA);

    phd = (Prod_header *) Inp_list[DRIVING].buf[buffer_ind];

    /* Initialize return value of this function. */
    ret = NORMAL;

    /* Get the time of this data ... either start of elevation time 
       or start of volume time. */
    if( Driving_input_type == ELEVATION_DATA ) 
	tm = phd->elev_t;

    else
	tm = phd->g.vol_t;

    /* Time is not a good indicator for checking whether data has been
       already processed by this function, especially if ELEVATION_DATA. */
    escape = TRUE;
    if( tm != processed_time )
       escape = FALSE;

    if( Input_stream == PGM_REALTIME_STREAM ){

       if( (Driving_input_type == ELEVATION_DATA)
                            &&
           (processed_elev_ind != phd->g.elev_ind) )
          escape = FALSE;

    }
 
    /* Get the volume scan sequence number of this input. */
    Vol_seq_num_wait_all = phd->g.vol_num;  

    /* Set the volume sequence number in the VI and PRQ modules. */
    VI_set_vol_seq_num( Vol_seq_num_wait_all );
    PRQ_set_vol_seq_num( Vol_seq_num_wait_all );

    /* If processed time == tm and processed_vol = Vol_seq_num_wait_all, this 
       data has already been processed by this function. */
    if ( escape ){

       if( processed_vol == Vol_seq_num_wait_all ){

          /* If the waiting for activation flag is set and we have re-read the 
             same driving input as last pass, then return TERMINATE.  This
             might happen if the application never read the input. Returning
             TERMINATE prevents an infinite loop. */
           if( (Waiting_for_activation)
                        &&
               (Input_stream == PGM_REALTIME_STREAM) ){

              LE_send_msg( GL_INFO, 
                 "Waiting For Activation And Input Re-read.  Return TERMINATE\n" );
              return( TERMINATE );

           }

           if( new_vol )
              AP_init_prod_list( Vol_seq_num_wait_all );

           if( Input_stream == PGM_REALTIME_STREAM )
	      return (ret);

       }
       else
          processed_vol = Vol_seq_num_wait_all;

    }
    else{


	processed_time = tm;
        processed_vol = Vol_seq_num_wait_all;
        processed_elev_ind = phd->g.elev_ind;

    }

    Inp_list[DRIVING].time = tm;
    Inp_list[DRIVING].elev_time = phd->elev_t;
    Inp_list[DRIVING].rpg_elev_index = phd->g.elev_ind;
    Inp_list[DRIVING].vol_time = phd->g.vol_t;
    Inp_list[DRIVING].vol_num = Vol_seq_num_wait_all;

    /* Input data indicate new volume scan if any of the following 
       conditions are met:

       (1) the driving input for this task is elevation data
           AND ((the RPG Elevation Index is 1) 
                             OR 
           (this is the first product generated in the current 
           volume scan (this last condition supports consumers 
           of elevation data for which the first product may 
           not correspond to the lowest elevation scan).  

           For driving input of REFLDATA_ELEV or COMBBASE_ELEV, 
           check if the data is also needed. 
       (2) the current elevation time corresponds to the current 
           volume time
       (3) the driving input for this task is volume data */
    if( (Driving_input_type == ELEVATION_DATA)
                            &&
                  ((phd->g.elev_ind == 1) 
                            || 
                   (phd->elev_cnt == 1)
                            ||
                   (phd->g.vol_t == phd->elev_t))){ 

       new_vol = TRUE;
           
       /* For REFLDATA_ELEV and COMBBASE_ELEV, both products have
          "elev_ind" and "elev_cnt" set to 1 for the first product
          of this type in the volume scan.  Therefore we distinguish
          between which is start of volume based on the product that
          is needed by the task. */   
       if( (Inp_list[DRIVING].type == REFLDATA_ELEV) 
                            ||
           (Inp_list[DRIVING].type == COMBBASE_ELEV) ){

          data_needed = WA_data_filtering( Inp_list + DRIVING,  buffer_ind  ); 
          if( data_needed == DATA_NOT_NEEDED )
             new_vol = FALSE;

       }

    }
    else if ((phd->g.vol_t == phd->elev_t)
                    ||
             (Driving_input_type == VOLUME_DATA))
       new_vol = TRUE;

   else
       new_vol = FALSE;

#ifdef DEBUG

    LE_send_msg( GL_ERROR, "--->new_vol flag set to %d\n", new_vol );

    if( phd->g.vol_t == phd->elev_t )
       LE_send_msg( GL_INFO, "------>volume time == elevation time\n" );

    else if( Driving_input_type == VOLUME_DATA )
       LE_send_msg( GL_INFO, "------>driving input type == VOLUME_DATA\n" );

    else if( Driving_input_type == ELEVATION_DATA ){

       if( phd->g.elev_ind == 1 ){

          if( phd->elev_cnt == 1 )
             LE_send_msg( GL_INFO, "------>phd->g.elev_ind == 1 && phd->elev_cnt == 1\n" );

          else
             LE_send_msg( GL_INFO, "------>phd->g.elev_ind == 1 && phd->elev_cnt != 1 (%d)\n",
                          phd->elev_cnt );

       }
       else if( phd->elev_cnt == 1 ){

          if( phd->g.elev_ind != 1 )
             LE_send_msg( GL_INFO, "------>phd->elev_cnt == 1 && phd->g.elev_ind != 1 (%d)\n", 
                          phd->g.elev_ind );

       }
       else
          LE_send_msg( GL_INFO, "------>phd->g.elev_ind != 1 (%d) && phd->elev_cnt != 1 (%d)\n", 
                       phd->g.elev_ind, phd->elev_cnt );

    }

#endif
    
    /* Update support environment. */
    Update_support_environment( (Base_data_header *) NULL, 0, 
                                new_vol, phd );

    /* Check if abort flag is set. */
    if (AP_abort_flag( UNKNOWN ) ){

        /* Abort flag is set */
	if ((Resume_time != NEW_VOLUME) ||
	    (new_vol && Resume_time == NEW_VOLUME)) {

	   if( Resume_time != NEW_VOLUME)
              LE_send_msg( GL_INFO, "Clearing Abort Flag For Resume_time != NEW_VOLUME\n" );

           else if( new_vol && Resume_time == NEW_VOLUME) {

              LE_send_msg( GL_INFO, "Clearing Abort Flag For NEW_VOLUME\n" );

              end_vol = TRUE;

           }

           /* Clear abort flag and set task resume time to 
              undefined. */
	    AP_abort_flag( FALSE );
	    Resume_time = RESUME_UNDEFINED;

	}
        else{

           end_vol = TRUE;
           processed_time = UNDEFINED;
           return(TERMINATE);

        }

    }

    /* Do data sequencing checks. */
    bad_input = FALSE;
    if (end_vol == TRUE) {			

        /* Set volume scan test number, elevation count, and reset
           end of volume flag. */
	vol_test_num = Vol_seq_num_wait_all;
	elev_cnt = phd->elev_cnt - 1;
	end_vol = FALSE;

    }
    
    /* For elevation based task we don't need sequencing check */
    if( (Driving_input_type == ELEVATION_DATA) 
                        &&	
	(Task_type != TASK_ELEVATION_BASED) ){

       /* Check for match on volume sequence number and elevation 
          count. */
       if( vol_test_num != phd->g.vol_num ){

          LE_send_msg( GL_INFO, "Volume Number Out Of Sequence (%d != %d)\n",
                       vol_test_num, phd->g.vol_num );
          bad_input = TRUE;

       } 
       else{

          if( ORPGPAT_get_class_id( Inp_list[DRIVING].type ) != BASEDATA_ELEV ){ 

             /* Make sure the elevations are in sequence. */
             if( (elev_cnt + 1) != phd->elev_cnt ){

                 LE_send_msg( GL_INFO, "Elevation Out of Sequence.  Elev Cnt (%d) != phd->elev_cnt (%d)\n",
                              (elev_cnt + 1), phd->elev_cnt ); 
	         bad_input = TRUE;

              }
	
          }
          else{

             /* Make sure the elevations are in sequence.  For FORMAT_TYPE_BASEDATA_ELEV 
                driving input, the elevation count may not change. */
             if( (elev_cnt != phd->elev_cnt) && ((elev_cnt + 1) != phd->elev_cnt) ){

                 LE_send_msg( GL_INFO, 
                              "Elevation Out of Sequence.  Elev Cnt (%d, %d) != phd->elev_cnt (%d)\n",
                              elev_cnt, (elev_cnt + 1), phd->elev_cnt ); 
	         bad_input = TRUE;

             }

          }

       }
       
       if( bad_input == FALSE )
          elev_cnt = phd->elev_cnt;

    }

    /* Input data error - Reset variables and enter the aborted 
       state. */
    if (bad_input){

	LE_send_msg (GL_INFO, "Product Input %d Is Out Of Sequence\n",
                     Inp_list[DRIVING].type );

        /* Set the end of volume flag, and clear the data processed 
           time. */
	end_vol = TRUE;
	processed_time = UNDEFINED;

        /* Make sure this driving input data will be re-processed. */
	ORPGDA_seek (Inp_list[DRIVING].data_id, -1, LB_CURRENT, NULL);

        /* Set task resume time if not already set. */
	WA_set_resume_time ();

        /* Set error return. */
	ret = TERMINATE;

        /* Set the aborted volume sequence number. */
        AP_set_aborted_volume( vol_test_num );

    }

    /* Set end of volume flag on "good" and "pseudo" end of volume. */
    if (phd->bd_status == GENDVOL || phd->bd_status == PGENDVOL) 
	end_vol = TRUE;

    /* If driving input is elevation data, update requests. */
    if (Driving_input_type == ELEVATION_DATA) {
 
	PRQ_update_requests( new_vol, (int)phd->g.elev_ind, 
			     Inp_list[DRIVING].time );
	ITC_update (new_vol);

    }
    else {

        /* Driving input is volume data or time data. */
	PRQ_update_requests( 1, (int)phd->g.elev_ind, 
                             Inp_list[DRIVING].time);
	ITC_update (1);

    }

    return (ret);

/* End of Check_data_prod() */
}

/********************************************************************

   Description: 
      This function checks whether any of outputs are requested.

   Inputs:

   Outputs:

   Return:
      TRUE if at least one output is requested or FALSE if none
      of the output requests is requested.

   Notes:

********************************************************************/
static int Output_requested (){

    int i;

    /* Search the list of outputs to determine if one of the outputs
       has been requested. */
    for (i = 0; i < N_outs; i++) {

	if (Out_list [i].requested)

            /* Output requested! */
	    return (TRUE);

    }

    /* No output requested. */
    return (FALSE);

/* End of Output_requested() */
}

/********************************************************************

   Description: 
      This function initializes variables used in this module. 

   Inputs:

   Outputs:

   Returns:

   Notes:

********************************************************************/
static void Initialize (){

    int i, cnt;

    /* If the task type is TASK_EVENT_BASED, the task will use
       the Event Services module to control task processing. */
    Task_type = INIT_task_type ();
    if( Task_type == TASK_EVENT_BASED )
       return;

    /* Get input/output data type list and number of inputs and 
       outputs. */
    N_inps = IB_inp_list (&Inp_list);
    N_outs = OB_out_list (&Out_list);
    if( (N_inps <= 0) || (N_outs <= 0) ){

        if( Monitor_only )
	   LE_send_msg( GL_INFO, "Input or output data type is not specified\n" );

        else
	   PS_task_abort ("Input or output data type is not specified\n");

    }

    /* Get the task's input stream type. */
    Input_stream = INIT_get_task_input_stream();

    /* Set the Aborted flag, initialize the task_type (TASK_VOLUME_BASED, 
       TASK_ELEVATION_BASED, etc. ), and task resumption time. */
    AP_abort_flag( TRUE );
    AP_set_aborted_volume( ORPGVST_get_volume_number() );
    Alg_control_flg = AP_alg_control( PBD_ABORT_UNKNOWN );
    WA_set_resume_time ();

    /* Set Driving_input_type. */
    Driving_input_type = UNDEFINED_DATA;

    /* Verify the input data timing. */
    cnt = 0;
    for( i = 0; i < N_inps; i++ ){

	if (Inp_list [i].timing == RADIAL_DATA) {

	    if( Driving_input_type == RADIAL_DATA )
		PS_task_abort ("Only a single radial input is allowed\n");

	    else{

		Driving_input_type = RADIAL_DATA;
		cnt++;

	    }

	}

    }

    /* If driving input type still undefined. */
    if( Driving_input_type == UNDEFINED_DATA ){

	for( i = 0; i < N_inps; i++ ){


	    if( Inp_list [i].timing == ELEVATION_DATA ){

		Driving_input_type = ELEVATION_DATA;
		cnt++;
		break;

	    }

	}

    }

    /* If driving input type is still undefined. */
    if( Driving_input_type == UNDEFINED_DATA ){

	for( i = 0; i < N_inps; i++ ){

	    if( Inp_list [i].timing == VOLUME_DATA ){

		Driving_input_type = VOLUME_DATA;
		cnt++;
		break;

	    }

	}

    }

    /* If driving input type is still undefine. */
    if( Driving_input_type == UNDEFINED_DATA ){

	for( i = 0; i < N_inps; i++ ){

	    if( Inp_list [i].timing == DEMAND_DATA ){

		Driving_input_type = DEMAND_DATA;
		cnt++;
		break;

	    }

	}

    }

    /* Time based data. */
    if( Driving_input_type == UNDEFINED_DATA )
	Driving_input_type = 0;	

    /* Driving input must be first input registered. */
    if( Driving_input_type != Inp_list [DRIVING].timing )
	PS_task_abort ("Driving input must be the first input\n");

    if( cnt > 1 )
	PS_task_abort 
	("All inputs must be either time based or using the driving input timing\n");


    return;

/* End of Initialize() */
}

/********************************************************************

   Description: 
       This function is used to control the initialization processing 
       of RPG algorithm/product task which are activated based the 
       availability of any of the task inputs.  The task inputs must be 
       non-radial based.

   Inputs:
       wait_for - Must be WAIT_ANY_INPUT.

   Outputs:

   Returns:

   Notes:

********************************************************************/
static void Wait_for_any_init( int wait_for ){

   int i;

   /* Make sure task waits for any input .... */
   if( wait_for != WAIT_ANY_INPUT ) 
      PS_task_abort("Illegal call to RPG_wait_for_any_data - WAIT_DRIVING_INPUT\n"); 

   /* First time initialization. */
   if( (INIT_task_type() != TASK_VOLUME_BASED)
                        &&
       (INIT_task_type() != TASK_ELEVATION_BASED) )
      PS_task_abort( "Task Must Be VOLUME or ELEVATION BASED\n" );

   /* Save this wait_for since it is used throughout this module. */
     Wait_for = wait_for;

   /* Do Until volume sequence number changes. */
   while(1){

      /* Get the volume sequence number from the volume status data. */
      Vol_seq_num_wait_any = ORPGVST_get_volume_number();

      if( Vol_seq_num_wait_any != ORPGVST_DATA_NOT_FOUND ){

         /* Set the volume sequence number in the VI and PRQ modules. */
         VI_set_vol_seq_num( Vol_seq_num_wait_any );
         PRQ_set_vol_seq_num( Vol_seq_num_wait_any );

         /* If volume sequence number has changed, break out of loop. */
         if( Old_vol_seq_num_wait_any < 0 )
               Old_vol_seq_num_wait_any = Vol_seq_num_wait_any;

         if( Vol_seq_num_wait_any != Old_vol_seq_num_wait_any ){

            /* Set file pointers for all registered inputs to one 
               beyond last message in input buffer LB. */
            for( i = 0; i < N_inps; i++ )
               ORPGDA_seek( Inp_list[i].data_id, 1, LB_LATEST, NULL );
    
            /* Set the resume time. */
            WA_set_resume_time();

            /* Break out of "while" loop. */
            break;

         }

      }
      else 
         PS_task_abort( "ORPGVST_get_volume_number Failed (%d)\n",
                        Vol_seq_num_wait_any );

      /* Sleep for a while and try again. */
      EN_control (EN_WAIT, 1000);

   /* End of "while" loop. */ 
   }

/* End of Wait_for_any_init() */
}

/********************************************************************

   Description: 
       This function updates the support environment.  See PS, SS, 
       OB, ADE, and ADP modules for further information.

   Inputs:      
       bhd - basedata header. 
       tm -  data time (either current time, start of 
             elevation time, or start of volume time.
       new_vol - flag, if set, indicates new volume scan.
       phd - orpg product header.

   Return:
       There is no return value define for this function.

********************************************************************/
static void Update_support_environment( Base_data_header *bhd, time_t tm, 
                                        int new_vol, Prod_header *phd ){

   unsigned int vol_num;

   /* Update supporting environment. */
   if( bhd != (Base_data_header *) NULL ){

      /* Send radial header to the PS module, update scan summary 
         array, and set product header for output. */
      PS_register_bd_hd (bhd);	
      SS_update_summary (bhd);	
      VS_update_volume_status (bhd);	

      OB_set_prod_hdr_info (bhd, (char *) NULL, new_vol);	

      /* Set data elevation/volume time and volume number. */
      Inp_list[DRIVING].elev_time = tm;
      Inp_list[DRIVING].rpg_elev_index = bhd->rpg_elev_ind;
      Inp_list[DRIVING].vol_time = PS_get_volume_time (bhd);

      vol_num = ORPGMISC_vol_seq_num( (int) bhd->vol_num_quotient,
                                      (int) bhd->volume_scan_num );
      Inp_list[DRIVING].vol_num = vol_num;

#ifdef DEBUG

      LE_send_msg( GL_ERROR, "--->Updating Support Environment.  new_vol: %d\n",
                   new_vol );

      LE_send_msg( GL_INFO, "------>elev_time: %d, vol_time: %d, vol_num: %d rpg_elev_index: %d\n",
                   Inp_list[DRIVING].elev_time, Inp_list[DRIVING].vol_time, 
                   Inp_list[DRIVING].vol_num, Inp_list[DRIVING].rpg_elev_index );

#endif

   }

   /* If product header pointer is defined, then ... */
   if( phd != (Prod_header *) NULL ){

      /* Send header info to product output module, update adaptation
         data, set scan summary data and volume status data. */
      OB_set_prod_hdr_info( (Base_data_header *) NULL, (char *)phd, new_vol );
      SS_read_scan_summary();
      VS_read_volume_status();

   }
 
   /* Update adaptation data */
   ADP_update_adaptation (new_vol);
   ADE_update_ade (new_vol);

/* End of Update_support_environment() */
}

/**********************************************************************

   Description:
      This function is responsible for waiting for a replay
      request before initiating algorithm processing.

      If there is a product request, a check is made for the 
      driving input availability.  If not available, then 
      we wait for the next request.  Otherwise, we return.

   Inputs:

   Outputs:

   Returns:
      Returns 0 if product request and driving input available.

   Notes:

**********************************************************************/
static int Wait_for_replay_request(){

   int num_reqs, status;
   Prod_request *requests;

   LE_send_msg( GL_INFO, "Waiting For Replay Product Request.\n" );

   while(1){

      /* Read the replay (one-time) product request LB for a product
         request. */
      requests = PRQ_check_for_replay_requests( &num_reqs );

      /* If there are requests for products, check for availability 
         of the driving input data. */
      if( num_reqs > 0 ){

         /* The driving input timing is RADIAL_DATA. */
         if( Inp_list[DRIVING].timing == RADIAL_DATA ){

            status = Check_radial_replay_availability( requests );
            if( status == NORMAL )
               return (0);

            continue;

         }

         /* The driving input is in the product database as warehoused
            data. */
         else if( Inp_list[DRIVING].timing == ELEVATION_DATA 
                                     ||
                  Inp_list[DRIVING].timing == VOLUME_DATA ){

            /* Set the product database query information. */
            status = Set_query_info( requests );
            if( status == NORMAL ){

               LB_id_t msg_id;

               /* Query the product database for the product. */
               msg_id = IB_product_database_query( DRIVING );
               if( msg_id == 0 ){

                  /* Data not found so abort the request. */
                  Abort_replay_request( requests );

               }
               else
                  return (0);

            }
            else
               Abort_replay_request( requests );

            continue;

         }
         else{

            /* For all other timings, just continue. */
            continue;

         }

      }
      else{

         /* No request for products which can be satisfied by this task.  
            Sleep a short time, then check again. */
         EN_control (EN_WAIT, 1000);
         continue;

      }
 
   }

   return (0);

/* End of Wait_for_replay_request() */
}

/***********************************************************************

   Description:
      This function is responsible for setting query information to
      be used during the product database query.  The volume time is
      derived from the volume sequence number and in the case of 
      ELEVATION_BASED driving input, the elevation angle corresponding 
      to the elevation index is derived.

   Inputs:
      requests - pointer to product request.

   Outputs:

   Returns:
      If successful, then NORMAL is returned.  Otherwise,
      NO_DATA is returned.

   Notes:

***********************************************************************/
static int Set_query_info( Prod_request *requests ){

   int elev_ang = -999;
   time_t vol_time;

   /* Set the replay request volume sequence number and elevation index. */
   Replay_info.replay_vol_seq_num = requests->vol_seq_num;
   Replay_info.replay_elev_ind = requests->elev_ind;

   /* Initialize all the other fields in replay request info. */
   Replay_info.replay_vol_time = -1;
   Replay_info.replay_elev_ang = -999;

   /* Extract information about the product to be queried. */
   vol_time = SS_get_volume_time( (unsigned int) requests->vol_seq_num );
   if( vol_time == 0 )
      return (NO_DATA);

   /* Set the replay request info volume time. */
   Replay_info.replay_vol_time = vol_time;

   /* If product is elevation data, get the elevation angle (*10)
      we which to query on. */
   if( Inp_list[DRIVING].timing == ELEVATION_DATA ){

      int index;

      /* Get the index of the elevation parameter. */
      index = ORPGPAT_elevation_based( requests->pid );
      if( index >= 0 ){

         /* Set the elevation parameter. */
         switch( index ){

            case 0:
            {
               elev_ang = requests->param_1;
               break;
            }
            case 1:
            {
               elev_ang = requests->param_2;
               break;
            }
            case 2:
            {
               elev_ang = requests->param_3;
               break;
            }
            case 3:
            {
               elev_ang = requests->param_4;
               break;
            }
            case 4:
            {
               elev_ang = requests->param_5;
               break;
            }
            case 5:
            {
               elev_ang = requests->param_6;
               break;
            }
            default:
               PS_task_abort( "Elevation Angle Not Found.\n" );

         }

      }

   }

   /* Set the replay request info elevation angle. */
   Replay_info.replay_elev_ang = elev_ang;
   
   /* Set the replay request type. */
   Replay_info.replay_type = requests->type;
   if( Replay_info.replay_type == ALERT_OT_REQUEST )
      LE_send_msg( GL_INFO, "Servicing Alert Paired-Product OT Request For Prod ID %d\n",
                   requests->pid );
   else if( Replay_info.replay_type == USER_OT_REQUEST )
      LE_send_msg( GL_INFO, "Servicing User OT Request For Prod ID %d\n",
                   requests->pid );

   /* Return NORMAL. */
   return (NORMAL);

/* End of Set_query_info() */
}

/***********************************************************************

   Description:
      Depending on the task timing, we check for the availability 
      radial replay data.  If available, we return NORMAL.
      If not available, then we initiate task abort processing
      and return NO_DATA.

   Inputs:
      requests - product request information needed to make 
                 driving input availability check.

   Outputs:

   Returns:
      NORMAL if driving input is available, or NO_DATA if data
      is not available.

   Notes:
      A change is necessary to support SAILS and AVSET.  When a product
      request is received, it carries an elevation index corresponding
      to the requested elevation angle for elevation-base products.
      The problem is that AVSET can change to location of the SAILS
      cut so the location of a elevation in the current volume may
      be different from the location in the previous volume.  To
      account for this code was added to redo the elevation index 
      in the request when interrogating the previous volume scan 
      for data availability.  

***********************************************************************/
static int Check_radial_replay_availability( Prod_request *requests ){

   LB_id_t msg_id = 0;
   unsigned int vol_seq_num;
   int vol_scan = 0, bytes_read = 0;
   int index, elev_ind, diff = 0;

   static RDA_rdacnt_t rda_rdacnt;

   if( requests->type == ALERT_OT_REQUEST )
      LE_send_msg( GL_INFO, "Servicing Alert Paired-Product OT Request For Prod ID %d\n",
                   requests->pid );
   else if( requests->type == USER_OT_REQUEST )
      LE_send_msg( GL_INFO, "Servicing User OT Request For Prod ID %d\n",
                   requests->pid );

   /* Set the volume sequence number to the volume sequence number in
      the product request. */
   vol_seq_num = (unsigned int) requests->vol_seq_num;

   /* Check the volume sequence number in the request first for availability
      of data, then check the previous volume if necessary. */
   while( (diff = abs( (int) (vol_seq_num - requests->vol_seq_num))) <= 1 ){

      /* "diff" is one if it is the previous volume scan.  We are only
         concerned with elevation-based tasks and the previous volume scan. */
      if( (Task_type == TASK_ELEVATION_BASED) && (diff == 1) ){

         /* Find the elev_ind closest to the requested angle.  It may be different
            if the previous volume is a different VCP or SAILS/AVSET is/was active. */
      
         /* Read the RDA RDACNT data for this volume scan. */
         bytes_read = ORPGDA_read( ORPGDAT_ADAPTATION, &rda_rdacnt,
                                   sizeof(RDA_rdacnt_t), RDA_RDACNT );

         /* If an error occurred reading the accounting data, then
            set the "msg_id" to 0 and break out of while loop. */
         if( bytes_read <= 0 ){

            LE_send_msg( GL_INFO, "ORPGDA_read( ORPGDAT_ADAPTATION, RDA_RDACNT ) Read Failed (%d)\n",
                         bytes_read );
            LE_send_msg( GL_INFO, "-->Set Replay Data Unavailable.\n" );
            msg_id = 0;
            break;

         }

         /* Extract information and verify.  Make sure the volume scan number in
            the RDACNT data matches the volume scan number derived from the request. */
         vol_scan = ORPGMISC_vol_scan_num( requests->vol_seq_num );
         index = vol_scan % 2;
         if( vol_scan != rda_rdacnt.data[index].volume_scan_number ){

            LE_send_msg( GL_INFO, "Request Vol Scan # (%d) != RDA_rdacnt Vol Scan # (%d)\n",
                         vol_scan, rda_rdacnt.data[index].volume_scan_number );
            LE_send_msg( GL_INFO, "-->Set Replay Data Unavailable.\n" );
            msg_id = 0;
            break;

         }

         /* Find the closest elevation to requested elevation.  Then
            use the elevation index to check for data availability. */
         elev_ind = Replay_find_closest_elev_ind( requests, &rda_rdacnt.data[index] );
         LE_send_msg( GL_INFO, "Replay: Requested Index: %d, Derived Index: %d\n",
                      requests->elev_ind, elev_ind );

         /* elev_ind < 1 implies some error occurred. */
         if( elev_ind < 1 ){

            LE_send_msg( GL_INFO, "Replay_find_closest_elev_ind Returned Error\n" );
            LE_send_msg( GL_INFO, "-->Set Replay Data Unavailable.\n" );
            msg_id = 0;
            break;

         }
        
         /* Change the index in the request message. */
         requests->elev_ind = elev_ind;

      }

      /* Check if the requisite data is available. */
      msg_id = WA_check_replay_elev_vol_availability( Inp_list[DRIVING].data_id,
                                                      Inp_list[DRIVING].type_mask,
                                                      requests->vol_seq_num, 
                                                      requests->elev_ind );

      /* A message ID > 0 indicates the data is available so process
         request. */
      if( msg_id != 0 ){

         /* Data is available so set the LB read pointer now. */
         if( ORPGBDR_set_read_pointer( Inp_list[DRIVING].data_id, msg_id, 
                                       0 ) == 0 ){

            LE_send_msg( GL_INFO, "Replay Data is AVAILABLE\n" );
            return (NORMAL);

         }
         else
            break;

      }

      /* Check the previous volume scan for available data. */
      if( requests->type == ALERT_OT_REQUEST || requests->vol_seq_num <= 1 )
         break;

      requests->vol_seq_num--;

   }
   
   /* "msg_id" <= 0 indicates the data is not available.  A negative
      return from setting read pointer indicates an error.  Abort
      processing and wait for next request. */
   requests->vol_seq_num = vol_seq_num;
   Abort_replay_request( requests );
   return( NO_DATA );

/* End of Check_radial_replay_availability() */
}

/**********************************************************************

   Description:
      Depending on the task timing, we check for the
      availability of the data.  If available, we return NORMAL.
      If not available, then we initiate task abort processing
      and return NO_DATA.

   Inputs:
      data_store_id - data store id
      sub_type - data type (COMBBASE_TYPE, REFLDATA_TYPE, ... )
      vol_seq_num - volume sequence number
      elev_ind - RPG elevation index

   Outputs:

   Returns:
      NORMAL if data is available, or NO_DATA if data is not available.

   Notes:

***********************************************************************/
int WA_check_replay_elev_vol_availability( int data_store_id, int sub_type,
                                           int vol_seq_num, int elev_ind ){

   LB_id_t msg_id = 0;

   /* Check the basedata replay accounting information for the desired data 
      if driving input is RADIAL_DATA. */
   if( Task_type == TASK_ELEVATION_BASED ){

      /* To satisfy this request, we need a complete elevation of data. */
      LE_send_msg( GL_INFO, "Checking Complete Elevation %d Availability\n", elev_ind );
      LE_send_msg( GL_INFO, "--->Volume Seq #: %d, Data ID: %d, Sub-Type: %d\n",
                   vol_seq_num, data_store_id, sub_type  );
      msg_id = ORPGBDR_check_complete_elevation( data_store_id, sub_type, vol_seq_num,
                                                 elev_ind );
   }
   else if( Task_type == TASK_VOLUME_BASED ){

      /* To satisfy this request, we need a complete volume scan of data. */                 
      LE_send_msg( GL_INFO, "Checking Complete Volume Availability\n" );
      LE_send_msg( GL_INFO, "--->Volume Seq #: %d, Data ID: %d, Sub-Type: %d\n",
                   vol_seq_num, data_store_id, sub_type  );
      msg_id = ORPGBDR_check_complete_volume( data_store_id, sub_type, vol_seq_num );
                                        
   }

   /* A message ID > 0 indicates the data is available so process
      request. */
   if( msg_id != 0 ){                

      /* Data is available so set the LB read pointer now. */
      if( ORPGBDR_set_read_pointer( data_store_id, msg_id, 0 ) == 0 ){

         LE_send_msg( GL_INFO, "Replay Data is AVAILABLE\n" );
         return (msg_id);

      }                              

   }

   return (msg_id);

/* End of Check_replay_elev_vol_availability() */
}

/***************************************************************************

   Description:
      Abort processing module for aborting a replay request because driving
      input is not available.

   Inputs:
      request - product request.
      vol_seq_num - volume sequence number.

   Outputs:

   Returns:

   Notes:

***************************************************************************/
static void Abort_replay_request( Prod_request *request ){

   LE_send_msg( GL_ERROR, "Unable To Satisfy Replay Request\n");
   LE_send_msg( GL_ERROR, "-->Input Data For Prod ID %d Not Available For Volume Seq # %d\n",
                request->pid, request->vol_seq_num );

   /* Set the volume sequence number of the aborted volume scan. */
   AP_set_aborted_volume( (int) request->vol_seq_num );

   /* Abort the product request. */
   AP_abort_request( request, PGM_REPLAY_DATA_UNAVAILABLE );

   /* Re-initialize the aborted products lists for next product request. */
   AP_init_prod_list( request->vol_seq_num );

/* End of Abort_replay_request() */
}

/********************************************************************

   Description:
      Checks the algorithm control flag in the base data radial
      header.  If flag indicates an abort condition, set aborting
      volume sequence number and return TERMINATE.

   Inputs:
      bhd - pointer to base data radial header.

   Outputs:

   Returns:
      NORMAL if no abort condition, or TERMINATE on abort condition.

   Notes:

*******************************************************************/
static int Check_alg_control( Base_data_header *bhd ){

   int vol_quotient, vol_scan_num;
   int ret = NORMAL;

   int alg_ctrl = (int) bhd->pbd_alg_control;

   /* Check algorithm control flag. */
   if( (Alg_control_flg = 
       AP_alg_control( (int) (alg_ctrl &  PBD_ABORT_FLAG_MASK) )) != PBD_ABORT_NO ){

      int alg_ctrl_reason = (int) (alg_ctrl & PBD_ABORT_REASON_MASK);

      LE_send_msg( GL_INFO, "The Algorithm Control Flag != NORMAL (%d)\n",
                   Alg_control_flg );

      /* Set abort reason code and aborted volume scan sequence number. */
      switch( alg_ctrl_reason ){

         case PBD_ABORT_RADIAL_MESSAGE_EXPIRED:
            LE_send_msg( GL_INFO, "Algorithm Control Reason:  RADIAL MESSAGE EXPIRED (%d)\n",
                         PBD_ABORT_RADIAL_MESSAGE_EXPIRED );
            AP_set_abort_reason( PGM_INPUT_DATA_ERROR );
            break;

         case PBD_ABORT_VCP_RESTART_COMMANDED:
            LE_send_msg( GL_INFO, "Algorithm Control Reason:  VCP RESTART COMMANDED (%d)\n",
                         PBD_ABORT_VCP_RESTART_COMMANDED );
            AP_set_abort_reason( PGM_SCAN_ABORT );
            break;

         case PBD_ABORT_INPUT_DATA_ERROR:
            LE_send_msg( GL_INFO, "Algorithm Control Reason:  INPUT DATA ERROR (%d)\n",
                         PBD_ABORT_INPUT_DATA_ERROR );
            AP_set_abort_reason( PGM_INPUT_DATA_ERROR );
            break;

         case PBD_ABORT_UNEXPECTED_VCP_RESTART:
            LE_send_msg( GL_INFO, "Algorithm Control Reason:  UNEXPECTED VCP RESTART (%d)\n",
                         PBD_ABORT_UNEXPECTED_VCP_RESTART );
            AP_set_abort_reason( PGM_SCAN_ABORT );
            break;

         default:
            LE_send_msg( GL_INFO, "Algorithm Control Reason:  *** UNKNOWN *** (%d)\n",
                         alg_ctrl_reason );
            AP_set_abort_reason( PGM_SCAN_ABORT );
            break;

      /* End of "switch" */
      }

      /* The Process Base Data process sets the volume scan number as 
         modulus( volume sequence number, MAX_SCAN_SUM_VOLS ) and the volume 
         quotient as volume sequence number divided by MAX_SCAN_SUM_VOLS.   The
         ORPGMISC_vol_seq_num() derives the volume sequence number as:
         (volume scan number)*(volume quotient) + modulus( volume scan number,
         MAX_SCAN_SUM_VOLS ).   Therefore to derive the aborted volume scan
         correctly, we must subtract 1 from the volume quotient if the aborted
         volume scan is less than the current volume scan and the current volume
         scan equals MAX_SCAN_SUM_VOLS. */
      vol_scan_num = (int) bhd->pbd_aborted_volume;
      vol_quotient = (int) bhd->vol_num_quotient;
      if( bhd->volume_scan_num == (short) bhd->pbd_aborted_volume )
         vol_quotient = (int) bhd->vol_num_quotient;

      else{

         if( (bhd->volume_scan_num == MAX_SCAN_SUM_VOLS) 
                                   &&
             (bhd->pbd_aborted_volume == (MAX_SCAN_SUM_VOLS-1)) )
            vol_quotient = (int) bhd->vol_num_quotient - 1;

      }

      Aborted_vol_seq_num = ORPGMISC_vol_seq_num( vol_quotient, vol_scan_num );
      AP_set_aborted_volume( Aborted_vol_seq_num );

      /* Set resume time base on Task_type and algorithm control flag. */
      if( Task_type == TASK_VOLUME_BASED ){

         /* Abort and wait until next elevation to resume 
            processing. */
	 if( Alg_control_flg == PBD_ABORT_FOR_NEW_EE ){

	    Resume_time = NEW_ELEVATION;
            ret = TERMINATE;

	 }

	 /* Abort and wait until next volume to resume processing. */
	 else if( (Alg_control_flg == PBD_ABORT_FOR_NEW_VV)
                                   ||
	          (Alg_control_flg == PBD_ABORT_FOR_NEW_EV) ){

	    Resume_time = NEW_VOLUME;
	    ret = TERMINATE;

	 }

      }
      else if( Task_type == TASK_ELEVATION_BASED ){

         /* Abort and wait until next elevation to resume processing. */
	 if( (Alg_control_flg == PBD_ABORT_FOR_NEW_EE)
                              ||
	     (Alg_control_flg == PBD_ABORT_FOR_NEW_EV) ){

	    Resume_time = NEW_ELEVATION;
	    ret = TERMINATE;

	 }

	 /* Abort and wait until next volume to resume processing. */
	 else if( Alg_control_flg == PBD_ABORT_FOR_NEW_VV ){

	    Resume_time = NEW_VOLUME;
	    ret = TERMINATE;

	 }

      }
      else if( Task_type == TASK_RADIAL_BASED ){

	 /* If the algorithm control flag specifies abort and wait for 
            next volume for Elevation-based and Volume-Based, we we do 
            the same for Radial-based. */
	 if( Alg_control_flg == PBD_ABORT_FOR_NEW_VV ){

	    Resume_time = NEW_VOLUME;
	    ret = TERMINATE;

	 }

      }

   }

   /* Return "ret" value. */
   return (ret);

/* End of Check_alg_control() */
}

/****************************************************************************

   Description:
      Performs first pass of the radial sequencing checks.  Checks for 
      unexpected start of volume scan or elevation cuts out of sequence.

   Inputs:
      bad_input - flag for indicating bad input.
      test - pointer to test variable structure.
      bhd - pointer to base data radial header.

   Outputs:

   Returns:
      FALSE if no errors found or TRUE if errors found.

   Notes:

*****************************************************************************/
static int Radial_seq_check_pass_1( Rad_test_t *test, Base_data_header *bhd ){

   int bad_input = FALSE;
 
   if( test->new_vol ){

      /* Did new volume occur without previous end of volume? */
      if( !test->end_vol ){	

         if( bhd->status == GOODBVOL )
            LE_send_msg( GL_INFO, "Unexpected Start of Volume %d Encountered\n",
                         Vol_seq_num_wait_all );

         else
            LE_send_msg( GL_INFO, "Unexpected Change to Volume %d Encountered\n",
                         Vol_seq_num_wait_all );

         /* Set input as bad to allow for algorithm cleanup. */
	 bad_input = TRUE;

         /* Set the aborted volume sequence number. */
         if( Aborted_vol_seq_num == 0 ){

            Aborted_vol_seq_num = Vol_seq_num_wait_all - 1;
            AP_set_aborted_volume( Aborted_vol_seq_num );

         }

      }
      else{ 

         test->end_vol = test->end_elev = FALSE;
	 test->vol_test_num = Vol_seq_num_wait_all;
	 test->rpg_cut_num = bhd->rpg_elev_ind;
	 test->rda_cut_num = bhd->elev_num;
	 test->azi_num = bhd->azi_num;

         test->n_cuts = bhd->elev_num;

      }

   }
   else if( test->new_elev ){

      /* Did new elevation occur with previous end of elevation? */
      if( test->end_elev ){

         /* Check for elevation cut number out of sequence. */
         if( test->rpg_cut_num != UNDEFINED ){

            if( test->rpg_cut_num != bhd->rpg_elev_ind ){

               if( (test->rpg_cut_num + 1) != bhd->rpg_elev_ind ){

                  /* Check if this is a supplemental cut.  If so, a gap in sequence
                     is OK.  Normally the upstream generator of this data will be
                     allowed to process the supplemental cut and therefore there
                     should not be a gap.  However we have to support the case that 
                     the upstream producer is not allowed to process supplemental cuts. */
                  if( (bhd->suppl_flags & RDACNT_SUPPL_SCAN) == 0 ){

                     LE_send_msg (GL_INFO,
  	                  "New Elev # (flags: %d) Out Of Seq Error (%d+1 != %d) @ EL/AZM %4.1f/%5.1f\n",
		          bhd->suppl_flags, test->rpg_cut_num, bhd->rpg_elev_ind, bhd->elevation, bhd->azimuth);
                     LE_send_msg (GL_INFO,
  	                  "--->test->rda_cut_num: %d, bhd->elev_num: %d\n", test->rda_cut_num, bhd->elev_num );

                     bad_input = TRUE;

                  }

               }

            } 

         }

      }
      else if( test->rpg_cut_num != bhd->rpg_elev_ind ) {

         /* A new cut started without the end of the previous cut and the new
            cut has a different cut number. */
         LE_send_msg (GL_INFO,
              "New Elev # (flags: %d) Out Of Seq Error (%d+1 != %d) @ EL/AZM %4.1f/%5.1f\n",
              bhd->suppl_flags, test->rpg_cut_num, bhd->rpg_elev_ind, bhd->elevation, bhd->azimuth);
         LE_send_msg (GL_INFO,
              "--->test->rda_cut_num: %d, bhd->elev_num: %d\n", test->rda_cut_num, bhd->elev_num );

         bad_input = TRUE;

      }

      test->end_elev = FALSE;
      test->rpg_cut_num = bhd->rpg_elev_ind;
      test->rda_cut_num = bhd->elev_num;
      test->azi_num = bhd->azi_num;

      test->n_cuts = bhd->elev_num;

   }

   return (bad_input);

/*End of Radial_seq_check_pass_1() */
}

/****************************************************************************

   Description:
      Performs the remaining radial sequencing checks.  Checks for azimuth
      numbers out of sequence, cut numbers out of sequence, and volume scan
      sequence number out of sequence.

   Inputs:
      test - pointer to structure containing test variables.
      bhd - pointer to base data radial header.

   Outputs:

   Returns:
      TRUE if sequencing errors found, or the value of bad_input.

   Notes:

***************************************************************************/
static int Radial_seq_check_pass_2( Rad_test_t *test, Base_data_header *bhd ){

   int bad_input = FALSE;
   static Base_data_header *p_bhd = NULL;

   if( p_bhd == NULL ){

      p_bhd = calloc( 1, sizeof(Base_data_header) );
      memcpy( p_bhd, bhd, sizeof(Base_data_header) );

   }
 
   /* First, check for radials out of sequence (az # sequence error.) */ 
   if( test->azi_num + 1 != bhd->azi_num ){

      if ( test->azi_num != UNDEFINED && !test->new_scan ){

         LE_send_msg (GL_INFO,
	     "Azimuth # Seq Error (Expected: %d+1 != Actual: %d) @ EL/AZM %4.1f/%5.1f, Rad Stat: %d\n", 
	      test->azi_num, bhd->azi_num, bhd->elevation, bhd->azimuth, bhd->status);
         LE_send_msg (GL_INFO,
             "--->Prev Seq #: %d, Prev EL/AZM: %4.1f/%5.1f, Prev Rad Stat: %d\n",
              p_bhd->azi_num, p_bhd->elevation, p_bhd->azimuth, p_bhd->status);
         LE_send_msg (GL_INFO, "--->RPG Elev #: %d, Prev RPG Elev #: %d\n",
                      bhd->rpg_elev_ind, p_bhd->rpg_elev_ind );


         bad_input = TRUE;

      }

   }
   else
       test->azi_num = bhd->azi_num;

   /* Check for unexpected elevation scan number change. */
   if( test->rda_cut_num != UNDEFINED ){

      if( test->rda_cut_num != bhd->elev_num ){

         if( !bad_input )
            LE_send_msg (GL_INFO,
	        "RDA Elev # Changed Unexpectedly (%d != %d) @ EL/AZM %4.1f/%5.1f\n",
	        test->rda_cut_num, bhd->elev_num, bhd->elevation, bhd->azimuth);

         bad_input = TRUE;

      }

      if( test->rpg_cut_num != bhd->rpg_elev_ind ){

         if( !bad_input )
            LE_send_msg (GL_INFO,
	        "RPG Elev # Changed Unexpectedly (%d != %d) @ EL/AZM %4.1f/%5.1f\n",
	        test->rpg_cut_num, bhd->rpg_elev_ind, bhd->elevation, bhd->azimuth);

         bad_input = TRUE;

      }

   }

   /* Check for unexpected volume scan sequence number change. */
   if( (test->vol_test_num != 0) 
              && 
       (test->vol_test_num != Vol_seq_num_wait_all) ){

      if( !bad_input )
         LE_send_msg (GL_INFO,
	        "Unexpected Vol Seq # Change: (new %d, old %d)\n",
	        Vol_seq_num_wait_all, test->vol_test_num);

      bad_input = TRUE;

   }

   /* Copy current header to previous. */
   memcpy( p_bhd, bhd, sizeof(Base_data_header) );

   return (bad_input);

/* End of Radial_seq_check_pass_2() */
}

/********************************************************************

   Description: 
      This function initializes the test variables used in WA_check_data. 

   Inputs:
      Pointer to test variable structure.
      vol_num - volume sequence number.
      end_vol - value to set end of volume flag.
      end_elev - value to set end of elevationb flag.

   Outputs:

   Returns:

   Notes:

********************************************************************/
static void Init_test_vars ( Rad_test_t *test, unsigned int vol_num,
                             int end_vol, int end_elev ){

   test->rpg_cut_num = test->rda_cut_num = UNDEFINED;
   test->azi_num = test->processed_time = UNDEFINED;
   test->vol_test_num = vol_num;
   test->end_vol = end_vol;
   test->end_elev = end_elev;

/* End of Init_test_vars() */
}

/*******************************************************************

   Description:
      Check if previous volume has ended.   If so, were the
      number of elevation cuts in the VCP the same as what was
      expected based on the VCP definition (this would happen, 
      for example, if AVSET were running).  If so, then let's 
      abort volume scan for elevation-based products to clear
      out any product requests for unscanned cuts that will 
      never be satisfied. 

      Note:  Currently the only know reason a VCP would end 
      prematurely and not be unexpected is because of AVSET.  
      Normally the AVSET flag would be checked.   Unfortunately 
      this is not totally reliable when playing back data. 

   Returns:
      Currently there is no return value defined.

*******************************************************************/
static int Check_avset(){

    int avset_status = ORPGRDA_get_status( RS_AVSET );
    int olind;
    unsigned int vol_seq_num = 0;

    /* Nothing to do if AVSET not enabled. */
    if( avset_status != AVSET_ENABLED )
        return 0;

    /* Set volume sequence number in the event products need to be
       aborted. */
    if( Wait_for == WAIT_DRIVING_INPUT )
       vol_seq_num = Vol_seq_num_wait_all;

    else if( Wait_for == WAIT_ANY_INPUT )
        vol_seq_num = Vol_seq_num_wait_any;
    
    /* For Radial driving input and Elevation-based outputs. */
    if( (Inp_list[DRIVING].timing == RADIAL_DATA)
                          &&
              (!AP_abort_flag( UNKNOWN )) ){

        /* Test.env_vol is set to TRUE by default but Test.vcp is not set
           unless the task is actually processing input (i.e., it must be
           scheduled. */
        if( (Test.vcp != UNDEFINED) && (Test.end_vol == TRUE) ){

            /* Is the number of cuts processed less than expected for this VCP? */
            if( Test.n_cuts < Test.n_exp_cuts ){

                /* Do for all outputs registered.... */
                for( olind = 0; olind < N_outs; olind++ ){

                    /* Only care about ELEVATION_DATA ..... */
                    if( Out_list[olind].timing == ELEVATION_DATA ){

                        /* Set the aborted volume number in the event products 
                           are aborted. */
                        AP_set_aborted_volume( vol_seq_num );

                        LE_send_msg( GL_INFO, "--->Check_avset: 1 - Aborting Product Type: %d\n",
                                     Out_list[olind].type );
                        RPGC_abort_datatype_because( Out_list[olind].type,
                                                     PGM_PROD_NOT_GENERATED );

                        /* Reset the aborted volume number. */
                        AP_set_aborted_volume( 0 );

                    }

                }

            }

            /* Reset to UNDEFINED .... it will be initialized when it starts 
               processing input data. */
            Test.vcp = UNDEFINED;
            Test.n_cuts = UNDEFINED;
            Test.n_exp_cuts = UNDEFINED;
            Test.end_vol = UNDEFINED;

        }

    }

    /* For Elevation Based and Volume Based tasks ...... */
    else if( ((Task_type == TASK_ELEVATION_BASED)
                    ||
         (Task_type == TASK_VOLUME_BASED))
                    &&
        (!AP_abort_flag( UNKNOWN )) ){

        /* Do for all outputs registered.... */
        for( olind = 0; olind < N_outs; olind++ ){

            /* Only care about ELEVATION_DATA ..... */
            if( Out_list[olind].timing == ELEVATION_DATA ){

                /* Did end of volume occur and is the number of cuts less
                   than expected? */
                if( (Vtest.end_vol[olind] == TRUE)
                                  &&
                    (Vtest.n_cuts[olind] < Vtest.n_exp_cuts) ){

                    /* Set the aborted volume number in the event products 
                       are aborted. */
                    AP_set_aborted_volume( vol_seq_num );

                    /* Abort the product datatype indicating it will not
                       be generated. */
                    LE_send_msg( GL_INFO, "--->Check_avset: 2 - Aborting Product Type: %d\n",
                                 Out_list[olind].type );
                    RPGC_abort_datatype_because( Out_list[olind].type,
                                                 PGM_PROD_NOT_GENERATED );

                    /* Reset the end_vol and n_cuts flags. */
                    Vtest.end_vol[olind] = UNDEFINED;
                    Vtest.n_cuts[olind] = ECUTMAX;

                    /* Reset the aborted volume number. */
                    AP_set_aborted_volume( vol_seq_num );

                }

            }

        } /* End of "for" loop. */

    } 

    /* Return to caller. */
    return 0;

/* End of Check_avset() */
}

/*******************************************************************

   Description:
      For replay requests ..... The replay request contains an
      elevation index.  This is based on the current elevation.
      If the previous volume scan needs to be searched for the 
      availability of data, then the elevation index may not be
      appropriate anymore .... the previous volume may be a
      different VCP or SAILS/AVSET may be active and change the
      location of the position of an angle within the VCP.

   Inputs:
      requests - Product request data
      rdacnt - RDA VCP information

   Returns:
      The elevation index of the closest angle matching request
      or -1 on error.

*******************************************************************/
static int Replay_find_closest_elev_ind( Prod_request *requests, 
                                         RDA_vcp_info_t *rdacnt ){

   int rpg_elev_ind, vcp_elev, param_ind, i;
   int mind = -1, min = 0, diff, elev;
   float vcp_elev_f = 0.0;

   VCP_ICD_msg_t *rdavcp = (VCP_ICD_msg_t *) &rdacnt->rdcvcpta[0];

   /* Get the elevation angle from the request. */
   param_ind = ORPGPAT_elevation_based( requests->pid );
   if( param_ind < 0 )
      return -1;

   /* The elevation parameter is in deg*10. */
   elev = *(&requests->param_1 + param_ind);

   /* Do For All cuts in the VCP .... */
   for( i = 0; i < rdavcp->vcp_elev_data.number_cuts; i++ ){

      vcp_elev_f = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, 
                           (int) (rdavcp->vcp_elev_data.data[i].angle) );
      vcp_elev = (int) RPGC_NINTD( vcp_elev_f*10.0 );

      diff = elev - vcp_elev;
#ifdef REPLAY_DEBUG
      LE_send_msg( GL_INFO, "Requested Elev: %d, Elev From VCP:  %d, diff: %d\n", 
                   elev, vcp_elev, diff );
#endif
      if( diff < 0 ) diff = -diff;

      if( (mind == -1) || (diff < min) ){

         min = diff;
         mind = i;

      }

    }

    /* Return the elevation index. */
    if( mind >= 0 ){

        /* Convert the RDA index to RPG index. */
        rpg_elev_ind = rdacnt->rdccon[mind]; 
#ifdef REPLAY_DEBUG
        LE_send_msg( GL_INFO, "Replay_find_closest_elev_ind Returns: %d\n",
                     rpg_elev_ind );
#endif
        return rpg_elev_ind;

    }

    /* Return error. */
    else
        return -1;

}

