/* 
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2014/09/02 20:44:10 $
 * $Id: cldm_main.c,v 1.16 2014/09/02 20:44:10 garyg Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 *
 */  

/* cldm.c - This file is the main file for the convert_ldm binary. The purpose of
            convert_ldm is to read Level II data from the basedata stream (real-time
            stream or the recombined stream) and re-package the data into LDM
            format. The source of the input data is determined by the LDM
            version number which is managed by the ORPGMISC library

   The primary functions of convert_ldm are:
     1. Read Base Data from the real-time datastream LB, or the Recombined LB
     2. Reformat the data, if needed, from RPG format to RDA format
     3. Compress and write to ORPGDAT_LDM_WRITER_INPUT LB, or to the local disk 
        file if the "record" mode flag is set.
        Note: Local mode has precedence over LDM mode
*/


#include "cldm.h"
#include "archive_II.h"
#include <orpggdr.h>
#include <unistd.h>
#include <ctype.h>

   /* macro definitions */

   /* Pending Archive II transmit status commands */

#define NO_PENDING_XMIT_STATUS_CHANGE -1
#define A2_XMIT_STATUS_PENDING_OFF     1
#define A2_XMIT_STATUS_PENDING_ON      2


   /* Level II statistics structure */

typedef struct Level_2_stats {
  int    total_bytes_c;      /* # of compressed bytes sent to LDM this vol scan */
  int    total_bytes_u;      /* # of uncompressed bytes sent to LDM this vol scan */
  time_t volume_start_time;  /* Volume scan start time (from MISC_systime()) */
  time_t volume_end_time;    /* Volume scan end time (from MISC_systime()) */
  int    vcp;                /* Volume Coverage Pattern in use. */
  float  last_elevation;     /* Last elevation cut, in deg. */
  int    ldm_version;        /* LDM Version in use */
} LDM_vcp_stats_t;

typedef struct {
   int current_status;       /* current transmit status */
   int pending_status;       /* pending transmit status change command */
} A2_xmit_status;


   /* Global Variables. */

static LDM_vcp_stats_t LDM_vcp_stats;     /* LDM statistics on a per VCP basis */
static A2_xmit_status  Xmit_status;       /* Archive II transmit status */

static int   LocalMode = 0;               /* Local Mode flag */
static int   RecordMode = 0;              /* Record Mode flag */
static int   Archive_II_cmd_received = 0; /* Flag specifiying Arch 2 cmd was rec'd */
static char  ICAO [5];                    /* site ICAO/id */
static int   RDA_channel = -1;            /* Legacy or Open RDA channel flag */
static int   Input_data_id = -1;          /* Data ID (LB) to read BaseData from */
static int   Realtime_data_id;            /* Data ID of the real-time datastream */
static int   Recombined_DP_data_id;       /* Data ID of the Dual Pol Recombined 
                                             datastream */
static int   Recombined_DP_removed_data_id;/* Data ID of the Recombined datastream 
                                              with Dual Pol removed */
   /* Function Prototypes. */

static void  Arch_II_cmd_recvd (int fd, LB_id_t msgid, int msg_info, void *arg);
static void  Clear_ldm_stats (void);
static void  Print_usage (char **argv);
static void  Print_ldm_stats (void);
static void  Process_archive_II_command (void);
static void  Read_A2_data (void);
static int   Read_options (int argc, char *argv[]);
static void  Recombine_rda_status_updated (EN_id_t event, char *msg, int msg_size, 
                                           void *arg);
static void  Select_input_source (int current_ldm_version);
static void  Start_level_II_transmission (void);
static void  Stop_level_II_transmission (void);
static int   Termination_Handler (int signal, int exit_code);


/********************************************************************************

  Description: This is the main routine for convert_ldm.  It performs 
               initialization and event/LB notifications, then calls the 
               runtime loop Read_A2_data ().

        Input: None

       Output: None

       Return: None

        Notes: None

 *********************************************************************************/

int main (int argc, char *argv[]) {

   int ret;

   Xmit_status.pending_status = NO_PENDING_XMIT_STATUS_CHANGE;

      /* read options */
#ifdef LEVEL2_TOOL
   LocalMode = 1;       
   RecordMode = 1;     
#endif

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

      /* Print the status of the command line options */

   LE_send_msg (GL_INFO, "LocalMode: %d, RecordMode: %d", 
                LocalMode, RecordMode);

   ORPGMISC_deau_init();

      /* initialize RPG parameters */

   if (INIT_RPG_parameters (ICAO, &RDA_channel) < 0)
      ORPGTASK_exit (GL_EXIT_FAILURE);

      /* initialize the input data LBs */

   ret = INIT_input_lbs (&Recombined_DP_data_id, &Recombined_DP_removed_data_id,
                         &Realtime_data_id);

   if (ret < 0) {
      LE_send_msg (GL_ERROR, "Error opening the LDM input LBs (ret: %d)", ret);
      ORPGTASK_exit (GL_EXIT_FAILURE);
   }

      /* initialize the Archive II transmit status */

   if (INIT_archive_status(&Xmit_status.current_status) < 0) {
      LE_send_msg (GL_ERROR, "Error initializing Archive II cmd LB");
      ORPGTASK_exit (GL_EXIT_FAILURE);
   }

      /* register for Recombined ORPGEVT_RDA_STATUS_CHANGE events */

   EN_register (ORPGEVT_RDA_STATUS_CHANGE, Recombine_rda_status_updated);

      /* Register for Archive II command events */

   if ((ret = ORPGDA_UN_register (ORPGDAT_ARCHIVE_II_INFO, ARCHIVE_II_COMMAND_ID, 
                                  Arch_II_cmd_recvd)) < 0) {
      LE_send_msg (GL_ERROR, "ORPGDAT_ARCHIVE_II_INFO registration Failed (%d)", ret);
      ORPGTASK_exit (GL_EXIT_FAILURE);
   }

      /* Initialize Level II LDM statistics and record information */

   memset (&LDM_vcp_stats, 0, sizeof( LDM_vcp_stats_t));
   WR_init_record_info (ICAO);

      /* Tell RPG manager we're ready */

   ORPGMGR_report_ready_for_operation();

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
   int   bytes_read;
   int   ldm_version;
   int   min_msg_size;
   char  *msg_buf = NULL;
   int   last_msg_discarded = -1;
   int   previous_ldm_version = -1;
   short previous_rda_status = -1;
   msg_data_t msg_data;

      /* initialize variables before entering the runtime loop */

   ldm_version = ORPGMISC_get_LDM_version ();
   Select_input_source (ldm_version);
   previous_ldm_version = ldm_version;
   msg_data.rda_config = ORPGRDA_get_rda_config (NULL);
   msg_data.radial_status = END_VOL;

   if ((msg_data.rda_status = ORPGRDA_get_status (RS_RDA_STATUS)) < 0)
      msg_data.rda_status = RS_STANDBY;

      /* main runtime loop */

   while (1) {

         /* Check for pending commands. */

      if (Archive_II_cmd_received) {
         Archive_II_cmd_received = 0;
         Process_archive_II_command ();
      }

         /* if Archive II Xmit Status is "Off", set the control variables 
            and loop here until we're commanded to Xmit data */

      if ((Xmit_status.current_status == ARCHIVE_II_TRANSMIT_OFF)  &&
          (Xmit_status.pending_status != A2_XMIT_STATUS_PENDING_ON)) {
         ldm_version = ORPGMISC_get_LDM_version ();
         msg_data.radial_status = END_VOL;
         sleep(1);
         continue;
      }

         /* select the input data source if the LDM versions changed */

      if (previous_ldm_version != ldm_version) {
         LE_send_msg (GL_INFO, "LDM version # changed (previous: %d;  current: %d)",
                      previous_ldm_version, ldm_version);
         Select_input_source (ldm_version);
         previous_ldm_version = ldm_version;
      }

         /* Read the next msg */

      bytes_read = RM_read_next_msg (&msg_buf, Input_data_id, RDA_channel); 

         /* Process any read errors */

      if (bytes_read < 0) {

         if (bytes_read == LB_TO_COME) {
            sleep (1); /* wait for new data to arrive */
         } else if (bytes_read == LB_EXPIRED) {
            LE_send_msg (GL_ERROR, 
                         "Messages Expired .... Skip to first unread message");
            ORPGDA_seek (Input_data_id, 0, LB_LATEST, NULL);
         } else
            LE_send_msg (GL_ERROR, 
                        "ORPGDA_read() Failed: data_id: %d; read err: %d",
                        Input_data_id, bytes_read);

            /* Keep close watch on the LDM version number selected while
               at EOV - we want to catch any last second status change 
               before starting the next volume scan */

         if (msg_data.radial_status == END_VOL) 
            ldm_version = ORPGMISC_get_LDM_version ();

         continue;

      } 

         /* Determine the offset to the msg header */

      RM_determine_msg_hdr_offset (msg_buf, &header_offset, &comm_mgr_type,
                                   Input_data_id);

         /* Discard any msgs that are just comm manager messages, or any data 
            messages that are less than the minimum message length allowed */

      min_msg_size = header_offset + sizeof (RDA_RPG_message_header_t);
      
      if ((comm_mgr_type != CM_DATA) || (bytes_read < min_msg_size)) {

          if ((comm_mgr_type == CM_DATA) && (bytes_read < min_msg_size))
            LE_send_msg (GL_ERROR, 
               "Msg discarded: Msg size (%d bytes) < min_msg_size (%d bytes); (Data_id: %d)",
               bytes_read, min_msg_size, Input_data_id);

          free (msg_buf);
          continue;

      } else if (bytes_read > MAXLEN_MESSAGE_LEN) {
          LE_send_msg (GL_ERROR, 
             "Msg size (%d) is > MAXLEN_SIZE allowed (%d)...clipping msg)",
             bytes_read, MAXLEN_MESSAGE_LEN);
          bytes_read = MAXLEN_MESSAGE_LEN;
      }

         /* Get various message properties we need to process the message */

      RM_extract_msg_data (msg_buf + header_offset, &msg_data, previous_rda_status); 

         /* if "Start transmission" cmd is pending, wait until the 
            BOV to begin processing data */

      if (Xmit_status.pending_status == A2_XMIT_STATUS_PENDING_ON) {
         if (msg_data.radial_status == BEG_VOL)
            Start_level_II_transmission ();
         else {
            free (msg_buf); 
            continue;
         }
      }

         /* Process the message based on the LDM perspective of message type */

      if (msg_data.ldm_msg_type == CLDM_RADIAL_DATA) {

            /* Add the msg to the radial buffer */

         WR_add_msg_to_radial_buffer (msg_buf + header_offset, &msg_data,
                                      ldm_version, previous_rda_status);

            /* Set the current vcp, elevation angle and volume scan end
               time in the LDM stats. Do this for every radial in the event 
               the volume scan terminates prematurely. */

         if (msg_data.radial_status != BEG_VOL) {         
            LDM_vcp_stats.vcp = msg_data.elev_status.vcp_num;
            LDM_vcp_stats.last_elevation = msg_data.elev_status.elev;
            LDM_vcp_stats.volume_end_time = MISC_systime(NULL);
         }
            /* Process the Metadata msg */

      } else if (msg_data.ldm_msg_type == CLDM_META_DATA) 
         MM_process_metadata_msg (msg_buf + header_offset, msg_data, ldm_version);
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

         /* If "Stop Xmission" is pending, perform house cleaning duties 
            before stopping transmission */

      if (((msg_data.radial_status == BEG_VOL) ||
         (msg_data.radial_status == END_VOL)   || 
         (msg_data.rda_status != RS_OPERATE))  &&
         (Xmit_status.pending_status == A2_XMIT_STATUS_PENDING_OFF)) {

          Print_ldm_stats ();
          Clear_ldm_stats ();
          Stop_level_II_transmission ();
          MM_clear_metadata_buf ();
          msg_data.radial_status = END_VOL;

         /* On the first radial of the beginning of each volume scan,
            write all the LDM information required that is part of the first 
            record of the LDM file (i.e. volume header, meta data, etc.) 
            and initialize LDM stats variables for this VCP */

      } else if (msg_data.radial_status == BEG_VOL) {

         Clear_ldm_stats ();
         LDM_vcp_stats.volume_start_time = MISC_systime (NULL);

         if (Input_data_id != Realtime_data_id)
            MM_clear_metadata_buf ();

         MM_write_metadata (&msg_data, ldm_version);
         LDM_vcp_stats.ldm_version = ldm_version;

         /* Perform housekeeping duties inbetween volume scans and
            when the RDA is not in operate */

      } else if (msg_data.radial_status == END_VOL) {

            /* Write the LDM stats to the RPG Status Log only
               on complete volume scans */

         if ((msg_data.rda_status == RS_OPERATE)  &&
             (previous_rda_status == RS_OPERATE)) {
               Print_ldm_stats (); 
               Clear_ldm_stats ();
         }

            /* Get the current LDM Version number */

         ldm_version =  ORPGMISC_get_LDM_version ();

         if (ldm_version == LDM_VERSION_UNKN) {
            MA_Abort_Task ("LDM version is unknown....task aborting");
         }
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


/********************************************************************************

    Description: Get the current operating modes 

          Input: LocalMode  - State of the Local Mode flag (global var) 
                 RecordMode - State of the Record Mode flag (global var)
                 Xmit_status.current_status - the LDM transmission status (global var)

         Output: local      - value of LocalMode
                 record     - value of RecordMode
                 ldm_status - 1 if Level II transmit status is "on";
                              0 if the transmit status is "off"

         Return: None

           Note: Global variables are defined at the beginning of this file

 ********************************************************************************/

 void MA_get_level2_modes (int *local, int *record, int *ldm_status) {

   *local = LocalMode;
   *record = RecordMode;

   if (Xmit_status.current_status == ARCHIVE_II_TRANSMIT_ON)
      *ldm_status = 1;
   else
      *ldm_status = 0;

   return;
}


/********************************************************************************

    Description: Update the LDM_vcp_stats compressed and uncompressed byte
                 counters

          Input: compressed_write_len   - the compressed byte length from the
                                          last packet write
                 uncompressed_write_len - the uncompressed byte length from the
                                          last packet write

         Output: LDM_vcp_stats.total_bytes_c - the total compressed byte count for 
                                               this VCP
                 LDM_vcp_stats.total_bytes_u - the total uncompressed byte count for 
                                               this VCP

         Return: None

           Note: LDM_vcp_stats is a global structure defined at the beginning of 
                 this file
   
 ********************************************************************************/

void MA_update_ldm_stats_cntrs (int uncompressed_write_len, 
                                int compressed_write_len) {

   LDM_vcp_stats.total_bytes_c += compressed_write_len;
   LDM_vcp_stats.total_bytes_u += uncompressed_write_len;

   return; 
}


/********************************************************************************

    Description:  The callback routine for Archive II commands
                 
          Input: fd    - LB registered for the notification
                 msgid - id of the msg registered for the notification
                 msg_info & *args - see LB notification documentation for more info

         Output: Archive_II_cmd_received - flag denoting an Archive II cmd has been
                 received

         Return: 0 on success; -1 on error
   
 ********************************************************************************/

void Arch_II_cmd_recvd (int fd, LB_id_t msgid, int msg_info, void *arg) {

   LE_send_msg( GL_INFO, "Archive II Command LB Updated\n" );
   Archive_II_cmd_received = 1;

}


/***************************************************************************
  
   Description: Clear the LDM statistics buffer

         Input: 

        Output: 

        Return: 
  
 **************************************************************************/

void  Clear_ldm_stats (void) {

   memset (&LDM_vcp_stats, 0, sizeof (LDM_vcp_stats_t));
   return;
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
   printf ("       -l Local Mode (Default: LDM Mode)\n");
   printf ("       -r Record Mode (Default: Not Recording)\n");
   printf ("       -h help\n");
   exit (0);
}


/***************************************************************************
  
   Description: Print the LDM statistics

         Input: 

        Output: 

        Return: 
  
 **************************************************************************/

void Print_ldm_stats (void) {

   if ((LDM_vcp_stats.total_bytes_c != 0) && (LDM_vcp_stats.volume_start_time != 0)) { 
      LE_send_msg (GL_STATUS | LE_RPG_INFO_MSG,
                   "LDM Stats: Ver: %d, VCP: %3d, Last Elev: %4.1f deg, Dur: %3d s, Bytes C: %-8d, U: %-8d\n",
                   LDM_vcp_stats.ldm_version, 
                   LDM_vcp_stats.vcp, LDM_vcp_stats.last_elevation,
                   LDM_vcp_stats.volume_end_time - LDM_vcp_stats.volume_start_time,
                   LDM_vcp_stats.total_bytes_c, LDM_vcp_stats.total_bytes_u);
   }
   return;
}


/***************************************************************************
  
   Description: Read the Archive II command sent by the operator

         Input: ORPGDAT_ARCHIVE_II_INFO    - Archive II command LB
                ARCHIVE_II_COMMAND_ID      - Archive II command msg id

        Output: Xmit_status.current_status - current Archive II transmit status
                Xmit_status.pending_status - pending commands that can not
                                             be serviced now
                ORPGDAT_ARCHIVE_II_INFO    - Archive II command LB
                ARCHIVE_II_FLOW_ID         - Archive II transmit status msg id

        Return: 

         Notes: Xmit_status is a global structure defined at the beginning 
                of this file
  
 **************************************************************************/

static void Process_archive_II_command (void) {

   int ret;
   int update_status = 0;
   ArchII_command_t a2_cmd;
   time_t current_time = time (NULL);

      /* Read latest archive II command */

   ret = ORPGDA_read (ORPGDAT_ARCHIVE_II_INFO, &a2_cmd, sizeof (ArchII_command_t),
                      ARCHIVE_II_COMMAND_ID);

      /* check error conditions */

   if (ret <= 0) {
      if (ret != LB_TO_COME)
         LE_send_msg (GL_ERROR,
            "Error reading ORPGDAT_ARCHIVE_II_INFO LB (err: %d)\n", ret);
      return;
   }

      /* Process the archive II command */

   switch (a2_cmd.command) {

      case ARCHIVE_II_NEED_TO_START:

         if (Xmit_status.current_status != ARCHIVE_II_TRANSMIT_ON) {
            Xmit_status.pending_status = A2_XMIT_STATUS_PENDING_ON;
            LE_send_msg (GL_INFO, "Cmd received to start LDM transmission");

         }   
         break;

      case ARCHIVE_II_NEED_TO_STOP:
         Xmit_status.pending_status = A2_XMIT_STATUS_PENDING_OFF;
         LE_send_msg (GL_INFO, "Cmd received to stop LDM transmission");
         break;

      case ARCHIVE_II_LOCAL_MODE:
         LE_send_msg( GL_INFO, "Cmd LDM to Local Mode" );
         LocalMode = 1;
         break;

      case ARCHIVE_II_LDM_MODE:
         LE_send_msg( GL_INFO, "Cmd LDM to LDM Mode\n" );
         LocalMode = 0;
         break;

      case ARCHIVE_II_RECORD_MODE:
         LE_send_msg( GL_INFO, "Cmd LDM to Record Mode\n" );
         RecordMode = 1;
         break;

      case ARCHIVE_II_NO_RECORD_MODE:
         LE_send_msg( GL_INFO, "Cmd LDM to turn off Record Mode\n" );
         RecordMode = 0;
         break;

      case ARCHIVE_II_NORMAL_MODE:

         LE_send_msg( GL_INFO, "Cmd LDM to Normal Mode\n" );

         if (RecordMode)
            LE_send_msg( GL_INFO, "--->Going to No Record Mode\n" );

         if (LocalMode)
            LE_send_msg( GL_INFO, "--->Going to LDM Mode\n" );

         RecordMode = 0;
         LocalMode = 0;
         break;

      default:
         LE_send_msg (GL_ERROR, "Invalid Archive II cmd received (cmd: %d)",
                      a2_cmd.command);
         return;
      break;   

   }

      /* Write the transmission status if required */

   if (update_status) {
      ArchII_transmit_status_t transmit_status;

      transmit_status.ctime = current_time;
      transmit_status.status = Xmit_status.current_status;

       ret = ORPGDA_write (ORPGDAT_ARCHIVE_II_INFO, (char *) &transmit_status,
                           sizeof (ArchII_transmit_status_t), ARCHIVE_II_FLOW_ID);

      if (ret < 0) {
         LE_send_msg (GL_ERROR, 
               " Failure writing ARCHIVE_II_FLOW_ID msg (err: %d)", ret);
         return;
      }
      else
         LE_send_msg (GL_INFO, "ARCHIVE_II Transmission Status updated");
   }
   return;
}


/***************************************************************************
  
   Description: Recombined RDA Status msg Event handler

         Input: input arguments - see the LB notification documentation
                Input_data_id   - Data ID of the input data source (global var)

        Output:

        Return: 

         Note:
  
 **************************************************************************/

static void Recombine_rda_status_updated (EN_id_t event, char *msg, int msg_size, 
                                          void *arg) {

   if ((msg != NULL) && msg_size == sizeof(int)) {
      if( (int) (*msg) != 0 )
         if ((Input_data_id == Recombined_DP_data_id)  ||
             (Input_data_id == Recombined_DP_removed_data_id))
             RM_set_recomb_rda_status_updated_flag ();
   }
   return;
}


/********************************************************************************

    Description: Read command line options

          Input: argc - number of command line options.
                 argv - the command line options.

         Output: LocalMode  - the local mode flag
                 RecordMode - the record mode flag

         Return: 0 on success; -1 on error

           Note: LocalMode is a global variable defined at the beginning of this file
                 RecordMode is a global variable defined at the beginning of this file

 ********************************************************************************/

int Read_options (int argc, char **argv) {

   extern char *optarg;    /* used by getopt */
   extern int optind;      /* used by getopt */
   int c;
   int ret = 0;

      /* Process command line arguments. */

   while ((c = getopt (argc, argv, "lrh?")) != EOF) {

      switch (c) {

         case 'l':
            LocalMode = 1;
            break;

         case 'r':
            RecordMode = 1;
            break;

         case 'h':
         case '?':
            Print_usage (argv);
            break;

      }
   }
   return (ret);
}


/********************************************************************************

    Description: Determine the input source (real-time or recombined) based on
                 the current LDM version number
                 
          Input: current_ldm_version - the curernt LDM version number

         Output: Input_data_id - the data id of the LB to read from

         Return: None
   
 ********************************************************************************/

static void Select_input_source (int current_ldm_version) {

   static int previous_input_source = -1;

   switch (current_ldm_version) {
      case 1:  /* pre-DP: Legacy msg 1 messages */
      case 2:  /* pre-DP: SR disabled at RDA */
      case 3:  /* pre-DP: SR enabled at RDA and fro LDM */
      case 5:  /* DP: SR disabled at RDA */
      case 6:  /* DP: SR and DP enabled at RDA and for LDM */
         Input_data_id = Realtime_data_id;
      break;

      case 4:  /* pre-DP: SR enabled at RDA and disabled for LDM */
               /* DP: DP removed and additional recombine applied */
         Input_data_id = Recombined_DP_removed_data_id;
      break;

      case 7:  /* DP: Recombined DP */
         Input_data_id = Recombined_DP_data_id;
      break;

      default:   /* default to Dual Pol recombined */
         Input_data_id = Recombined_DP_data_id;
      break;
   }

      /* set read pointer to the end of the LB queue if the input LB changed */

   if (previous_input_source != Input_data_id)
      ORPGDA_seek (Input_data_id, 0, LB_LATEST, NULL);

      /* clear the Metadata buffer if the input data source changes */

   if (Input_data_id != previous_input_source)
      MM_clear_metadata_buf ();

   previous_input_source = Input_data_id;

   LE_send_msg (GL_INFO, "LDM version read: %d (input lb selected: %d)", 
                current_ldm_version, Input_data_id);

   return;
}


/********************************************************************************

  Description: Start the Level II data transmission

        Input: Xmit_status.pending_status - the command that is pending

       Output: Xmit_status.current_status - the current transmission status

       Return: None

        Notes: Xmit_status is a global structure defined at the top of this file

 *********************************************************************************/

static void Start_level_II_transmission (void) {

   int ret;
   ArchII_transmit_status_t xmit_stat;

   Xmit_status.pending_status = NO_PENDING_XMIT_STATUS_CHANGE;
   Xmit_status.current_status = ARCHIVE_II_TRANSMIT_ON;

   xmit_stat.ctime = time (NULL);
   xmit_stat.status = Xmit_status.current_status;

   ret = ORPGDA_write (ORPGDAT_ARCHIVE_II_INFO, (char *) &xmit_stat,
                       sizeof (ArchII_transmit_status_t), ARCHIVE_II_FLOW_ID);

   if (ret < 0)
      LE_send_msg (GL_ERROR, 
                  " Failure writing ARCHIVE_II_FLOW_ID msg (err: %d)", ret);
   else
      LE_send_msg (GL_INFO, "Starting LDM transmission");

   return;
}


/********************************************************************************

  Description: Stop the Level II data transmission

        Input: Xmit_status.pending_status - the command that is pending

       Output: Xmit_status.current_status - the current transmission status

       Return: None

        Notes: Xmit_status is a global structure defined at the top of this file

 *********************************************************************************/

static void Stop_level_II_transmission (void) {

   int ret;
   ArchII_transmit_status_t xmit_stat;

   Xmit_status.pending_status = NO_PENDING_XMIT_STATUS_CHANGE;
   Xmit_status.current_status = ARCHIVE_II_TRANSMIT_OFF;

   xmit_stat.ctime = time (NULL);
   xmit_stat.status = Xmit_status.current_status;

   ret = ORPGDA_write (ORPGDAT_ARCHIVE_II_INFO, (char *) &xmit_stat,
                       sizeof (ArchII_transmit_status_t), ARCHIVE_II_FLOW_ID);

   if (ret < 0)
      LE_send_msg (GL_ERROR, 
                  " Failure writing ARCHIVE_II_FLOW_ID msg (err: %d)", ret);
   else
      LE_send_msg (GL_INFO, "Stopping LDM transmission");

   return;
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
