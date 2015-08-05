/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/13 15:01:26 $
 * $Id: change_nyquist.c,v 1.1 2014/05/13 15:01:26 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 *
 */  

#include "change_nyquist.h"
#include "archive_II.h"
#include <orpggdr.h>
#include <unistd.h>
#include <ctype.h>

/* macro definitions */

#define MIN_MSG_SIZE    COMMGR_HDR_SIZE + CTM_HDRSZE_BYTES \
                        + sizeof (RDA_RPG_message_header_t)

/* Used for aliasing velocity. */
float NYQUIST_VEL = 0.0;

/* Global Variables. */

static char  ICAO [5];                  /* site ICAO/id */
static int   Input_data_id = CLDM_SR_BASEDATA;          
					/* Data ID (LB) to read BaseData from */


/* Function Prototypes. */

static void  Print_usage (char **argv);
static void  Read_A2_data (void);
static int   Read_options (int argc, char *argv[]);
static int   Termination_Handler (int signal, int exit_code);
static void  Rda_status_updated (EN_id_t event, char *msg, int msg_size,
                                 void *arg);



/********************************************************************************

  Description: This is the main routine for change_nyquist.  It performs 
               initialization then calls the runtime loop Read_A2_data ().

        Input: None

       Output: None

       Return: None

        Notes: None

*********************************************************************************/
int main (int argc, char *argv[]) {

   int ret;

   /* Initialize the ICAO. */
   memset( &ICAO[0], 0, 5 );

   /* read options */
   if (Read_options (argc, argv) != 0)
      exit (1);

   /* Register termination handler */
   if ((ret = ORPGTASK_reg_term_handler (Termination_Handler)) < 0) {

      LE_send_msg (GL_ERROR, "Term handler register Failed (ret: %d)", ret);
      exit(1);

   }

   /* Initialize Log-Error services. */
   if ((ret = ORPGMISC_LE_init( argc, argv, 1500, 0, -1, 0)) < 0) {

      LE_send_msg (GL_ERROR, "ORPGMISC_LE_init() Failed (ret: %d)", ret);
      ORPGTASK_exit (GL_EXIT_FAILURE);

   }

   /* Initialize DEAU. */
   ORPGMISC_deau_init();

   /* initialize RPG parameters */
   if (strlen( &ICAO[0] ) == 0) {

      if (INIT_RPG_parameters (ICAO) < 0)
         ORPGTASK_exit (GL_EXIT_FAILURE);

   }

   /* register for Recombined ORPGEVT_RDA_STATUS_CHANGE events */
   EN_register (ORPGEVT_RDA_STATUS_CHANGE, Rda_status_updated);

   /* initialize the input data LBs */
   ORPGDA_seek (Input_data_id, 0, LB_LATEST, NULL);

   /* clear the Metadata buffer if the input data source changes */
   MM_clear_metadata_buf ();

   /* Initialize record information */
   WR_init_record_info (ICAO);

   /* Enter the runtime loop */
   Read_A2_data ();

   exit (EXIT_SUCCESS);

}


/********************************************************************************

    Description: This is the runtime loop that reads and packages Level II data
                 in LDM format

          Input:

         Output: LDM_vcp_stats - is globally defined at the beginning of this file.

         Return:
   
********************************************************************************/
 static void Read_A2_data (void) {

   int   header_offset;
   int   comm_mgr_type;
   int   ret;
   char  *msg_buf = NULL;
   int   last_msg_discarded = -1;
   short previous_rda_status = -1;
   msg_data_t msg_data;

   /* initialize variables before entering the runtime loop */
   msg_data.rda_config = ORPGRDA_get_rda_config (NULL);
   msg_data.radial_status = END_VOL;

   if ((msg_data.rda_status = ORPGRDA_get_status (RS_RDA_STATUS)) < 0)
      msg_data.rda_status = RS_STANDBY;

   /* main runtime loop */
   while (1) {

      /* Read the next msg */
      ret = RM_read_next_msg (&msg_buf, Input_data_id); 

      /* Process any read errors */
      if( ret < 0 ){

         if( ret == LB_TO_COME ) 
            sleep (1); /* wait for new data to arrive */

         else if( ret == LB_EXPIRED ){

            LE_send_msg (GL_ERROR, 
                         "Messages Expired .... Skip to first unread message");
            ORPGDA_seek (Input_data_id, 0, LB_LATEST, NULL);

         } 
         else
            LE_send_msg (GL_ERROR, 
                        "ORPGDA_read() Failed: data_id: %d; read err: %d",
                        Input_data_id, ret);

         continue;

         /* Discard any msgs that are just comm manager messages, or any data 
            messages that are less than the minimum message length allowed */

      } 

      /* Determine the offset to the msg header */
      RM_determine_msg_hdr_offset( msg_buf, &header_offset, &comm_mgr_type,
                                   Input_data_id );
      
      if ((comm_mgr_type != CM_DATA) || (ret < MIN_MSG_SIZE)) {

          if ((comm_mgr_type == CM_DATA) && (ret < MIN_MSG_SIZE))
            LE_send_msg (GL_ERROR, 
               "Msg discarded: Msg size (%d bytes) < MIN_MSG_SIZE (%d bytes); (Data_id: %d)",
               ret, MIN_MSG_SIZE, Input_data_id);

          free (msg_buf);
          continue;

      } else if (ret > MAXLEN_MESSAGE_LEN) {
          LE_send_msg (GL_ERROR, 
             "Msg size (%d) is > MAXLEN_SIZE allowed (%d)...clipping msg)",
             ret, MAXLEN_MESSAGE_LEN);
          ret = MAXLEN_MESSAGE_LEN;
      }

      /* Get various message properties we need to process the message */
      RM_extract_msg_data (msg_buf + header_offset, &msg_data, previous_rda_status); 

      /* Process the message based on the LDM perspective of message type */
      if (msg_data.ldm_msg_type == CLDM_RADIAL_DATA) {

         /* Add the msg to the radial buffer */
         WR_add_msg_to_radial_buffer (msg_buf + header_offset, &msg_data,
                                      previous_rda_status);

         /* Process the Metadata msg */

      } else if (msg_data.ldm_msg_type == CLDM_META_DATA) 
         MM_process_metadata_msg (msg_buf + header_offset, msg_data);

      else {  /* discard this msg - we're not processing it */
         if (last_msg_discarded != msg_data.msg_hdr.type) {
            LE_send_msg (GL_INFO, 
                  "Discarding Msg Type %d messages (RDA Status: %d;  Radial Status: %d)",
                  msg_data.msg_hdr.type, msg_data.rda_status, 
                  msg_data.radial_status);
            last_msg_discarded = msg_data.msg_hdr.type;
         }

         if (msg_buf != NULL) {
            free (msg_buf); 
            msg_buf = NULL;
         }
         continue;
      }

      if (msg_data.radial_status == BEG_VOL) {

         MM_clear_metadata_buf ();
         MM_write_metadata (&msg_data);

      }

      if (msg_buf != NULL) {
         free (msg_buf); 
         msg_buf = NULL;
      }

      previous_rda_status = msg_data.rda_status;
   }

   return;
}


/********************************************************************************

    Description: Task abort handler

          Input: *msg - msg string to print to the task log

         Output: None

         Return: None

********************************************************************************/
void MA_Abort_Task (char *msg) {

   LE_send_msg (GL_INFO,"%s...Aborting Execution", msg);
   ORPGTASK_exit (GL_EXIT_FAILURE);

}

/***************************************************************************
  
   Description: Print the usage message to the screen and exit

   Input: argv - command line arguments.

   Output: 

   Return: 
  
**************************************************************************/
void Print_usage (char **argv) {

   printf ("Usage: %s (options)\n", argv[0]);
   printf ("       Options:\n");
   printf ("       -n Nyquist Velocity (default: No Change)\n" );
   printf ("       -s site ICAO (default: from site_info.dea)\n" );
   printf ("       -i Input Data ID (default: 76)\n" );
   printf ("       -h help\n");
   exit (0);

}

/********************************************************************************

    Description: Read command line options

    Input: argc - number of command line options.
           argv - the command line options.

    Return: 0 on success; -1 on error

********************************************************************************/
int Read_options (int argc, char **argv) {

   extern char *optarg;    /* used by getopt */
   extern int optind;      /* used by getopt */
   int c;
   int ret = 0;

   /* Process command line arguments. */
   while ((c = getopt (argc, argv, "n:s:i:h?")) != EOF) {

      switch (c) {

         case 'n':
            NYQUIST_VEL = atof(optarg);
            if( (NYQUIST_VEL < 0.0) || (NYQUIST_VEL > 36.0) )
               NYQUIST_VEL = 0.0;
            break;

         case 's':
            strncpy( &ICAO[0], optarg, 4 );
            ICAO[4] = '\0';
            break;

         case 'i':
            Input_data_id = atoi(optarg);
            if( (Input_data_id < 0) || (Input_data_id > 3000) )
               Input_data_id = CLDM_SR_BASEDATA;
            break;

         case 'h':
         case '?':
            Print_usage (argv);
            break;

      }

   }

   LE_send_msg( GL_INFO, "Nyquist Velocity: %f\n", NYQUIST_VEL );
   LE_send_msg( GL_INFO, "Input Data ID:    %d\n", Input_data_id );

   return (ret);
}

/********************************************************************************

    Description: Task termination handler

          Input: signal   - the signal causing the termination
                 exitcode - the exit code of the handler

         Output: None

         Return: None

           Note: 

 ********************************************************************************/

int Termination_Handler (int signal, int exit_code) {

  time_t current_time = time(NULL);

  LE_send_msg (GL_INFO,"Task terminated by signal %d (%s) @ %s\n",
               signal, ORPGTASK_get_sig_name( signal),
               asctime (gmtime(&current_time)));
  return 0;
}

/***************************************************************************
   
   Description: RDA Status msg Event handler

         Input: input arguments - see the LB notification documentation
                Input_data_id   - Data ID of the input data source (global var)

        Output:

        Return: 

         Note:
  
**************************************************************************/
static void Rda_status_updated (EN_id_t event, char *msg, int msg_size, void *arg) {

   if ((msg != NULL) && msg_size == sizeof(int)) {

      if( (int) (*msg) != 0 )
         RM_set_rda_status_updated_flag ();

   }
   return;
}

