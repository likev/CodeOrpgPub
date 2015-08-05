/*******************************************************************

	Main module for VCP translation support.

*******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/12/17 16:14:53 $
 * $Id: translate_main.c,v 1.8 2012/12/17 16:14:53 steves Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>            /* EXIT_FAILURE                            */
#include <orpg.h>
#include <vcp.h>

#include <basedata.h>
#include <misc.h>
#include <a309.h>

#define GLOBAL_DEFINED
#include <translate_main.h>
#include <comm_manager.h>
#include <orpgsite.h>
#include <missing_proto.h>

#define INPUT_DATA_SIZE	  128000	/* Maximum size of RDA message, in bytes */

static int Log_file_nmsgs = 1000;	/* Default size, in number of message, for 
                                           Log Error (LE) file. */
static char *Rda_msg = NULL; 
static char *Rpg_msg = NULL;
static int Suppl_vcp_info_lbfd = 0;
static int Suppl_vcp_info_updated = 0;

/* Local Function Prototypes. */
static int Read_options( int argc, char **argv );
static int Cleanup_fxn( int signal, int status );
static void Open_lb();
static void TRNS_abort (char *format, ... );
static void Check_request( char *rpg_msg );
static void Check_response( char *rda_msg );
void Lb_notify_handler( int fd, LB_id_t msgid, int msg_info, void *arg );

/*******************************************************************

   Description:   
      Main module for the VCP translation process.

      Normally the wideband comm manager communicates directly with
      pbd and control_rda.   This translation process sits between
      the comm manager and pbd/control_rda.  Alternate request/response
      LBs need to be specified.  These are used to communicate between
      this process and the comm manager.  The standard request/response
      LBs are used to communicate between this process and pbd/control_rda.

      Currently the following messages need to be processed:
         1) Radial Message (type 31)
         2) RDA Status Message (type 2)
         3) RDA/RPG VCP Message (type 5)
         4) RDA Control Command (type 6)
         5) RPG/RDA VCP Message (type 7)

      For Message Type 31, the VCP number, the radial status and the 
      elevation index are changed. 

      For Message Type 2, the VCP number is changed.

      For Message Type 5 and 7, the VCP data is translated according
      to the translation table.

      For Message Type 6, in the event of an elevation restart, the
      elevation cut number needs to be determined for VCPs that are
      translated.

   Input:
      argc - number of command line arguments.
      argv - command line arguments.

   Output:

   Returns:
      There is no return value defined for this function.  
      Always returns 0.
 
********************************************************************/
int main( int argc, char *argv[] ){

    /* Read command line options.  Task exits on failure. */
    if (Read_options (argc, argv) != 0)
	ORPGTASK_exit (GL_EXIT_FAILURE);

    /* Initialize Log-Error services. */
    if( ORPGMISC_init( argc,           /* Number command line arguments. */
                       argv,           /* Command Line arguments. */
                       Log_file_nmsgs, /* Log Error file size. */
                       0,              /* Use default LB types. */
                       -1,             /* No instance */
                       0               /* Allow system status log messages. */) < 0 ){

       LE_send_msg( GL_INFO, "ORPGMISC_init Failed\n" );
       ORPGTASK_exit(GL_EXIT_FAILURE);

    }

    /* Register termination handler. */
    ORPGTASK_reg_term_handler( Cleanup_fxn );

    /* Initialize the RDA data read buffer.  Exit on malloc failure. */
    Rda_msg = (char *) malloc( INPUT_DATA_SIZE );
    if( Rda_msg == NULL ){

       LE_send_msg( GL_INFO, "malloc Failed For %d Bytes\n", INPUT_DATA_SIZE );
       ORPGTASK_exit(GL_EXIT_FAILURE);

    }

    /* Initialize the RPG data read buffer.  Exit on malloc failure. */
    Rpg_msg = (char *) malloc( INPUT_DATA_SIZE );
    if( Rpg_msg == NULL ){

       LE_send_msg( GL_INFO, "malloc Failed For %d Bytes\n", INPUT_DATA_SIZE );
       ORPGTASK_exit(GL_EXIT_FAILURE);

    }

    /* Open all LB's */
    Open_lb();

    /* Read the VCP translation table. */
    if( Read_translation_table( ) < 0 )
       ORPGTASK_exit(GL_EXIT_FAILURE);

    /* Tell RPG manager "translate" is ready. */
    ORPGMGR_report_ready_for_operation();

    /* Wait for operational mode before continuing with initialization. */
    if( ORPGMGR_wait_for_op_state( (time_t) 120 ) < 0 )
       LE_send_msg( GL_ERROR, "Waiting For RPG Operational State TIMED-OUT\n" );

    else
       LE_send_msg( GL_INFO, "The RPG is Operational\n" );

    /* Do Forever .... */
    while (1) {

        /* Check if the Supplemental VCP Information has been
           updated. Note:   This is not expected operationally.
           However having this simplifies testing. */
        if( Suppl_vcp_info_updated ){

           Suppl_vcp_info_updated = 0;
           Read_translation_table();

        }

        /* Check communication manager request LB. */
        Check_request( Rpg_msg );

        /* Check communication manager response LB */
	Check_response( Rda_msg );

        /* Sleep a short period of time and do it all over
           again. */
        msleep(500);

    } /* End of "while(1)" loop. */
  
} /* End of main() */

/***********************************************************************

       Description:  
          This function reads command line arguments.

       Input:        
          argc - Number of command line arguments.
          argv - Command line arguments.

       Output:       
          Usage message

       Returns:      
          0 on success or -1 on failure

*********************************************************************/
static int Read_options (int argc, char **argv){

    extern char *optarg;    
    extern int optind;
    int c, err;       

    err = 0;
    TRNS_verbose = 0;

    while ((c = getopt (argc, argv, "hl:v")) != EOF) {
	switch (c) {

            /* Change the size of the task log file. */
            case 'l':
                Log_file_nmsgs = atoi( optarg );
                if( Log_file_nmsgs < 0 || Log_file_nmsgs > 5000 )
                   Log_file_nmsgs = 500;
                break;

            case 'v':
               TRNS_verbose = 1;
               break;

            /* Print out the usage information. */
	    case 'h':
	    case '?':
		err = 1;
		break;
	}
    }

    if( err == 1 ){

	printf ("Usage: %s [options]\n", argv[0]);
	printf ("       Options:\n");
	printf ("       -l Log File Number LE Messages (%d)\n", Log_file_nmsgs);
        printf ("       -v Verbose Mode\n" ); 
        printf ("       -h Usage Information\n" ); 
	ORPGTASK_exit (GL_EXIT_FAILURE);

    }

    return (0);

} /* End of Read_options() */

#define MAX_MSG_SIZE	256
/********************************************************************

    Description:  
       This function logs an error message and terminates the process.

    Input:
       format - message format string;
          ... - list of variables whose values are to be printed.

********************************************************************/
void TRNS_abort( char *format, ... ){

    char buf [MAX_MSG_SIZE];
    va_list args;

    /* If non-null format string, then .... */
    if( (format != NULL) && (*format != '\0') ){

	va_start (args, format);
	vsprintf (buf, format, args);
	va_end (args);

    }
    else
	buf [0] = '\0';

    /* Log error message. */
    LE_send_msg( GL_ERROR, buf );
    LE_send_msg( GL_ERROR, "terminates\n" );

    /* Terminate this process. */
    ORPGTASK_exit( GL_EXIT_FAILURE );

} /* End of TRNS_abort() */

/**************************************************************************************

   Description:
      Termination handler for translate.  A return value of 0 cause task termination.

*************************************************************************************/
static int Cleanup_fxn( int signal, int status ){

   /* Don't have anything to do.  Report signal received and return. */
   LE_send_msg( GL_INFO, "Signal %d Received\n", signal );
   return (0);

} /* End of Cleanup_fxn() */

/*************************************************************************************

   Description:
      Open all LB's used by this process.

**************************************************************************************/
static void Open_lb(){

   int retval = 0;
   char task_name[ORPG_TASKNAME_SIZ];
   Orpgtat_entry_t *task_entry = NULL;
   int ret_rsi = 0, ret_rso = 0, ret_rqi = 0, ret_rqo = 0;

  /* Get my task name .... this will be used to access the task table entry. */
   if( (ORPGTAT_get_my_task_name( (char *) &task_name[0], ORPG_TASKNAME_SIZ ) >= 0)
                                   &&
       ((task_entry = ORPGTAT_get_entry( (char *) &task_name[0] )) != NULL) ){

      /* Check for match on Response Input. */
      if( (TRNS_response_in_lb = ORPGTAT_get_data_id_from_name( task_entry, "RESPONSE_IN" )) >= 0)
         LE_send_msg( GL_INFO,
                "Translation Response Input LB (%d):\n", TRNS_response_in_lb );

      /* Check for match on Response Output. */
      if( (TRNS_response_out_lb = ORPGTAT_get_data_id_from_name( task_entry, "RESPONSE_OUT" )) >= 0)
         LE_send_msg( GL_INFO,
                "Translation Response Output LB (%d):\n", TRNS_response_out_lb );

      /* Check for match on Request Input. */
      if( (TRNS_request_in_lb = ORPGTAT_get_data_id_from_name( task_entry, "REQUEST_IN" )) >= 0)
         LE_send_msg( GL_INFO,
                "Translation Request Input LB (%d):\n", TRNS_request_in_lb );

      /* Check for match on Request Output. */
      if( (TRNS_request_out_lb = ORPGTAT_get_data_id_from_name( task_entry, "REQUEST_OUT" )) >= 0)
         LE_send_msg( GL_INFO,
                "Translation Request Output LB (%d):\n", TRNS_request_out_lb );

   }

   LE_send_msg( GL_INFO, "Response In: %d, Response Out: %d\n",
                TRNS_response_in_lb, TRNS_response_out_lb );

   LE_send_msg( GL_INFO, "Request In: %d, Request Out: %d\n",
                TRNS_request_in_lb, TRNS_request_out_lb );

   /* Open LB's for response messages from comm manager to downstream applications. */
   if( ((ret_rsi = ORPGDA_open( TRNS_response_in_lb, LB_READ )) < 0 )
                                    ||
       ((ret_rso = ORPGDA_open( TRNS_response_out_lb, LB_WRITE )) < 0) 
                                    ||
       ((ret_rqi = ORPGDA_open( TRNS_request_in_lb, LB_READ )) < 0) 
                                    ||
       ((ret_rqo = ORPGDA_open( TRNS_request_out_lb, LB_WRITE )) < 0) ){

      LE_send_msg( GL_ERROR, "Open Error on Request/Response LBs: (%d, %d, %d, %d\n",
                   ret_rsi, ret_rso, ret_rqi, ret_rqo );

      ORPGTASK_exit( GL_EXIT_FAILURE );

   }

   /* Open the ORPGDAT_SUPPL_VCP_INFO LB for LB_WRITE. */
   retval = ORPGDA_open( ORPGDAT_SUPPL_VCP_INFO, LB_WRITE );
   if( retval < 0 ){

      LE_send_msg( GL_ERROR, "ORPGDA_open for ORPGDAT_SUPPL_VCP_INFO Failed: %d\n",
                   retval );
      ORPGTASK_exit( GL_EXIT_FAILURE );

   }

   /* Register for updates to the VCP Translation Table.  Normally this LB message
      is written once by a maintenance task.   Added LB notification mainly for testing
      on-the-fly changes. */
    if( (retval = ORPGDA_UN_register( ORPGDAT_SUPPL_VCP_INFO, TRANS_TBL_MSG_ID,
                                      Lb_notify_handler )) < 0 ){

      LE_send_msg( GL_ERROR,
                   "ORPGDAT_SUPPLE_VCP_INFO LB Notification Failed (%d)\n", retval );
      ORPGTASK_exit( GL_EXIT_FAILURE );

   }

   /* Get the fd from the LB. */
   Suppl_vcp_info_lbfd = ORPGDA_lbfd( ORPGDAT_SUPPL_VCP_INFO );

   /* Free the space allocated to task_entry. */
   if( task_entry != NULL )
      free( task_entry );

} /* End of Open_lb() */

/*************************************************************************************

   Description:
      Reads the Comm Manager Response LB, checks if the response has data and the 
      data is a message that needs to be processed, and process it.

      The processed message is written to the Response LB specified in the System
      Configuration File for the wideband link.

   Inputs:
      rda_msg - buffer to receive the RDA/Comm Manager message.

**************************************************************************************/
void Check_response( char *rda_msg ){

   CM_resp_struct *resp;
   char *radar_data;
   int retval = 0, ret1 = 0, ret2 = 0, offset = 0;
   int read_returned, msg_type, has_comm_header;

   /* This flag assumes that VCP data is provided to the RPG prior to the start 
      of every volume scan. */
   static int has_supple_cuts = 0;

   /* Do While messages to be read .... */
   while (1) {

      /* Do Until a messsage of interest is read ....... */
      while(1){

         /* Check communication manager response LB */
         read_returned = ORPGDA_read ( TRNS_response_in_lb, rda_msg, INPUT_DATA_SIZE, 
                                       LB_NEXT );
         radar_data = rda_msg;
         has_comm_header = 0;
         msg_type = 0;

         /* The message read must be at least as large of the Comm
            manager response structure. */
         if( read_returned >= (int) CM_RESP_HEADER_SIZE ){
        
            short msg_len;
            int first_word;
            RDA_RPG_message_header_t *msg_header = NULL;

            /* We need to determine if message contains a Comm Manager
               header ... data read from Archive II tape does not
               contain a Comm Manager header. */
            first_word = *((int *) radar_data);
            msg_len = (first_word >> 16) & 0xffff;

            /* If the message length is zero, this assumes the radar data is
               prefixed with a CM_resp_struct header. */
            if( msg_len == 0 ){

               /* Only care about data messages. */
               resp = (CM_resp_struct *) rda_msg; 
               if( resp->type != RQ_DATA ){

                  /* Write the message to output LB. */
                  if( (retval = ORPGDA_write( TRNS_response_out_lb, rda_msg, 
                                              read_returned, LB_ANY )) < 0 )
	             TRNS_abort( "ORPGDA_write( TRNS_response_out_lb: %d ) failed (%d)\n", 
                                 TRNS_response_out_lb, retval );

                  continue;

               }

               /* Strip off the CM_resp_struct header and the CTM header. */
               radar_data += RESP_OFFSET;

               /* Set flag indicating it has a CM header. */
               has_comm_header = 1;

            } 
            else{

               /* Clear flag indicating it doesn't have a CM header. */
               has_comm_header = 0;

            }

            /* Do ICD to local host conversion of the message header.   This 
               does conversion from Big Endian to Little Endian if host machine 
               is Little Endian.  The RDA data is assumed Big Endian format.  
               Also floating point number conversion (Concurrent to IEEE 754) 
               is done. */
            UMC_RDAtoRPG_message_header_convert( radar_data );

            /* Get message type. */
            msg_header = (RDA_RPG_message_header_t *) radar_data;
            msg_type = (int) (msg_header->type);

            /* If RDA basedata, status message or RDA/RPG VCP data, convert the 
               message data from external format to internal format (i.e., if the 
               RDA is Big-Endian and the RPG is Little-Endian (or vice versa) based
               on the message type. */
            if( (msg_type == GENERIC_DIGITAL_RADAR_DATA) 
                          || 
                (msg_type == RDA_STATUS_DATA) 
                          ||
                (msg_type == RDA_RPG_VCP) ){

               UMC_RDAtoRPG_message_convert_to_internal( msg_type, radar_data );
               break;

            }
            else{

               /* Convert the message header back to external format. */
               UMC_RDAtoRPG_message_header_convert( radar_data );
                    
               /* Write the message so it can be processed by downstream consumers. */
               retval = ORPGDA_write( TRNS_response_out_lb, rda_msg, read_returned, LB_ANY );
	       if( retval < 0 )
	          TRNS_abort( "ORPGDA_write( TRNS_response_out_lb: %d ) failed (%d)\n", 
                              TRNS_response_out_lb, retval );

            }

         }
         else{

            /* Set the message type to 0 if it is a message type we do not care
               anything about. */
            msg_type = 0;

         }

         /* Process various LB_read errors. */

         /* If data not currently available, wait a short period then try
               reading again.  Currently we wait 1 second. */
	 if( read_returned == LB_TO_COME )
	    return;

         /* If the next message in LB has expired, LB is either sized too small
            or something is delaying processing.  In either case, seek to the 
            first unread message. */
	 else if( read_returned == LB_EXPIRED )
            ORPGDA_seek ( TRNS_response_in_lb, 0, LB_FIRST, NULL ); 

         /* All other read failures are (at this point in time) fatal.  In 
            the future we may wish to have special processing for specific 
            read errors or we may wish to ignore them. */
	 else if( read_returned < 0 )
	    TRNS_abort( "ORPGDA_read failed (%d)\n", read_returned);

      } 

      /* Process message according to message type() */
      if( msg_type == GENERIC_DIGITAL_RADAR_DATA ){

         /* The elevation cut may need to be discarded ....  If return value is
            negative, discard the cut. */
         offset = 0;
         if( has_comm_header ){

            offset = RESP_OFFSET;
            radar_data -= RESP_OFFSET;

         }

         if( (radar_data = Process_radial ( (char *) radar_data, offset,
                                            has_supple_cuts )) == NULL )
            continue;

         /* Remove the headers. */
         if( has_comm_header ){

            rda_msg = radar_data;
            radar_data += RESP_OFFSET;

         }

      }
      else if( msg_type == RDA_STATUS_DATA )
         Process_status ( (char *) radar_data );

      else if( msg_type == RDA_RPG_VCP ){

         RDA_RPG_message_header_t *hdr = (RDA_RPG_message_header_t *) radar_data;
         int msg_size = hdr->size*sizeof(short);

         /* The VCP message size changes during translation.   Need to 
            account for this. */
         Process_vcp ( (char *) radar_data, &msg_size );

#ifdef BUILD13_OR_EARLIER
         /* Check for Supplemental Cuts, and if present, remove them. */
         has_supple_cuts = Remove_suppl_cuts_from_vcp( (char *) radar_data, 
                                                       &msg_size );
#endif

         if( TRNS_verbose ){

            LE_send_msg( GL_INFO, "RDA --> RPG VCP Data .......\n" );
            Write_vcp_data( radar_data );

         }
    
         /* Adjust the message size for the mandatory CTM header. */
         msg_size += CTM_HEADER_SIZE;

         /* Adjust the size in the CM header, if the message has a CM
            header. */
         if( has_comm_header ){

            CM_resp_struct *cm_hdr = (CM_resp_struct *) rda_msg;
            cm_hdr->data_size = msg_size;

            msg_size += CM_RESP_HEADER_SIZE;

         }

         read_returned = msg_size;

      }

      /* Convert the data back to external format.  Write the message to the response LB.  
         Write errors are considered fatal errors.  */
      if( ((ret1 = UMC_RDAtoRPG_message_header_convert( radar_data )) < 0)
                                  ||
         ((ret2 = UMC_RPGtoRDA_message_convert_to_external( msg_type, radar_data )) < 0)
                                  ||
         ((retval = ORPGDA_write( TRNS_response_out_lb, (char *) rda_msg, read_returned, LB_ANY )) < 0) ){
         
         TRNS_abort( "UMC or ORPGDA__write( TRNS_response_out_lb: %d ) Failed (%d, %d, %d)\n", 
                     TRNS_response_out_lb, ret1, ret2, retval );

      }

   }

} /* End of Check_response() */

#define REQUEST_READ_SIZE  128000

/*************************************************************************************

   Description:
      Reads the Wideband Request LB, determines if the message is something that
      needs to be processed, processes it, and write the processed message to 
      the alternate Wideband Request LB.

   Inputs:
      rpg_msg - pointer to buffer to receive the message.

**************************************************************************************/
void Check_request( char *rpg_msg ){

   static char *rpg_data = NULL;
   static RDA_RPG_message_header_t *msg_header;
   
   int read_returned, retval, msg_type;

   /* Do Until read error occurs. */
   while(1){

      read_returned = ORPGDA_read( TRNS_request_in_lb, rpg_msg, REQUEST_READ_SIZE, 
                                   LB_NEXT );
      if( read_returned > 0 ){

         CM_req_struct *cm_hdr = NULL;
         int msg_size;

         /* Strip off the CM_req_struct header and the CTM header. */
         rpg_data = rpg_msg + REQ_OFFSET;

         /* Do ICD to local host conversion of the message header.   This
            does conversion from Big Endian to Little Endian if host machine
            is Little Endian.  The RDA data is assumed Big Endian format.
            Also floating point number conversion (Concurrent to IEEE 754)
            is done. */
         UMC_RDAtoRPG_message_header_convert( rpg_data );

         /* Get message type. */
         msg_header = (RDA_RPG_message_header_t *) rpg_data;
         msg_type = (int) (msg_header->type);

         /* If RPG/RDA VCP data or RDA Control Command, convert the
            message data from external format to internal format (i.e., if the
            RDA is Big-Endian and the RPG is Little-Endian (or vice versa) based
            on the message type. */
         if( msg_type == RPG_RDA_VCP ){

            cm_hdr = (CM_req_struct *) rpg_msg;
            msg_size = (msg_header->size*sizeof(short)) + CTM_HEADER_SIZE;

            UMC_RDAtoRPG_message_convert_to_internal( msg_type, rpg_data );
   
            /* Process the VCP data .... processing includes translation of the 
               VCP data if translation for this VCP is required. */
            Process_vcp ( (char *) rpg_data, &msg_size );

            if( TRNS_verbose ){

               LE_send_msg( GL_INFO, "RDA --> RPG VCP Data .......\n" );
               Write_vcp_data( rpg_data );

            }

            /* Convert the data back to external format.  Write the message to the 
               request LB.  Write errors are considered fatal errors.  */
            if( (UMC_RDAtoRPG_message_header_convert( rpg_data ) < 0)
                                       ||
                (UMC_RPGtoRDA_message_convert_to_external( msg_type, rpg_data ) < 0) )
               TRNS_abort ("UMC Conversion Failed");

            /* Adjust the size in the CM header in the event the VCP was translated. */
            cm_hdr->data_size = msg_size + CTM_HEADER_SIZE;

            /* Adjust the size of the mesasge to be written to LB in the event
               the VCP was translated. */
            read_returned = cm_hdr->data_size + CM_REQ_HEADER_SIZE;

         }
         else if( msg_type == RDA_CONTROL_COMMANDS ){

            char *msg_body = rpg_data + sizeof(RDA_RPG_message_header_t);
            msg_size = (msg_header->size*sizeof(short)) + CTM_HEADER_SIZE;

            UMC_RDAtoRPG_message_convert_to_internal( msg_type, rpg_data );
   
            /* Process the RDA Control Command,if necessary.  If the command if for an
               RDA Elevation Restart and translation is active, need to change the cut 
               number needing to be restarted.  */
            Process_rda_control ( msg_body );

            /* Convert the data back to external format.  Write the message to the 
               request LB.  Write errors are considered fatal errors.  */
            if( (UMC_RDAtoRPG_message_header_convert( rpg_data ) < 0)
                                       ||
                (UMC_RPGtoRDA_message_convert_to_external( msg_type, rpg_data ) < 0) )
               TRNS_abort ("UMC Conversion Failed");

         }
         else{

            if( UMC_RDAtoRPG_message_header_convert( rpg_data ) < 0 )
               TRNS_abort ("UMC Conversion Failed");

         }

         retval = ORPGDA_write( TRNS_request_out_lb, rpg_msg, read_returned, LB_ANY );
         if( retval < 0 )
            TRNS_abort( "ORPGDA_write Failed" );

      }
      else if( read_returned == LB_TO_COME )
         break;

      else if( read_returned == LB_EXPIRED )
         ORPGDA_seek( TRNS_request_in_lb, 0, LB_FIRST, NULL );

      else
         TRNS_abort( "ORPGDA_read() Failed" );
      
   }

} /* End of Check_request() */

/***********************************************************************

   Description:  
      This function services the Supplemental VCP Information 
      LB update notification. 

   Inputs:       
      fd - file descriptor associated with updated LB.
      msgid - message ID of the updated message. 
      msg_info - length of message.
      arg - passes the data_id of updated LB.

   Outputs:

   Returns:      
      There is no return value defined for this function.

*********************************************************************/
void Lb_notify_handler( int fd, LB_id_t msgid, int msg_info, void *arg ){

   /* Check the fd to make sure it is ORPGDAT_SUPPL_VCP_INFO and 
      the message is the translation table information. */
   if( (fd == Suppl_vcp_info_lbfd) && (msgid == TRANS_TBL_MSG_ID) ){

      /* Set flag to indicate the LB was updated. */
      Suppl_vcp_info_updated = 1;

      LE_send_msg( GL_INFO, "Supplemental VCP Information Updated.\n" );

   }

/* End of Lb_notify_handler() */
}

