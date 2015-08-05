/**************************************************************************
   
   Module:  otr_main.c
   
   Description:
   
   This process is the Scheduler for One-time products.
   One-time requests can be made by users through p_server, and
   alert paired products can be requested by the alerting task.  
   See the RPG/PUP ICDs for the format of the Product Request Message and the
   Request Response Message.
         
   One-time requests messages from the p_server instances and the alerting
   task are posted to the onetime request LB.  This process reads the
   requests.  The message can be of three types: 1.) a request
   for a product, 2.) a request for an alert paired product, or 
   3.) a request to delete all outstanding requests from a specific
   line.  
   
   This main routine initializes data and event handlers, then goes
   into a loop.  The loop checks for events and performs the processing
   to go with those events, passing request message to the appropriate
   routine, and calling periodic routines to check for timeouts and
   special processing at the start of a volume scan.                       
   
   Assumptions:
   It is assumed that requests are not put in the LB from
   users that are not privileged enough to make the request. 
   
   It is assumed that the fields in the message requesting the
   products have already been checked and are valid before being
   put on the LB.
   
   **************************************************************************/
/*
* RCS info
* $Author: steves $
* $Locker:  $
* $Date: 2006/05/30 15:02:44 $
* $Id: otr_main.c,v 1.54 2006/05/30 15:02:44 steves Exp $
* $Revision: 1.54 $
* $State: Exp $
*/
/*
* System Include Files/Local Include Files
*/
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <prod_user_msg.h>
#include <prod_distri_info.h>
#include "otr_new_volume_product_requests.h"
#include <otr_process_onetime_requests.h>
#include <le.h>
#include <assert.h>
#include <orpg.h>
#include <prod_gen_msg.h>
#include <otr_alert_paired_product_request.h>
#include <itc.h>
#include <rpgc.h>
/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
/*
* Static Globals
*/

/* this indicates the size of the task log file, in number
   of messages. */
static int Log_file_nmsgs = 1000;

/* this indicates that a new volume has been received and 
   that one-time requests for current volume need to be
   processed. */
static int OTR_new_volume = 0;

/* this indicates that the status for some product has changed
    and a check should be made for a fulfilled request */
static int OTR_new_product = 0;

/* this indicates that the replay process has responded */
static int OTR_replay_product = 0;

extern  Hrdb_date_time hrdb_data; /* USP itc data gets written here */

/*
* Static Function Prototypes
*/
void handle_new_alert_signal(en_t event_code, void *message, size_t message_length);
void handle_volume_signal(en_t event_code, void *message, size_t message_length);
void handle_request_signal(en_t event_code, void *message, size_t message_length);
void handle_product_signal(en_t event_code, void *message, size_t message_length);
void handle_replay_response_signal(en_t event_code, void *message, size_t message_length);
int Read_options( int argc, char *argv[] );

/**************************************************************************
   Description: This is the main for the one-time request handler.
   It first validates the invoking command line, then goes into a loop.
   It waits for start of volume events, and for requests to be added to an LB.
   It will read the request LB for a message header, then passes this
   header to the routine that reads and decodes the individual requests.
   
   Input: 
         Command line arguments for help or verbose output. 
         Events:
            ORPGEVT_OT_REQUEST - a new request has been posted
            ORPGEVT_START_OF_VOLUME - a new volume scan has started
            ORPGEVT_PROD_STATUS - a product's generation status has changed (e.g. new product made)
            ORPGEVT_WX_ALERT_OT_REQ - An alert paired product request has been posted
            ORPGEVT_REPLAY_RESPONSE - replay has generated a requested product
         LBs:
            ORPGDAT_OT_REQUESTS - one time requests
                   
   
   Output: All handled by subroutines.
   
   Returns: The routine returns a -1 if it exits because of an error in
      the invoking command line.
      It returns via the standard ORPGTASK_exit if it can't open the
      LBs that it needs, or it can't register for events.  Otherwise this 
      process continues until terminated via a signal.
   
   Notes:
**************************************************************************/
int main(int argc, char **argv){

   int   event_registration_status;  /* status of the attempt at event registration */
   int   request_lb_status;          /* status of last action on the onetime request LB */
   LB_id_t id;                       /* message id for the current request in the onetime request LB */
   LB_info info;                     /* information on the message in the onetime request LB */
   Pd_msg_header *message_header_ptr;
   int retval ;                      /* returned value for the task initialization */
   int usp_itc_id = ITC_CD07_USP;    /* ID for the USP data to get end hour */
   int sync_prd = ITC_ON_CALL;       /* will only need this in certain circumstances */


   /* First validate command line */
   retval = Read_options( argc, argv );
   if( retval < 0 ){

      LE_send_msg( GL_ERROR, "Command Line Options Failed\n" );
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }
   
   /* Command line is valid, now initialize the process */
   if( ORPGMISC_init(argc, argv, Log_file_nmsgs, 0, -1, 0) < 0 ){

       LE_send_msg( GL_ERROR, "ORPGMISC_init Failed\n" );
       ORPGTASK_exit(GL_EXIT_FAILURE); 

   }

   retval = ORPGTASK_reg_term_hdlr (NULL);  /* register the process termination handler to default */
   if (retval < 0) {
      LE_send_msg(GL_ERROR| LE_VL0, "ORPGTASK_reg_term_hdlr failed: %d", retval) ;
      exit(EXIT_FAILURE) ;
   }
   
   /* register the ITC for USP product to be able to get the last end hour value */
   RPGC_itc_in (usp_itc_id, (char *)&hrdb_data, sizeof(hrdb_data), sync_prd);

   OTR_new_volume = 0;
   OTR_new_product = 0;
   /* set LB pointers to the latest message in the buffer */
   id = LB_LATEST;
   request_lb_status = ORPGDA_info(ORPGDAT_OT_REQUEST, id, &info);
   request_lb_status = ORPGDA_info(ORPGDAT_REPLAY_RESPONSES, id, &info);
   
   /* initialize event handlers */
   event_registration_status = EN_register(ORPGEVT_START_OF_VOLUME,
                                           (void *) &handle_volume_signal);
   if (event_registration_status != 0){
      LE_send_msg (GL_ERROR| LE_VL0, "could not register start of volume event handler \n");
      ORPGTASK_exit (GL_CONFIG);
   }
   event_registration_status = EN_register(ORPGEVT_OT_REQUEST,
                                           (void *) &handle_request_signal);
   if (event_registration_status != 0){
      LE_send_msg (GL_ERROR| LE_VL0,  "could not register one-time request event handler \n");
      ORPGTASK_exit (GL_CONFIG);
   }
   
   event_registration_status = EN_register(ORPGEVT_PROD_STATUS,
                                           (void *) &handle_product_signal);
   if (event_registration_status != 0){
      LE_send_msg (GL_ERROR| LE_VL0,  "could not register one-time request event handler \n");
      ORPGTASK_exit (GL_CONFIG);
   }

   event_registration_status = EN_register(ORPGEVT_WX_ALERT_OT_REQ,
                                           (void *) &handle_new_alert_signal);
   if (event_registration_status != 0){
      LE_send_msg (GL_ERROR| LE_VL0,  "could not register alert paired product request event handler \n");
      ORPGTASK_exit (GL_CONFIG);
   }

   event_registration_status = EN_register(ORPGEVT_REPLAY_RESPONSES,
                                     (void *) &handle_replay_response_signal);
   if (event_registration_status != 0){
      LE_send_msg (GL_ERROR| LE_VL0,  "could not register replay response event handler \n");
      ORPGTASK_exit (GL_CONFIG);
   }

   OTR_update_elevation_data();
   
   /* end of initialization, now start the processing loop */
   
   /*do forever:      */
   for (;;){
      sleep(5);   /*Wait for  new product status event */
                  /* note: the argument for the sleep function
                      added to REPLAY_TIMEOUT in otr_new_volume_product_request.c
                      must add up to less than 90 seconds to meet one time
                      response requirements */
                   /* or ORPGEVT_START_OF_VOLUME */
                   /* or ORPGEVT_OT_REQUEST  (i.e. Request placed on LB) */
                   /* or ORPGEVT_WX_ALERT_OT_REQ (i.e. An alert paired product request) */
                   /* or ORPGEVT_PROD_STATUS (i.e. Product generation status changed */
                   /* or ORPGEVT_REPLAY_RESPONSE (i.e. a product from replay) */
                   /* or a timeout (check for replay timeouts) */

      
      /* check for a reply received from the replay stream */
      if (OTR_replay_product > 0){
          OTR_replay_product= 0;
          if (OTR_replay_message_received() > 0){
               /* if not all replays were read e.g. Due to LB_to_COME
                * then flag for reading again next time 
                */
               OTR_replay_product++; 
          }
      }

     
      /*If the wait ended due to a ORPGEVT_PROD_STATUS event, 
         then do all the new-product processing */
      if (OTR_new_product > 0){
         OTR_new_product = 0;
         OTR_product_status_update();
      }
      
      /*If the wait ended due to a ORPGEVT_START_OF_VOLUME event, 
        then do all the new-volume processing e.g. posting requests for the new volume*/
      if (OTR_new_volume > 0){
          OTR_new_volume = 0;
          OTR_new_volume_product_requests();
      }
     
    /* now check the user onetime requests LB and
       loop until there is nothing left on the request LB */
      
    request_lb_status = 0;
    while(request_lb_status >= 0){
       request_lb_status = ORPGDA_read (ORPGDAT_OT_REQUEST, 
                    (char **) &message_header_ptr, 
                    LB_ALLOC_BUF, LB_NEXT);
       if(request_lb_status > 0){

          LE_send_msg(GL_INFO | LE_VL3,"one time request LB read,  status %d \n", 
                      request_lb_status);
          id = ORPGDA_get_msg_id();
          if(message_header_ptr->n_blocks >= 1){
          
             /* verify message size with assert 
                (this is for developmental debugging) */
              assert(request_lb_status == (sizeof(Pd_msg_header)
                +(message_header_ptr->n_blocks - 1) 
                * (sizeof(Pd_request_products))));
             if (message_header_ptr->n_blocks == 1){
             /* 
               no requests are with the header block, so cancel
               requests from this user line
             */
              LE_send_msg(GL_INFO | LE_VL3, "1 - Remove volume requests\n" );
              OTR_remove_volume_requests(message_header_ptr);
             }
             else if (id == ALERT_OT_REQ_MSGID){
                LE_send_msg(GL_INFO | LE_VL3, "2 - Process alert-paired product request\n" );
                OTR_alert_paired_product_request(message_header_ptr);
             }
             else {
                LE_send_msg(GL_INFO | LE_VL3, "3 - Process onetime request\n" );
                OTR_process_onetime_requests(message_header_ptr, id);
             } /* end if (message_header_ptr->n_blocks == 1) */
          } /* end if (message_header_ptr->n_blocks >= 1 */
          free ((void *)message_header_ptr);
       } /* end if (request_lb_status > 0) */
    }/* end while(request_lb_status >= 0) */
  
      /* check for replay timeouts and send next request to replay stream*/
      OTR_replay_timeout_check();
 
   } /*end do forever*/
} /* end main program */

/**************************************************************************
   Description: 
      Handles the event notification of a message being placed on the 
      alert paired product request lb.
   
   Input: None

   Output: None

   Returns: Void
   
   Notes: 
      This routine must comply with the requirements of a signal handling
      routine.

**************************************************************************/
void handle_new_alert_signal( en_t event_code, void *message, 
                              size_t message_length ){
   
   /* having the event handler here will interrupt the sleep call in the 
      main program and start the read of the alert paired product request LB,
      so no other action is needed. */   

}

/**************************************************************************
   Description: 
      Handles the event notification of a message being placed on the 
      request lb.
   
   Input: None

   Output: None

   Returns: void
   
   Notes: 
      This routine must comply with the requirements of a signal handling
      routine.
**************************************************************************/
void handle_request_signal( en_t event_code, void *message, 
                            size_t message_length ){

   /* having the event handler here will interrupt the sleep call in the 
      main program and start the read of the request LB, so no other action
      is needed. */   

}

/**************************************************************************
   Description: 
      This handles processing for the start-of-volume event.

   Input: None

   Output: None

   Returns: Void

   Notes:  
      This routine must comply with the requirements of a signal handling
      routine.
**************************************************************************/
void handle_volume_signal( en_t event_code, void *message, 
                           size_t message_length ){

   /* processing needed at start-of-volume signal */
   /* indicate that a new volume has been received */
   OTR_new_volume++;

}

/**************************************************************************
   Description: 
      This handles processing for the new-product-status event.

   Input: None

   Output: None

   Returns: Void

   Notes:  
      This routine must comply with the requirements of a signal handling
      routine.
**************************************************************************/
void handle_product_signal( en_t event_code, void *message,
                            size_t message_length ){

   /* processing needed at new-product-status signal */
   /* indicate that a new product has been received */
   OTR_new_product++;

}

/**************************************************************************
   Description: 
      This handles processing for the replay reply event.

   Input: None

   Output: None

   Returns: Void

   Notes:  
      This routine must comply with the requirements of a signal handling
      routine.
**************************************************************************/
void handle_replay_response_signal( en_t event_code, void *message,
                                    size_t message_length ){

   /* processing needed at new-replay product-status signal */
   OTR_replay_product++;    

}

/**************************************************************************

   Description:
      Processes the command line arguments.

   Inputs:
      argc - # of command line arguments.
      argv - the command line arguments.

   Outputs:

   Returns:
      0 on success, -1 on failure.

   Notes:

***************************************************************************/
int Read_options( int argc, char *argv[] ){

   int c;                            /* used by getopt to hold command line character */
   int err=0;                        /* indicates a help action was selected */
   opterr = 0; /* don't print error if -v has no number */

   while ((c = getopt (argc, argv, "hl:v:?")) != EOF) {
      switch (c) {
    
         case 'v':   /* verbosity level */
         { 
             int v_level = 0;       
             sscanf (optarg, "%d", &v_level);
             LE_local_vl(v_level);
             break;
         }

         case 'l':
         {
            Log_file_nmsgs = atoi(optarg);
            if( Log_file_nmsgs < 0 || Log_file_nmsgs > 5000 )
               Log_file_nmsgs = 1000;
            break;

         }

         case 'h':   /* help for command line */
         case '?':
         {
            if (optopt == 'v'){

                  /* if -v had no number, set verbosity to one */
                  LE_local_vl(1);
            }
            else
               err = 1;
            
            break;

         }

      }

   }

   if (err == 1) {              /* Print usage message and exit */
      printf ("Usage: %s (options) \n", argv[0]);
      printf ("       Options:\n");
      printf ("       -h print this message\n");
      printf ("       -v (verbose mode)\n");
      printf ("       -l (number of log file messages)\n");
      ORPGTASK_exit (GL_EXIT_FAILURE);
   }


   return 0;

}
