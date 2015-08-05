/********************************************************************

	This module contains the functions that support 
        abort processing in the orpg.	

********************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/10/16 17:26:28 $
 * $Id: rpg_abort_processing.c,v 1.41 2012/10/16 17:26:28 steves Exp $
 * $Revision: 1.41 $
 * $State: Exp $
 */

#include <rpg.h>
#include <prod_status.h>

#define MAXN_PROD_LIST   20     /* default size of product list. */


static int Initialized = FALSE; /* initialization flag */

static Out_data_type *Out_list; /* list of output products */

static int N_outs = 0;		/* number of output products */

static Prod_header *Hd_info;	/* for storing the product header info */

static int N_prod_list = 0;     /* number of products in product list. */

static int Previous_N_prod_list = MAXN_PROD_LIST;
                                /* current maximum size of product list. */

static Prod_header *Prod_list = NULL;
                                /* pointer to product list. */

static int Aborted = FALSE;     /* The aborted state (TRUE/FALSE) */
    
static int Alg_control_flg = PBD_ABORT_NO;    
                                /* The abort flag from basedata header.
                                   Initialized to NO ABORT */

static int Abort_reason_code = 0;
				/* Holds the reason a task aborted. */

static unsigned int Aborted_vol_seq_number = 0;
				/* Volume scan sequence number of the aborted
				   volume scan. */

static int Input_stream = PGM_REALTIME_STREAM;
				/* The task's input data stream. */

static int Message_mode = 0;	/* Flag, if set, indicates process is
				  in additional message mode. */

/* local functions */
static int Check_prod_list( Prod_header *phd, int olind, int status );
static void Initialize();
static void Set_prod_dep_params( Prod_header *phd, short *user_array );
static int Write_prod_gen_msg( Prod_gen_msg *phd );
int AP_abort_datatype( Prod_header *phd, int olind, int reason );
static int AP_check_prod_status( int elev_index, int prod_id, int p_code, 
                                 int *index, int *status, short *uarray );

/* Public Functions. */

/***************************************************************************

   Description: 
       This function handles abort processing on behalf of the aborting task.  
       For a (non-radial and non-demand type) output specified by datatype, 
       the output is forwarded to downstream consumers.  The outputted product 
       consists solely of a orpg product header. The output product header has 
       the "len" field set to the abort reason.

       A product generation message is also generated indicating the product 
       wasn't generated owing to an abort condition.

   Inputs:      
       pr - pointer to the product request message.
       reason - reason why the process was aborted.

   Return:      
       The return value is currently unused.

   Notes:
       This is the public interface for AP_abort_request().

***************************************************************************/
int RPG_abort_request( void *request, int *reason ){

   static Prod_request prod_req;

   int pid, abort_reason;

   /* First initialize the product request data ... set to 0's */
   memset( (void *) &prod_req, 0, sizeof( Prod_request ) );

   /* Transfer the request data from legacy request to Prod_request structure. */
   {
      short *req = (short *) request;

      pid = ORPGPAT_get_prod_id_from_code ( req[PREQ_PID] ); 
      if( pid <= 0 ){

         LE_send_msg( GL_ERROR, "Unable to Abort Request .... Bad Product Code (%d)\n",
                      req[PREQ_PID] );
         return(-1);

      }

      prod_req.pid = pid;
      prod_req.param_1 = req[PREQ_WIN_AZI]; 
      prod_req.param_2 = req[PREQ_WIN_RANGE];
      prod_req.param_3 = req[PREQ_ELAZ];
      prod_req.param_4 = req[PREQ_STORM_SPEED];
      prod_req.param_5 = req[PREQ_STORM_DIR];
      prod_req.param_6 = req[PREQ_SPARE];
      prod_req.elev_ind = req[PREQ_ELEV_IND];

   }
   
   /* Set the appropriate abort reason. */
   switch( *reason ){
   
      /* Check reasons returned from RPGC_get_outbuf() as well as
         the standard "PROD_....." reasons. */
      case NO_MEM:
      case PROD_MEM_SHED:
      case PGM_MEM_LOADSHED:
         abort_reason = PGM_MEM_LOADSHED;
         break;
   
      case PROD_DISABLED_MOMENT:
      case PGM_DISABLED_MOMENT:
         abort_reason = PGM_DISABLED_MOMENT;
         break;
  
      case PGM_INVALID_REQUEST:
         abort_reason = PGM_INVALID_REQUEST; 
         break;

      case TERMINATE:
      case NO_DATA:
      default:
         abort_reason = AP_get_abort_reason();
         if( abort_reason == 0 )
            abort_reason = PGM_SCAN_ABORT;
         break;
 
   /* End of "switch" statement. */
   }
  
   return( AP_abort_request( &prod_req, abort_reason ) );

/* End of RPG_abort_request() */
}

/****************************************************************************

   Description:
      Writes a request response for a replay product request.  Currently
      only supports the CFCPROD.

   Inputs:
      pid - product ID.
      reason - reason the product can not be generated.
      dep_params - list of product dependent parameters.

   Outputs:

   Returns:
      Always returns 0.

   Notes:
      This module is specifically geared toward the legacy CFCPROD.  It is
      not intended to be used for normal replay product request responses.
      Only should be used for product generators which must directly 
      communicate with ps_onetime.

****************************************************************************/
int RPG_product_replay_request_response( fint pid, fint reason, 
                                         fint2 *dep_params ){

   Prod_header phd;
   int ret, i;

   /* Check for valid product ID. */
   if( pid != CFCPROD )
      PS_task_abort( "Illegal Call To RPG_product_replay_request_response\n" );

   /* Fill in product generation message information. */
   phd.g.vol_t = 0;
   phd.g.prod_id = pid;
   phd.g.gen_t = time(NULL);
   phd.g.len = reason;
   phd.g.input_stream = Input_stream;
   phd.g.id = 0;

   phd.orig_size = sizeof(Prod_header);
   phd.compr_method = COMPRESSION_NONE;

   /* Set the product dependent parameters in the product header. */
   for( i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++ )
      phd.g.req_params[i] = dep_params[i];

   /* Write product generation message */
   ret = ORPGDA_write( ORPGDAT_REPLAY_RESPONSES, (char *) &phd,
                       sizeof(Prod_gen_msg), LB_ANY );

   if( ret != sizeof(Prod_gen_msg) )
      PS_task_abort( "Replay Response Write Failed (%d) For Product %d, Reason %d\n", 
                     ret, pid, reason );

   EN_post(ORPGEVT_REPLAY_RESPONSES,0,0,0);

   return (0);

/* End of RPG_product_replay_request_response( ) */
}

/***********************************************************************

   Description:
      All acquired input buffers and output buffers are released, then
      processing is aborted with reason "reason".

   Inputs:
      reason - contains the abort reason

   Outputs:

   Returns:
      Currently always returns 0.

   Notes:

***********************************************************************/
int RPG_cleanup_and_abort( int *reason ){

   int abort_reason = *reason;
   int disposition = DESTROY;

   /* Release all acquired input buffers. */
   IB_rel_all_inbufs();

   /* Release all acquired output buffers with DESTROY disposition. */
   OB_rel_all_outbufs( &disposition );

   /* Get (set) the aborting reason. */
   if( (Abort_reason_code != 0) 
                   &&
       ((Abort_reason_code > PGM_MAX_GEN_FAILURE) 
                   || 
        (Abort_reason_code < PGM_MIN_GEN_FAILURE)) ){

      switch( *reason ){

         case PROD_MEM_SHED:
            abort_reason = PGM_MEM_LOADSHED;
            break;

         case PROD_DISABLED_MOMENT:
            abort_reason = PGM_DISABLED_MOMENT;
            break;

         case TERMINATE:
         case NO_DATA:
         default:
            abort_reason= PGM_SCAN_ABORT;
            break;

      /* End of "switch" */
      } 

   }
   else
      abort_reason = Abort_reason_code;
         
   /* Abort task processing. */
   RPG_abort_processing( &abort_reason );

   return(0);

/* End of RPG_cleanup_and_abort() */
}

/***********************************************************************

    Description: 
        This function is called by an aborting function to terminate the 
        current processing.  

    Input: 
        reason - abort reason code.

    Return:	
        Return value is never used.

***********************************************************************/
int RPG_abort_processing ( fint *reason ){

   /* If abort flag already set, no need to abort a second time. */
   if( Aborted == TRUE ){

      LE_send_msg( GL_INFO, "RPG_abort_processing:  Aborted == TRUE\n" );
      return (0);

   }

   /* If task waits for any data, check aborted volume scan against current
      volume scan.  If current volume scan not the same as aborted volume
      scan, return.  We do this because the current volume scan sequence 
      number returned from PS_get_current_vol_num is updated only when a
      new input is received.  We assume if an input from the previous volume
      scan is received after an input from the current volume scan and the 
      aborted flag from the old volume scan is set, the process has likely
      already aborted.  The "Aborted_vol_seq_number is updated only on an
      abort condition. */
   if( !WA_wait_driving() ){

      int vol_scan_num;
      unsigned int vol_seq_num = PS_get_current_vol_num( &vol_scan_num );

      if( vol_seq_num != Aborted_vol_seq_number )
         return 0;

   }

   /* Validate the reason for failure.  If invalid reason, terminate. */
   if( (*reason > PGM_MAX_GEN_FAILURE) || (*reason < PGM_MIN_GEN_FAILURE) )
      PS_task_abort( "RPG_abort_processing With Bad Reason (%d)\n", *reason );

   /* Set the task resume time ... NEW_VOLUME, NEW_ELEVATION, or NEW_DATA.
      Set the Aborted flag. */
   WA_set_resume_time ();
   AP_abort_flag(TRUE);

   /* Produce outputs and product generation messages for aborted outputs. 

      NOTE:  For aborting the remaining volume scan, nothing needs to 
             be done.  The resume time will determine when this task
             should be reactivated. */
   if( *reason != PGM_ABORT_REMAIN_SCAN )
      AP_abort_outputs( *reason );

   return (0);

/* End of RPG_abort_processing */
}

/***********************************************************************

    Description: 
        This function is called by an aborting function to info the 
        orpg that "datatype" is not being generated.  

    Input:     
        datatype - Product ID of data to abort
        reason - Abort reason code.

    Return:	
        Return value is never used.

***********************************************************************/
int RPG_abort_datatype_processing ( fint *datatype, fint *reason ){

   /* Set the task resume time ... NEW_VOLUME, NEW_ELEVATION, or NEW_DATA. 
      Set the Aborted flag. */
   WA_set_resume_time ();
   AP_abort_flag(TRUE);

   /* Produce output and product generation message for aborted output. */
   AP_abort_single_output( *datatype, *reason );

   return (0);

/* End of RPG_abort_datatype_processing */
}

/**************************************************************************

   Description:
       This module commits HARI KIRI.

   Returns:
       There is no return value defined for this function.

**************************************************************************/
void RPG_hari_kiri(){

   PS_task_abort( "Committing HARI KIRI\n" ); 

}

/**************************************************************************

   Description:
       Algorithms that want to abort call this function. This function
       has the same functionality as hari_kiri, but the function name
       better describes what it does.

   Returns:
       There is no return value defined for this function.

**************************************************************************/
void RPG_abort_task(){

   PS_task_abort( "Committing HARI KIRI\n" );

}

/**************************************************************************

   Description:
       This module returns the abort reason code. 

   Inputs:
       reason - pointer to int which will receive the abort reason code.

   Outputs:
       reason - pointer to int which receives the abort reason code.

   Returns:
       There is not return value for this function.

**************************************************************************/
int RPG_get_abort_reason( fint *reason ){

   *reason = AP_get_abort_reason();

   return (0);
}

/* Private Functions. */

/***************************************************************************

   Description:
       Initializes the output data type table and number of outputs,
       and Hd_info.

   Return:
       There is no return value(s) defined for this function.

***************************************************************************/
static void Initialize(){

   /* Set prointer to the output data type table and number of outputs, 
      N_outs. */
   N_outs = OB_out_list( &Out_list );

   /* Set pointer to the product header data. */
   Hd_info = OB_hd_info();

   /* Set the task's input stream. */
   Input_stream = INIT_get_task_input_stream();

   /* Set the initialization flag to TRUE. */
   Initialized = TRUE;

}

/***************************************************************************

   Description: 
       This function handles abort processing on behalf of the aborting task.
       For each (non-radial and non-demand type) output registered, an output 
       is forwarded to downstream consumers.  The outputted products consist 
       solely of a orpg product header. The output product header has the 
       "len" field set to the abort reason.

       A product generation message is also generated indicating the product 
       wasn't generated owing to an abort condition.

   Inputs:
       reason - reason why the process was aborted.

   Return:      
       The return value is currently unused.

***************************************************************************/
int AP_abort_outputs( int reason ){

   int olind, rad_status, resume_time;
   Prod_header phd;

   static unsigned int old_seq_num = 0;

   /* Validate the reason for failure.  If invalid reason, terminate. */
   if( reason > PGM_MAX_GEN_FAILURE || reason < PGM_MIN_GEN_FAILURE )
      PS_task_abort( "Aborting Outputs With Bad Reason (%d)\n", reason );

   /* Perform "initial" initialization or on configuration change,
      if not already done. */
   if( !Initialized )
      Initialize();

   /* Get task resume time. */
   resume_time = WA_set_resume_time();

   if( Aborted_vol_seq_number != 0 ){

      LE_send_msg( GL_INFO, "Aborting Task Processing For Vol Seq # %d\n",
                   Aborted_vol_seq_number );

      /* For "resume_time == NEW_VOLUME", do not abort if this volume sequence
         number already aborted. */
      if( (resume_time == NEW_VOLUME) 
                    && 
          (Aborted_vol_seq_number == old_seq_num) ){

         LE_send_msg( GL_INFO, "Volume Seq # %d Already Aborted\n",
                      Aborted_vol_seq_number );
         return (0);

      }

   }

   /* Do for all outputs registered.... */
   for( olind = 0; olind < N_outs; olind++ ){

      /* Ignore RADIAL_DATA, DEMAND_DATA, and time-based outputs. */
      if( (Out_list[olind].timing != RADIAL_DATA)
                           &&
          (Out_list[olind].timing != DEMAND_DATA)
                           &&
          (Out_list[olind].timing != TIME_DATA) ){

         /* Update (write) all registered ITCs. */
         ITC_write_all( Out_list[olind].type );

         /* Fill in orpg product header information.  This is copied from
            Hd_info. */
         if( OB_hd_info_set() ){

            memcpy((char*) &phd, (char *) Hd_info, sizeof(Prod_header) );
            rad_status = WA_radial_status();
            if( rad_status >= 0 ){

               /* If the task resume time is NEW_VOLUME, no more output(s)
                  are produced for this volume scan.  Therefore we set 
                  the bd_status in the product header to "Good END of Volume".
                  This is done to support the data sequencing checks in
                  wait_act.c. */ 
               if( resume_time == NEW_VOLUME )
                  phd.bd_status = GENDVOL;

               else
                  phd.bd_status = rad_status;

            }

         }
         else{

            /* Initialize the data in the product header to all zeros. */
            memset( (char*) &phd, 0, sizeof(Prod_header) );

            /* Set the elevation index to 1. */
            phd.g.elev_ind = 1;
 
            /* For replay input streams and processes which wait for ANY_DATA, the 
               aborting volume scan is "Aborted_vol_seq_number". */
            if( (Input_stream == PGM_REALTIME_STREAM)
                              &&
                (WA_wait_driving()) ){

               /* Get the volume scan sequence number from the volume
                  status. */
               int vol_scan_num;
               phd.g.vol_num = PS_get_current_vol_num( &vol_scan_num );

            }

         }

         /* If the aborted volume scan sequence number is set, use this. */
         if( Aborted_vol_seq_number != 0 )
            phd.g.vol_num = Aborted_vol_seq_number;

         if( (reason != PGM_MEM_LOADSHED)
                      &&
             (reason != PGM_INVALID_REQUEST) ){

            /* Set the elevation index in the product header to 1 so that all
               elevations are processed.  Want to make sure all abort messages
               are generated that need to be generated. */
            phd.g.elev_ind = 1;

         }
            
         /* Initialize fields in the product header. */
         phd.g.prod_id = Out_list[olind].type;
         phd.g.gen_t = time(NULL);
         phd.g.len = reason;
         phd.g.input_stream = Input_stream;

         phd.orig_size = sizeof(Prod_header);
         phd.compr_method = COMPRESSION_NONE;
         
         /* Abort this datatype. */
         AP_abort_datatype( &phd, olind, reason );

      }

   /* End of "for" loop. */
   }

   /* Inform operator of task resumption time. */
   if( reason != PGM_TASK_FAILURE ){

      if( resume_time == NEW_VOLUME )
         LE_send_msg( GL_INFO, "Task Aborted ... Task Resumes At Start Of NEW VOLUME\n" );
      else if( resume_time == NEW_ELEVATION )
         LE_send_msg( GL_INFO, "Task Aborted ... Task Resumes At Start Of NEW ELEVATION\n" );
      else if( resume_time == NEW_DATA )
         LE_send_msg( GL_INFO, "Task Aborted ... Task Resumes At Start Of NEW DATA\n" );

   }
   else
      LE_send_msg( GL_INFO, "Task Failure.  Processing Suspended!!!\n" );

   /* Set the old_seq_num to the current volume sequence number. */
   old_seq_num = Aborted_vol_seq_number;
   if( (Aborted_vol_seq_number == 0 )
                 &&
       (reason == PGM_SCAN_ABORT) )
      old_seq_num = phd.g.vol_num;

   return (0);

/* End of AP_abort_outputs */
}

/***************************************************************************

   Description: 
       This function handles abort processing on behalf of the aborting task.  
       For a (non-radial and non-demand type) output specified by datatype, 
       the output is forwarded to downstream consumers.  The outputted product 
       consists solely of a orpg product header. The output product header has 
       the "len" field set to the abort reason.

       A product generation message is also generated indicating the product 
       wasn't generated owing to an abort condition.

   Inputs:      
       datatype - the output data type to be aborted
       reason - reason why the process was aborted.

   Return:      
       The return value is currently unused.

***************************************************************************/
int AP_abort_single_output( int datatype, int reason ){

   int olind, rad_status;
   Prod_header phd;

   /* Perform initialization if not already performed. */
   if( !Initialized )
      Initialize();

   /* Do for all outputs registered. */
   for( olind = 0; olind < N_outs; olind++ ){

      /* If match on datatype, .... */
      if( Out_list[olind].type == datatype ){

         /* Ignore RADIAL_DATA and DEMAND_DATA based outputs. */
         if( Out_list[olind].timing != RADIAL_DATA &&
             Out_list[olind].timing != DEMAND_DATA ){

            /* Update (write) all registered ITCs. */
            ITC_write_all( datatype );

            /* Fill in orpg product header information. */
            if( OB_hd_info_set() ){

               memcpy((char*) &phd, (char *) Hd_info, sizeof(Prod_header) );
               rad_status = WA_radial_status();
               if( rad_status >= 0 )
                  phd.bd_status = rad_status;

            }
            else{

               phd.g.vol_t = 0;
               phd.elev_t = 0;

            }

            phd.g.prod_id = datatype;
            phd.g.gen_t = time(NULL);
            phd.g.len = reason;
            phd.g.input_stream = Input_stream;

            phd.orig_size = sizeof(Prod_header);
            phd.compr_method = COMPRESSION_NONE;
         
            if( Out_list[olind].timing == ELEVATION_DATA )
                Out_list[olind].gen_cnt++;

            phd.elev_cnt = Out_list[olind].gen_cnt;
            if (Out_list [olind].timing == ELEVATION_DATA)
               Out_list [olind].elev_cnt |= (1 << (phd.g.elev_ind-1));
            else
               Out_list [olind].elev_cnt = (unsigned long) -1;      

            /* Abort this datatype */
            AP_abort_datatype( &phd, olind, reason );

         }

         /* Product of datatype type processed .... break out of loop. */
         break;

      }

   /* End of "for" loop */
   }

   return (0);

/* End of AP_abort_single_output */
}

/***************************************************************************

   Description: 
       This function handles abort processing on behalf of the aborting task.  
       For a (non-radial and non-demand type) output specified by datatype, 
       the output is forwarded to downstream consumers.  The outputted product 
       consists solely of a orpg product header. The output product header has 
       the "len" field set to the abort reason.

       A product generation message is also generated indicating the product 
       wasn't generated owing to an abort condition.

   Inputs:      
       pr - pointer to the product request message.
       reason - reason why the process was aborted.

   Return:      
       The return value is currently unused.

***************************************************************************/
int AP_abort_request( Prod_request *pr, int reason ){

   int olind, rad_status, nlist, ret;
   Prod_header phd;
   LB_info list;

   /* Perform initialization if not already performed. */
   if( !Initialized )
      Initialize();

   /* Do for all outputs registered. */
   for( olind = 0; olind < N_outs; olind++ ){

      /* If match on datatype, .... */
      if( Out_list[olind].type == pr->pid ){

         /* Ignore RADIAL_DATA and DEMAND_DATA based outputs. */
         if( (Out_list[olind].timing != RADIAL_DATA)
                                &&
             (Out_list[olind].timing != DEMAND_DATA) ){

            /* Fill in orpg product header information. */
            if( OB_hd_info_set() ){

               memcpy((char*) &phd, (char *) Hd_info, sizeof(Prod_header) );
               rad_status = WA_radial_status();
               if( rad_status >= 0 )
                  phd.bd_status = rad_status;

            }
            else
               phd.g.vol_t = 0;

            phd.g.prod_id = pr->pid;
            phd.g.gen_t = time(NULL);
            phd.g.len = reason;
            phd.g.input_stream = Input_stream;

            phd.orig_size = sizeof(Prod_header);
            phd.compr_method = COMPRESSION_NONE;
         
            phd.elev_cnt = Out_list[olind].gen_cnt;
            if (Out_list [olind].timing == ELEVATION_DATA)
               Out_list [olind].elev_cnt |= (1 << (phd.g.elev_ind-1));
            else
               Out_list [olind].elev_cnt = (unsigned long) -1;  

            /* Set the product dependent parameters in orpg product header. */
            phd.g.req_params[0] = pr->param_1;
            phd.g.req_params[1] = pr->param_2;
            phd.g.req_params[2] = pr->param_3;
            phd.g.req_params[3] = pr->param_4;
            phd.g.req_params[4] = pr->param_5;
            phd.g.req_params[5] = pr->param_6;

            /* Check if product has already been produced!  This may be
               necessary during split cut processing since there is no
               distinction between elevation index between the two cuts. */
            if( Check_prod_list( &phd, olind, PGS_SCHEDULED ) )
               continue;

            LE_send_msg( GL_INFO,
                         "Abort Request (Reason: %d) For Prod %d (%d %d %d %d %d %d)\n",
                         phd.g.len, pr->pid, pr->param_1, pr->param_2, pr->param_3,
                         pr->param_4, pr->param_5, pr->param_6 );

            /* Write product header to product LB. */
            ret = ORPGDA_write( Out_list[olind].type, (char *) &phd, 
                                sizeof( Prod_header ), LB_ANY );

            if( ret != sizeof( Prod_header ) )
               PS_task_abort( "ORPGDA_write product %s Failed (%d)\n",
                              Out_list[olind].name, ret );

            else{

	       LE_send_msg( GL_INFO, 
                            "ABORT MSG: prod %d (vol# %d wx_mode %d, vcp %d) -> %s (reason %d)\n", 
	                    Out_list[olind].type, phd.g.vol_num, phd.wx_mode, phd.vcp_num,
                            Out_list[olind].name, phd.g.len);
               LE_send_msg( GL_INFO, "--->Request Params: %d  %d  %d  %d  %d  %d\n",
                            phd.g.req_params[0], phd.g.req_params[1],
                            phd.g.req_params[2], phd.g.req_params[3],
                            phd.g.req_params[4], phd.g.req_params[5] );
            }

            /* Need to add product to Prod_list. */
            AP_add_output_to_prod_list( &phd );

            /* Extract the message id of product message just written. */
            nlist = ORPGDA_list( Out_list[olind].type, &list, 1 );
            if( nlist < 0 )
               LE_send_msg( GL_ERROR, 
                        "ORPGDA_list Failed (LB name = %s, Ret = %d)\n",
                        Out_list[olind].name, nlist );

            else
               phd.g.id = list.id;

            /* Write product generation message. */
            Write_prod_gen_msg( &phd.g ); 
              
         }

         /* Product of datatype type processed .... break out of loop. */
         break;

      }

   }

   return (0);

/* End of AP_abort_request() */
}

/***************************************************************************

   Description: 
       This function adds a product header to Prod_list.  The product header 
       is the orpg product header for the product just written to the product 
       LB.  The Prod_list maintains a list of products produced during the 
       current volume scan.

   Inputs:      
       phd - the orpg product header. 

   Return:      
       The return value is currently unused.

***************************************************************************/
void AP_add_output_to_prod_list( Prod_header *phd ){

   Prod_header *temp_list = NULL;

   /* If run out of space in the product list, allocate more memory
      and copy product list to new memory block. */
   if( N_prod_list >= Previous_N_prod_list ){

      temp_list = Prod_list;

      Prod_list = (Prod_header *) malloc( sizeof(Prod_header)*(N_prod_list+1) );
      if( Prod_list == NULL )
         PS_task_abort( "Product List malloc Failed For %d Bytes\n",
                        sizeof(Prod_header)*(N_prod_list+1) );

      memcpy( (void *) Prod_list, (void *) temp_list, 
              sizeof(Prod_header)*N_prod_list );

      /* Free previous product list. */
      free( temp_list );

      /* Update size of Product List. */
      Previous_N_prod_list = N_prod_list + 1;

   }

   /* Append the new header to the product list. */
   memcpy( (void *) (Prod_list + N_prod_list), 
           (void *) phd, sizeof(Prod_header) );

   N_prod_list++;

/* End of AP_add_output_to_prod_list */
}

/***************************************************************************

   Description: 
       This function initializes the number of products listed in Prod_list 
       and Prod_list if memory not allocated.  

   Inputs:
      vol_seq_num - volume scan sequence number.

   Return:
       The return value is currently unused.

***************************************************************************/
void AP_init_prod_list( unsigned int vol_seq_num ){

   if( Message_mode )
      PS_message( "Init List Of Prods Gen'ed For Vol Seq # %d\n", vol_seq_num );

   /* Free Product List space. */
   if( Prod_list != NULL ){

      free( Prod_list );
      Prod_list = NULL;

   }

   /* Allocate space for the product list if not already allocated. */
   if( Prod_list == NULL ){

      Prod_list = (Prod_header *) malloc( sizeof(Prod_header)*MAXN_PROD_LIST );

      if( Prod_list == NULL )
         PS_task_abort( "Product List malloc Failed For %d Bytes\n",
                        sizeof(Prod_header)*MAXN_PROD_LIST );

   }
      
   N_prod_list = 0;
   Previous_N_prod_list = MAXN_PROD_LIST;

/* End of AP_init_prod_list */
}

/***************************************************************************

   Description: 
       This function is called whenever a product abort message is to be 
       written.  If checks whether the abort message product header matches 
       any products already in Prod_list.  If true, the function returns TRUE.  
       Otherwise, the function returns FALSE.

   Inputs:      
       phd - the product header to match entries in Prod_list.
       olind - the output list index for this product.

   Return:      
       Returns TRUE on product match, FALSE otherwise.

***************************************************************************/
static int Check_prod_list( Prod_header *phd, int olind, int status ){

   int match, ind, i;

   /* Search all products in Prod_list. */
   for( i = 0; i <= N_prod_list - 1; i++ ){

      /* Check and match product id. */
      if( phd->g.prod_id == Prod_list[i].g.prod_id ){

         /* Check and match volume sequence number. Match elevation index 
            if product timing is ELEVATION_DATA. */
         if( phd->g.vol_num == Prod_list[i].g.vol_num ){

            /* For products whose timing is ELEVATION_DATA, force match
               on elevation index unless status is PGS_UNKNOWN.  This was
               added to support SAILS where the elevation index values for
               particular elevation angles can change from volume to volume
               for the same VCP.  In this case we force match solely on 
               parameters instead of elevation index and then parameters. */
            if( (Out_list[olind].timing == ELEVATION_DATA)
                                &&
                        (status != PGS_UNKNOWN) ){

               /* Check elevation index match. */
               if( phd->g.elev_ind != Prod_list[i].g.elev_ind )
                  continue;

            }

            /* If here, the volume sequence number and elevation index
               (if ELEVATION_DATA) match.  What is left is to check and
               match product dependent parameters. */
            match = TRUE;
            for( ind = 0; ind < NUM_PROD_DEPENDENT_PARAMS; ind++ ){

               /* For products which have elevation angle as a dependent
                  parameter, we wish to match elevation index instead. */
               if( ind == (int) ORPGPAT_elevation_based( phd->g.prod_id ) )
                  continue; 

               if( phd->g.req_params[ind] != Prod_list[i].g.req_params[ind] ){

                  /* Mismatch causes for loop exit. */
                  match = FALSE;
                  break;

               }

            }

            /* If match found, return TRUE. */
            if( match )
               return( match );

         }

      }

   }

   if( i >= N_prod_list )
      return (FALSE);

   return (TRUE);

/* End of Check_prod_list */
}

/***********************************************************************

    Description: 
        This function is called to return the current value of the
        algorithm control flag (Alg_control_flag) if input argument has
        value of PBD_ABORT_UNKNOWN, or sets the value of the algorithm
        control flag based on the input argument, then returns the 
        current value. 

    Input:     
        alg_control - Algorithm control value.

    Return:	
        Return value of algorithm control flag (Alg_control_flag).

***********************************************************************/
int AP_alg_control( int alg_control ){

   /* If argument is PBD_ABORT_UNKNOWN, return current value of algorithm
      control_flag. */
   if( alg_control == PBD_ABORT_UNKNOWN )
      return( Alg_control_flg );

   /* Set algorithm control flag, and return its value. */
   Alg_control_flg = alg_control;
   return (Alg_control_flg);

/* End of AP_alg_control */
}

/***********************************************************************

    Description: 
        This function is called to return the current value of the
        abort flag (Aborted) if input argument has value of UNKNOWN, 
        or sets the value of the abort flag based on the input argument, 
        then returns the current value.  If the flag is being cleared,
        prior to return the abort reason is initialized. 

    Input:     
        flag_value - Abort flag (either UNKNOWN, TRUE, or FALSE).

    Return:	
        Return value of abort flag (Aborted).

***********************************************************************/
int AP_abort_flag( int flag_value ){

   /* If flag_value has value of UNKNOWN, return current value of abort
      flag. */
   if( flag_value == UNKNOWN )
      return( Aborted );
 
   /* Set Aborted to flag_value and return Aborted. */
   Aborted = flag_value;

   /* If the abort flag is being cleared, initialize the abort reason. */
   if( flag_value == FALSE ){

      if( Message_mode )
         PS_message( "Clearing the Aborted Flag\n" );

      AP_init_abort_reason();
      AP_set_aborted_volume( 0 );

   }

   return ( Aborted );

/* End of AP_abort_flag */
}

/**************************************************************************

   Description:
       This module initializes the abort processing module.

   Returns:
       There is no return value defined for this function.

**************************************************************************/
void AP_initialize(){

   unsigned int vol_seq_num = ORPGVST_get_volume_number();

   /* Initialize Product List. */
   AP_init_prod_list( vol_seq_num );

   /* Initialize this module. */
   Initialize();

   /* Check if process is in additional message mode. */
   Message_mode = PS_in_message_mode();

}

/**************************************************************************

   Description:
       This module sets the abort reason code. 

   Inputs:
       reason - holds the abort reason code.

   Returns:
       There is no return value defined for this function.

**************************************************************************/
void AP_set_abort_reason( int reason ){

   /* Validate the reason. */
   if( reason < PGM_MIN_GEN_FAILURE )
      Abort_reason_code = 0;

   if( reason > PGM_MAX_GEN_FAILURE )
      Abort_reason_code = 0;

   Abort_reason_code = reason;

   if( Abort_reason_code == 0 )
      LE_send_msg( GL_ERROR, "Bad Abort Reason In AP_set_abort_reason (%d)\n",
                   reason );

}

/**************************************************************************

   Description:
       This module returns the abort reason code. 

   Returns:
       Returns the abort reason.

**************************************************************************/
int AP_get_abort_reason(){

   return( Abort_reason_code );

}

/**************************************************************************

   Description:
       This module initializes the abort reason code. 

   Returns:
       There is no return value defined for this function.

**************************************************************************/
void AP_init_abort_reason( ){

   Abort_reason_code = 0;

}

/*************************************************************************
   Description:
      Sets the volume sequence number of the volume scan for which products 
      are aborted.
 
   Inputs:
      vol_num - volume scan sequence number.

   Outputs:

   Returns:

   Notes:

*************************************************************************/
void AP_set_aborted_volume( unsigned int vol_num ){

   Aborted_vol_seq_number = vol_num;

   if( vol_num == 0 ){

      if( Message_mode )
         PS_message( "The Aborted Vol Seq # Has Been Set To %d\n", 
                     Aborted_vol_seq_number );

   }
   else
      LE_send_msg( GL_INFO, "The Aborted Vol Seq # Has Been Set To %d\n", 
                   Aborted_vol_seq_number );

/* End of AP_set_aborted_volume() */
}

/************************************************************************
   Description:
      Transfers the product dependent parameters from the product request 
      into the product header.

   Inputs:
      phd - pointer to product header to receive parameters.
      user_array - pointer to array containing product dependent 
                   parameters.

   Outputs:
      phd - pointer to product header receiving parameters.

   Returns:

   Notes:

************************************************************************/
static void Set_prod_dep_params( Prod_header *phd, short *user_array ){

   phd->g.req_params[0] = user_array[PREQ_WIN_AZI];
   phd->g.req_params[1] = user_array[PREQ_WIN_RANGE];
   phd->g.req_params[2] = user_array[PREQ_ELAZ];
   phd->g.req_params[3] = user_array[PREQ_STORM_SPEED];
   phd->g.req_params[4] = user_array[PREQ_STORM_DIR];
   phd->g.req_params[5] = user_array[PREQ_SPARE];
   
/* End of Set_prod_dep_params() */
}

/************************************************************************

   Description:
      For replay data types, writes product generation message to 
      replay response LB.  For all input data streams, write product
      generation message to product generation message LB.

   Inputs:
      phd - pointer to product generation message.

   Outputs:

   Returns:
      0 on success, or -1 on failure.

   Notes:
      Task may terminate in some cases if write fails.

*************************************************************************/
static int Write_prod_gen_msg( Prod_gen_msg *phd ){

   int ret;

   /* For the replay data stream, write the product generation message
      to the replay response LB. */
   if( Input_stream == PGM_REPLAY_STREAM ){

      ret = ORPGDA_write( ORPGDAT_REPLAY_RESPONSES, (char *) phd,
                          sizeof(Prod_gen_msg), LB_ANY );

      if( ret != sizeof(Prod_gen_msg) )
         PS_task_abort( "Replay Response Write Failed (%d)\n", ret );

      EN_post(ORPGEVT_REPLAY_RESPONSES,0,0,0);

   }

   /* Write the product generation message to the product generation
      message LB. */
   ret = ORPGDA_write( ORPGDAT_PROD_GEN_MSGS, (char *) phd, sizeof(Prod_gen_msg), 
                       LB_ANY );
   if( ret != sizeof( Prod_gen_msg ) ){

      /* If ORPGDA_write fails, terminate task processing.  Only do this 
         if abort reason code is not PGM_TASK_FAILURE to avoid recursion. */
      if( AP_get_abort_reason() != PGM_TASK_FAILURE )
         PS_task_abort( "ORPGDA_write prod_gen_msg Failed (Ret = %d)\n", ret );

      else
         return( -1);

   }

   return 0;

/* End of Write_prod_gen_msg() */
} 


/***************************************************************************

   Description: 
       This function handles abort processing on behalf of the aborting task.
       For each (non-radial and non-demand type) output registered, an output 
       is forwarded to downstream consumers.  The outputted products consist 
       solely of a orpg product header. The output product header has the 
       "len" field set to the abort reason.

       A product generation message is also generated indicating the product 
       wasn't generated owing to an abort condition.

   Inputs:
       phd - pointer to ORPG product header for abort message.
       olind - output list index for the datatype being aborted.
       reason - reason why the process was aborted.

   Return:      
       The return value is currently unused.

***************************************************************************/
int AP_abort_datatype( Prod_header *phd, int olind, int reason ){

   int nlist, ret, ind, index, p_code, status;
   int aborted_vol_scan, task_type, num_elevation_cuts, aborted, done;
   short user_array[10][10];
   LB_info list;

   /* Get the task timing .... */
   task_type = INIT_task_type();

   /* Perform module initialization. */
   num_elevation_cuts = -1;
   aborted = FALSE;
   done = FALSE;

   /* If Aborted_vol_seq_number is not set, set the aborted 
      volume scan to the current volume.  This can happen 
      when the task aborts a data type if it can't get 
      sufficient output buffer space or maybe if the 
      input data moment (or variable) is disabled. */
   if( Aborted_vol_seq_number != 0 )
      aborted_vol_scan = Aborted_vol_seq_number;

   else
      aborted_vol_scan = phd->g.vol_num;

   /* Account for modulus of 0. */
   aborted_vol_scan %= MAX_VSCAN;
   if( aborted_vol_scan == 0 )
      aborted_vol_scan = MAX_VSCAN;

   /* Do until product aborted. */
   while(1){

      /* Get product request information for this elevation scan. */
      index = 1;
      p_code = ORPGPAT_get_code( phd->g.prod_id );
      PRQ_populate_user_array( phd->g.elev_ind, phd->g.prod_id, p_code, 
                               NULL, aborted_vol_scan, &index, user_array[0] );

      /* If no requests found, check the product status for elevation
         index and volume scan. */
      status = PGS_SCHEDULED;
      if( index == 1 )
         AP_check_prod_status( phd->g.elev_ind, phd->g.prod_id, 
                               Out_list[olind].int_or_final,
                               &index, &status, user_array[0] );

      /* Do for all product requests scheduled for this elevation
         scan.  Volume-based task types are scheduled for all elevation
         scans. */
      for( ind = 0; ind < index - 1; ind++ ){
 
         /* Set the product dependent parameters in the orpg product header. */
         Set_prod_dep_params( phd, user_array[ind] );

         /* Check if product has already been produced!  This may be
            necessary during split cut processing since there is no
            distinction between elevation index between the two cuts. */
         if( Check_prod_list( phd, olind, status ) )
            continue;

         /* Update the count of products generated.  Only do this once per
            product. */
         if( ind == 0 && Out_list[olind].timing == ELEVATION_DATA )
            Out_list[olind].gen_cnt++;

         phd->elev_cnt = Out_list[olind].gen_cnt;
         if(Out_list [olind].timing == ELEVATION_DATA)
            Out_list [olind].elev_cnt |= (1 << (phd->g.elev_ind-1));
         else
            Out_list [olind].elev_cnt = (unsigned long) -1;     

         phd->orig_size = sizeof(Prod_header);
         phd->compr_method = COMPRESSION_NONE;

         /* Write product to product LB.  The message ID must be initialized
            to 0 to indicate this product is not found in the product database. */
         phd->g.id = 0;
         ret = ORPGDA_write( Out_list[olind].type, (char *) phd, 
                             sizeof( Prod_header ), LB_ANY );

         /* If write fails, terminate task processing.  Only do this if
            abort reason code is not PGM_TASK_FAILURE to avoid recursion. */
         if( ret != sizeof( Prod_header ) ){

            if( reason != PGM_TASK_FAILURE )
               PS_task_abort( "ORPGDA_write product %s Filed (%d)\n",
                              Out_list[olind].name, ret );
            else
               return (-1);

         }
         else{

            if( Out_list[olind].timing == ELEVATION_DATA ){

	       LE_send_msg( GL_INFO, 
		        "ABORT MSG: prod %d (vol# %d, wx_mode %d vcp %d elev_ind %d) -> %s (reason %d)\n", 
		         Out_list[olind].type, phd->g.vol_num, phd->wx_mode, phd->vcp_num, phd->g.elev_ind, 
                         Out_list[olind].name, phd->g.len);
               LE_send_msg( GL_INFO, "--->Request Params: %d  %d  %d  %d  %d  %d\n",
                            phd->g.req_params[0], phd->g.req_params[1],
                            phd->g.req_params[2], phd->g.req_params[3],
                            phd->g.req_params[4], phd->g.req_params[5] );
            }
            else{

	       LE_send_msg( GL_INFO,
	                "ABORT MSG: prod %d (vol# %d wx_mode %d vcp %d) -> %s (reason %d)\n", 
		         Out_list[olind].type, phd->g.vol_num, phd->wx_mode, phd->vcp_num,
                         Out_list[olind].name, phd->g.len);
               LE_send_msg( GL_INFO, "--->Request Params: %d  %d  %d  %d  %d  %d\n",
                            phd->g.req_params[0], phd->g.req_params[1],
                            phd->g.req_params[2], phd->g.req_params[3],
                            phd->g.req_params[4], phd->g.req_params[5] );

            }

            /* Set flag to indicate task was aborted only for intermediate products.  
               This ensures that elevation products abort even though the task 
               timing is volume-based. */
            if( Out_list[olind].int_or_final == INT_PROD )
               aborted = TRUE;

         }

         /* Need to add product to Prod_list. */
         AP_add_output_to_prod_list( phd );

         /* Extract the message id of product message just written. */
         nlist = ORPGDA_list( Out_list[olind].type, &list, 1 );
         if( nlist < 0 )
            LE_send_msg( GL_ERROR, 
                        "ORPGDA_list Failed (LB name = %s, Ret = %d)\n",
                        Out_list[olind].name, nlist );

         else
            phd->g.id = list.id;

         /* Write product generation message. */
         if( Write_prod_gen_msg( &phd->g ) < 0 )
            return (-1);

         /* If an event is associated with this product, post it now. */
         if( Out_list[olind].event_id != ORPGEVT_NULL_EVENT ){

            ret = EN_post( Out_list[olind].event_id, (void *) NULL, (size_t) 0,
                           (int) 0 );
            if( ret != 0 )
               LE_send_msg( GL_EN(ret), "AP_abort_outputs: EN_post Failed.  Ret = %d\n", ret );

            else{

               if( Message_mode )
                  PS_message( "AP_abort_outputs: Event ID %d Posted.\n",
                              Out_list[olind].event_id ); 

            }

         }

      }

      /* For non TASK_ELEVATION_BASED task types, set done flag only if
         output data timing is not ELEVATION_DATA.  If the task type 
         is TASK_ELEVATION_BASED, set done flag if algorithm control 
         flag (Alg_control_flg) indicates abort for new elevation, or 
         abort is due to either memory loadshed or disabled moment.  
         The algorithm control flag is set in the radial header by
         PBD, whereas the abort reason is set by the task based
         on some condition such as lack of data moment(s) or lack
         of sufficient resources. */
      if( (task_type != TASK_ELEVATION_BASED) 
                     && 
          (task_type != TASK_RADIAL_BASED) ){

         if( (Out_list[olind].timing != ELEVATION_DATA)
                                  ||
             ((Out_list[olind].timing == ELEVATION_DATA) && aborted) )
            done = TRUE;

      }

      else if (task_type == TASK_ELEVATION_BASED){

         if ( (Alg_control_flg == PBD_ABORT_FOR_NEW_EV) 
                               ||
              (Alg_control_flg == PBD_ABORT_FOR_NEW_EE) )
            done = TRUE; 

         else if ( reason == PGM_MEM_LOADSHED ) 
            done = TRUE;

      }
      
      else if (task_type == TASK_RADIAL_BASED){

         if( Out_list[olind].timing == RADIAL_DATA )
            done = TRUE;

      }

      /* For elevation-based outputs, abort products for all 
         elevation cuts not yet processed, if required. */
      if( Out_list[olind].timing == ELEVATION_DATA && !done ){

         phd->elev_cnt++;
         phd->g.elev_ind++;

         /* Set the number of elevation cuts this VCP. */
         if( num_elevation_cuts < 0 ){

            Scan_Summary *scan_sum = NULL;

            /* Initialially set the number of elevations cuts to the
               maximum.  Get the number of cuts in the aborted volume
               scan from the Scan Summary data. */
            num_elevation_cuts = MAX_ELEVATION_CUTS;

            if( phd->vcp_num > 0 ){

               scan_sum = ORPGSUM_get_scan_summary( phd->g.vol_num );

               if( (scan_sum != NULL) && (scan_sum->rpg_elev_cuts > 0) )
                  num_elevation_cuts = (int) scan_sum->rpg_elev_cuts;

            }

         }
         
         if( phd->g.elev_ind <= num_elevation_cuts )
            continue;

      }

      /* Break out of while loop. */
      break;

   }

   return (0);

/* End of AP_abort_datatype */
}

/***************************************************************************

   Description:
       This function checks the product status for product requests.  This
       may be required, for example, when an elevation-based product is
       scheduled in the previous volume scan at an elevation angle that
       is not scheduled in the current volume scan. 

   Inputs:
       elev_index - elevation index for the product request.
       prod_id - product ID of the product request.
       prod_code - product code of the product request.

   Outputs:
       index - returns the number of requests + 1.
       status - product status
       uarray - holds the product request information.

   Return:
       The return value is currently unused.  Always returns 0.

***************************************************************************/
static int AP_check_prod_status( int elev_index, int prod_id, int p_code, 
                                 int *index, int *status, short *uarray ){

   int i, j, len;
   char *buf = NULL;
   Prod_gen_status_header *ps = NULL;
   Prod_gen_status *entry = NULL;

   /* Read product status from LB. */
   len = ORPGDA_read( ORPGDAT_PROD_STATUS, &buf, LB_ALLOC_BUF, PROD_STATUS_MSG );
   if( len <= 0 )
       return (0);
   
   /* Check to make sure product status is valid. */
   ps = (Prod_gen_status_header *) buf;
   if( (len < sizeof (Prod_gen_status_header))
                         ||
       (ps->list < sizeof (Prod_gen_status_header))
                         || 
                   (ps->length < 0)
                         ||
       (len != ps->list + ps->length*sizeof (Prod_gen_status)) )
      return (0);

   /* Find the matching volume sequence number. */
   for( i = 0; i < ps->vdepth; i++ ){

      if( Aborted_vol_seq_number != ps->vnum[i] )
         continue;

      /* Matching volume sequence number. */
      entry = (Prod_gen_status *) (buf + ps->list);
      for( j = 0; j < ps->length; j++ ){

         /* Find matching product ID and elevation index. */
         if( (entry->prod_id == prod_id)
                          &&
             (entry->elev_index == elev_index) ){

            /* If status indicates product status has not been updated,
               fill in request information to user array. */
            if( (entry->msg_ids[i] == PGS_UNKNOWN)
                             ||
                (entry->msg_ids[i] == PGS_SCHEDULED)
                             ||
                (entry->msg_ids[i] == PGS_TIMED_OUT) ){

               /* Update user array with product status information. */
               uarray[PREQ_PID] = p_code;
               uarray[PREQ_WIN_AZI] = entry->params[0];
               uarray[PREQ_WIN_RANGE] = entry->params[1];
               uarray[PREQ_ELAZ] = entry->params[2];
               uarray[PREQ_STORM_SPEED] = entry->params[3];
               uarray[PREQ_STORM_DIR] = entry->params[4];
               uarray[PREQ_SPARE] = entry->params[5];
               uarray[PREQ_ELEV_IND] = elev_index;
               uarray[PREQ_REQ_NUM] = 0;
               uarray[PREQ_RESERVED] = 1;  

               *index = *index + 1;
               *status = entry->msg_ids[i];

            }

            /* No need to check additional product status entries. */
            break;

         }

         /* Go to the next entry in product status. */
         entry++;

      }

      /* No need to check additional volumes. */
      break;
      
   }

   if( buf != NULL )
      free( buf );

   return 0;
    
/* End of AP_check_prod_status() */
}
