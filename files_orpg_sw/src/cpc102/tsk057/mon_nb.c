/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/09/05 15:54:22 $
 * $Id: mon_nb.c,v 1.12 2014/09/05 15:54:22 steves Exp $
 * $Revision: 1.12 $
 * $State: Exp $
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <orpg.h>
#include <comm_manager.h>
#include <prod_user_msg.h>
#include <time.h>
#include <mon_nb.h>

/* Static Global Variables. */
int Line = -1;               /* Line index to monitor. */
int Response_updated = 0;    /* Narrowband Line Response LB updated flag. */
int Request_updated = 0;     /* Narrowband Line Request LB updated flag. */
int Cm_instance = -1;        /* Comm Manager index. */
int Add_color = 0;	     /* Color code output option. */

char Message[256];
  
/* Static Function Prototypes. */
static int Read_response();
static int Read_request();
static void Process_connect_disconnect( int type, int ret_code );
static void Process_cm_event( int ret_code );
static void Response_lb_callback( int fd, LB_id_t msg_id, int msg_info,
                                  void *arg );
static void Request_lb_callback( int fd, LB_id_t msg_id, int msg_info,
                                 void *arg );
static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static int Cleanup_fxn( int signal, int status );

/************************************************************************

   Description: Main function for mon_nb tool.

                Monitors ICD message traffic for a specific narrowband
                line supplied as command line argument.

************************************************************************/
int main( int argc, char **argv ){

   int err = 0;
   int len;
   char *in_lb_name = NULL;
   char *out_lb_name = NULL;

   /* Process command line arguments. */
   if( (err = Read_options( argc, argv )) < 0 )
      exit(0);

   /* Register callback for LB notification of narrowband line 
      response lb. */
   err = ORPGDA_UN_register( ORPGDAT_CM_RESPONSE + Line,
                             LB_ANY, Response_lb_callback );
   if( err < 0 ){

      fprintf( stderr, "ORPGDA_UN_register For Response LB Returned Error (%d)\n",
               err );
      exit(0);

   }

   in_lb_name = ORPGDA_lbname( ORPGDAT_CM_RESPONSE + Line );

   if( Cm_instance >= 0 ){

      /* Register callback for LB notification of narrowband line 
         response lb. */
      err = ORPGDA_UN_register( Cm_instance, LB_ANY, Request_lb_callback );
      if( err < 0 ){

         fprintf( stderr, "ORPGDA_UN_register For Request LB Returned Error (%d)\n",
                  err );
         exit(0);

      }

   }

   if( in_lb_name != NULL )
      len = sprintf( Message, "Monitoring Narrowband Line %d (%s)", Line, in_lb_name );

   else
      len = sprintf( Message, "Monitoring Narrowband Line %d", Line );

   Print_msg( len, -1 );

   out_lb_name = ORPGDA_lbname( Cm_instance );
   if( out_lb_name != NULL )
      len = sprintf( Message, "Monitoring Comm Manager Instance %d, (%s)\n", 
                     Cm_instance - ORPGDAT_CM_REQUEST, out_lb_name );

   else
      len = sprintf( Message, "Monitoring Comm Manager Instance %d\n", 
                     Cm_instance - ORPGDAT_CM_REQUEST );
   Print_msg( len, -1 );

   /* Register termination handler. */
   ORPGTASK_reg_term_handler( Cleanup_fxn );

   if( Add_color )
      fprintf( stdout, BCKGRND "\n" );

   /* Main processing loop. */
   while(1){

      if( Request_updated ){

         Request_updated = 0;
         if( (err = Read_request()) < 0 )
            break;

      }

      if( Response_updated ){

         Response_updated = 0;
         if( (err = Read_response()) < 0 )
            break;

      }

      sleep(10);

   /* End of "while" loop. */
   }

   if( Add_color )
      fprintf( stdout, RESET );

   return 0;

}

/**************************************************************************************

   Description:
      Termination handler. 

   Returns:
      Always returns 0. 

*************************************************************************************/
static int Cleanup_fxn( int signal, int status ){

   if( Add_color )
      fprintf( stdout, RESET );

   return (0);

/* End of Cleanup_fxn() */
}


/**************************************************************************

   Description: Reads the Response LB for narrowband line Line. 
                Processes all CM response types.
      
   Returns:     Negative number on error.

**************************************************************************/
static int Read_response(){

   int err, len;
   CM_resp_struct *resp;
   short *msg_data;
   char *buf = NULL;

   /* Do until no more uread messages in LB. */
   while(1){

      buf = NULL;
      err = ORPGDA_read( ORPGDAT_CM_RESPONSE + Line, &buf,
                         LB_ALLOC_BUF, LB_NEXT );

      if( err < 0 ){

         if( err == LB_TO_COME )
            break;

         fprintf( stderr, "ORPGDA_read Error (%d)\n", err );
         if( err == LB_EXPIRED )
            continue;

         return(err);

      }

      resp = (CM_resp_struct *) buf;

      if( resp->link_ind != Line ){

         fprintf( stderr, "resp->link_ind != Line\n" );
         err = -1;
         if( buf != NULL )
            free( buf );
         return(err);

      }

      switch( resp->type ){

         case CM_LOST_CONN:

            len = sprintf( Message, "---> LOST LINE CONNECTION" );
            Print_msg( len, RED );
            err = -1;
            break;

         case CM_STATUS:

            len = sprintf( Message, "---> CM STATUS DATA" );
            Print_msg( len, GRAY );
            break;

         case CM_DATA:
   
            /* Strip off the comm manager header if data, then process the data. */
            msg_data = (short *) (buf + sizeof(CM_resp_struct));

            Process_incoming_ICD_message( msg_data, resp->data_size );
            break;

         case CM_WRITE:
            break;

         case CM_EVENT:

            Process_cm_event( resp->ret_code );
            break;

         case CM_CANCEL:
         case CM_SET_PARAMS:
            break;
     
         case CM_CONNECT:
         case CM_DISCONNECT:
     
            Process_connect_disconnect( resp->type, resp->ret_code );
            break;
         
         default:

            len = sprintf( Message, "---> UNKNOWN TYPE (%d)", resp->type );
            Print_msg( len, -1 );
            break;

      /* End of "switch" statement. */
      }

      /* Free the read buffer. */
      if( buf != NULL )
         free(buf);

   /* End of "while" loop. */
   }

   return(0);

/* End of Read_response() */
}

/**************************************************************************

   Description: Reads the Request LB for narrowband line Line. 
                Processes all CM request types.
      
   Returns:     Negative number on error.

**************************************************************************/
static int Read_request(){

   int read_len, len;
   CM_req_struct *req;
   short *msg_data;
   char *buf = NULL;

   /* Do until all unread messages read. */
   while(1){

      buf = NULL;
      read_len = ORPGDA_read( Cm_instance, &buf, LB_ALLOC_BUF, LB_NEXT );

      if( read_len < 0 ){

         if( read_len == LB_TO_COME )
            break;

         fprintf( stderr, "ORPGDA_read Error (%d)\n", read_len );
         if( read_len == LB_EXPIRED )
            continue;
    
         return(read_len);

      }

      req = (CM_req_struct *) buf;

      if( req->link_ind != Line ){

         if( buf != NULL )
            free( buf );

         continue;

      }

      switch( req->type ){

         case CM_STATUS:
            break;

         case CM_WRITE:

            /* Strip off the comm manager header if data, then process the data. */
            msg_data = (short *) (buf + sizeof(CM_req_struct));

            Process_outgoing_ICD_message( (CM_req_struct *) buf, msg_data, req->data_size );
            break;

         case CM_DATA:
            break;

         case CM_EVENT:
            break;

         case CM_CANCEL:
         case CM_SET_PARAMS:
            break;
     
         case CM_CONNECT:

            len = sprintf( Message, "<--- REQUESTING LINE CONNECTION\n" );
            Print_msg( len, BLACK );
            break;

         case CM_DISCONNECT:
     
            len = sprintf( Message, "<--- REQUESTING LINE DISCONNECTION\n" );
            Print_msg( len, BLACK );
            break;
         
         default:
            break;

      /* End of "switch" statement. */
      }

      /* Free the read buffer. */
      if( buf != NULL )
         free(buf);

   /* End of "while" loop. */
   }

   return(0);

/* End of Read_request() */
}

/*************************************************************************************

   Description:  Services Connect/Disconnnect 

   Inputs:     type - CM_CONNECT or CM_DISCONNECT
               ret_code - return code from comm manager.
   
*************************************************************************************/
static void Process_connect_disconnect( int type, int ret_code ){

   char status[32];
   int len, color = GRAY;

   switch( ret_code ){

      case CM_SUCCESS:
         len = sprintf( status, "SUCCESSFUL" );
         status[len] = '\0';
         color = GREEN;
         break;

      case CM_TIMED_OUT:
         len = sprintf( status, "TIMED OUT" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_NOT_CONFIGURED:
         len = sprintf( status, "NOT CONFIGURED" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_DISCONNECTED:
         len = sprintf( status, "DISCONNECTED" );
         status[len] = '\0';
         color = ORANGE;
         break;

      case CM_CONNECTED:
         len = sprintf( status, "CONNECTED" );
         status[len] = '\0';
         color = GREEN;
         break;

      case CM_BAD_LINK_NUMBER:
         len = sprintf( status, "BAD LINK NUMBER" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_INVALID_PARAMETER:
         len = sprintf( status, "INVALID PARAMETER" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_TOO_MANY_REQUESTS:
         len = sprintf( status, "TOO MANY REQUESTS" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_IN_PROCESSING:
         len = sprintf( status, "IN PROCESSING" );
         status[len] = '\0';
         color = GREEN;
         break;

      case CM_TERMINATED:
         len = sprintf( status, "TERMINATED" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_FAILED:
         len = sprintf( status, "FAILED" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_REJECTED:
         len = sprintf( status, "REJECTED" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_LOST_CONN:
         len = sprintf( status, "LOST CONNECTED" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_LINK_ERROR:
         len = sprintf( status, "LINK ERROR" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_START:
         len = sprintf( status, "START" );
         status[len] = '\0';
         color = GREEN;
         break;

      case CM_TERMINATE:
         len = sprintf( status, "TERMINATE" );
         status[len] = '\0';
         color = ORANGE;
         break;

      case CM_STATISTICS:
         len = sprintf( status, "STATISTICS" );
         status[len] = '\0';
         color = GREEN;
         break;

      case CM_EXCEPTION:
         len = sprintf( status, "EXCEPTION" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_NORMAL:
         len = sprintf( status, "NORMAL" );
         status[len] = '\0';
         color = GREEN;
         break;

      case CM_PORT_IN_USE:
         len = sprintf( status, "PORT IN USE" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_DIAL_ABORTED:
         len = sprintf( status, "DIAL ABORTED" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_INCOMING_CALL:
         len = sprintf( status, "IN COMING CALL" );
         status[len] = '\0';
         color = GREEN;
         break;

      case CM_BUSY_TONE:
         len = sprintf( status, "BUSY TONE" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_PHONENO_FORBIDDEN:
         len = sprintf( status, "PHONE NUMBER FORBIDDEN" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_PHONENO_NOT_STORED:
         len = sprintf( status, "PHONE NUMBER NOT STORED" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_NO_DIALTONE:
         len = sprintf( status, "NO DIAL TONE" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_MODEM_TIMEDOUT:
         len = sprintf( status, "MODEM TIMED OUT" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_INVALID_COMMAND:
         len = sprintf( status, "INVALID COMMAND" );
         status[len] = '\0';
         color = RED;
         break;

      case CM_TRY_LATER:
         len = sprintf( status, "TRY LATER" );
         status[len] = '\0';
         color = RED;
         break;

      default:
         len = sprintf( status, "%d (\?\?\?\?)", ret_code );
         status[len] = '\0';
         break;

   /* End of "switch" statement */
   }

   if( type == CM_CONNECT ){

      len = sprintf( Message, "---> CONNECTION REQUEST RETURNED: %s\n", status );
      Print_msg( len, color );

   }
   else if( type == CM_DISCONNECT ){

      len = sprintf( Message, "---> DISCONNECTION REQUEST RETURNED: %s\n", status );
      Print_msg( len, color );

   }

/* End of Process_connect_disconnect() */
}

/*************************************************************************************

   Description: Services Comm Manager Events.

   Inputs:     ret_code - return code in event.

*************************************************************************************/
static void Process_cm_event( int ret_code ){

   int len;

   switch (ret_code) {

      case CM_LOST_CONN:
      case CM_LINK_ERROR:
         len = sprintf( Message, 
                  "---> CM EVENT: CM LOST CONNECTION (%d)\n", ret_code );
         Print_msg( len, RED );
         break;

      case CM_TERMINATE:
         len = sprintf( Message, "---> CM EVENT: CM TERMINATED\n" );
         Print_msg( len, RED );
         break;

      case CM_START:
         len = sprintf( Message, "---> CM EVENT: CM STARTED\n" );
         Print_msg( len, BLACK );
         break;

      case CM_TIMED_OUT:
         len = sprintf( Message, "---> CM EVENT: DATA TRANSMIT TIME-OUT\n" );
         Print_msg( len, RED );
         break;

      case CM_STATISTICS:
         break;

      case CM_EXCEPTION:
         len = sprintf( Message, "---> CM EVENT: CM EXCEPTION\n" );
         Print_msg( len, RED );
         break;

      case CM_NORMAL:
         len = sprintf( Message, "---> CM EVENT: CM NORMAL\n" );
         Print_msg( len, BLACK );
         break;

      default:
         break;

   /* End of "switch" statement. */

   }

/* End of Process_cm_event() */
}

/**************************************************************************

   Description: LB notification callback function for response LB updates.

   Inputs:     See LB man page for description.

   Returns:    There is no return value for this function.
      
**************************************************************************/
static void Response_lb_callback( int fd, LB_id_t msg_id, int msg_info,
                                  void *arg ){

   /* Set the Response LB updated flag. */
   Response_updated = 1;

/* End of Response_lb_callback() */
}

/**************************************************************************

   Description: LB notification callback function for request LB updates.

   Inputs:     See LB man page for description.

   Returns:    There is no return value for this function.
      
**************************************************************************/
static void Request_lb_callback( int fd, LB_id_t msg_id, int msg_info,
                                 void *arg ){

   /* Set the Response LB updated flag. */
   Request_updated = 1;

/* End of Response_lb_callback() */
}

/**************************************************************************

   Description: Prints message to stdout, similar to lelb_mon.

   Inputs:     len - length of message text, in bytes.
               color - color to paint the text

**************************************************************************/
void Print_msg( int len, int color ){

   static char disp_date[9];
   static char disp_time[9];
   static int cur_day, cur_month, cur_yr;

   int day, month, year, yr, hr, min, sec;
   time_t cur_time;

   cur_time = time(NULL); 
   unix_time( (time_t *) &cur_time, &year, &month, &day, &hr, &min, &sec );
   yr = year % 100;

   if ((cur_day != day) || (cur_month != month) || (cur_yr != yr)) {

      /* Display the day/month/year information only if it has changed. */
      cur_day = day ; cur_month = month ; cur_yr = yr ;
      sprintf(disp_date, "%02d/%02d/%02d", cur_month,cur_day,cur_yr) ;
      if( Add_color )
         fprintf( stdout, BLKTEXT "%s\n" RESETTC, disp_date );

      else
         fprintf( stdout, "%s\n", disp_date );

   }                                        

   Message[len] = '\0';
   sprintf( disp_time, "%02d:%02d:%02d", hr, min, sec );

   if( !Add_color )
      fprintf( stdout, "%s  %s\n", disp_time, Message );

   else{

      fprintf( stdout, BCKGRND );

      if( color == BLACK )
         fprintf( stdout, BLKTEXT "%s  %s\n" RESETTC, disp_time, Message );

      else if( color == BLUE )
         fprintf( stdout, BLUTEXT "%s  %s\n" RESETTC, disp_time, Message );

      else if( color == GRAY )
         fprintf( stdout, GRYTEXT "%s  %s\n" RESETTC, disp_time, Message );

      else if( color == YELLOW )
         fprintf( stdout, YLWTEXT "%s  %s\n" RESETTC, disp_time, Message );

      else if( color == RED )
         fprintf( stdout, REDTEXT "%s  %s\n" RESETTC, disp_time, Message );

      else if( color == ORANGE )
         fprintf( stdout, ORGTEXT "%s  %s\n" RESETTC, disp_time, Message );

      else if( color == GREEN )
         fprintf( stdout, GRNTEXT "%s  %s\n" RESETTC, disp_time, Message );

      else 
         fprintf( stdout, GRYTEXT "%s  %s\n" RESETTC, disp_time, Message );

   }

/* End of Print_msg() */
}

/**************************************************************************

   Description: This function reads command line arguments.

   Inputs:     argc - number of command arguments
               argv - the list of command arguments

   Return:     It returns 0 on success or -1 on failure.

**************************************************************************/
static int Read_options (int argc, char **argv){

   extern char *optarg;    /* used by getopt */
   extern int optind;
   int c;                  /* used by getopt */
   int err;                /* error flag */

   err = 0;
   Line = -1;
   Cm_instance = -1;
   Terse_mode = 0;
   Add_color = 0;

   /* Must have at least 1 command line argument. */
   if( argc == 1 ){

      Print_usage( argv );
      return (-1);

   }
      
   while ((c = getopt (argc, argv, "cth?")) != EOF){

      switch (c) {
  
         case 't':
         Terse_mode = 1;
         break;

         case 'c':
            Add_color = 1;
            break;

         case 'h':
         default:
            Print_usage (argv);
            break;

      }

   }
  
   /* Get the narrowband line index. */
   if( optind == argc - 1)
      sscanf( argv[optind], "%d", &Line );

   if( (Line < 1) || (Line > 47) ){

      fprintf( stderr, "Line Index Error.  Must Be In Range 1-47\n" );
      err = -1;

   }

   /* Get the comm manager LB_id based on this line index. */
   Cm_instance = ORPGCMI_request( Line );
   if( Cm_instance < 0 ){

      fprintf( stderr, "Comm Manager Instance Failed\n" );
      err = -1;

   }

   return (err);

/* End of Read_options() */
}                       

/***************************************************************************

   Description: This function prints usage information.

   Inputs:     argv - the list of command line arguments

   Return:     There is no return value for this function.

***************************************************************************/
static void Print_usage (char **argv){

   printf ("\n Usage: %s [options] line index\n", argv[0]);
   printf ("\n Options:\n");
   printf ("     t - terse mode\n");
   printf ("     c - color code some of the output (Default: NO)\n");
   printf ("     h - help (see also man page mon_nb(1))\n");
   printf ("\n NOTE: line index must be in the range 1 - 47 inclusive\n");
   exit (0);

/* End of Print_usage() */
}
