/*******************************************************************

	Main module for pre-processing the RDA base data and creating 
	the RPG base data.

*******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/06/05 20:43:02 $
 * $Id: pbd.c,v 1.2 2013/06/05 20:43:02 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */  

#define GLOBAL_DEFINED
#include <pbd.h>

#define INPUT_DATA_SIZE	        25000   /* Maximum size of RDA radial, in bytes */
#define CTM_HEADER_SIZE            12  	/* Size of CTM header, in bytes */

#define MAX_RPG_RADIAL_SIZE     16920   /* This size is based on the following 
                                           assumptions:

                                           Radial Header - (sizeof(Base_data_header))
                                           8 possible data moments, 3 standard + 5 additional
                                           2 bytes/datum for REF, VEL, SPW, PHI and RHO
                                           1 byte/datum for ZDR, SNR and RFR
                                           460 km range, 250 m gates for SNR and REF = 1840 bins
                                           300 km range, 250 m gates for ZDR, PHI, RHO, VEL and SW = 1200 bins
                                           60 km range, 250 m gates for 240 bins
                                           5 additional data headers - (sizeof(Generic_moment_t)) = 32 bytes

                                             200 - Radial header 
                                           16560 - moment data 
                                             160 - additional headers
                                           -----
                                           16920

                                        */

#define MAX_WAIT_TIME		5   	/* Amount of time to wait for routine product
                                   	   scheduler to post event indicating the 
                                           generation tables are set.  At start of 
                                           volume scan, pbd will delay RPG radials
                                           up to MAX_WAIT_TIME seconds to allow the
					   generation tables to be set. */

#define NOT_INIT                -1.e20f /* Uninitialized float point value .  Used for 
                                           SNR thresholding. */


static int Generation_tables_set;	/* Flag, when set, indicates product generation
                                           has completed set-up of master generation
                                           list of products to be generated. */

static int Rda_control_command_received; /* Flag, when set, indicates an RDA command
                                            command was received. */

static int Rda_control_command_lbfd;	/* File descriptor of the RDA Control Command LB. */

static int Volume_status_lbfd;          /* LB fd for the the ORPGDAT_GSM_DATA. */

static int PBD_pid;                     /* Process ID.  Used in conjunction with LB
                                           notification. */

static int Reset_inactivity_alarm = 0; 	/* Reset RDA inactivity alarm flag.  This alarm
       					   is activated when no messages have been received
					   from the RDA for PBD_INACTIVITY_ALARM_INTERVAL. */

static int Reset_inbuf_check_alarm = 0; /* Reset RDA input buffer check alarm flag. */

static int Inactvty_alarm_reg = 0; 	/* Indicates whether the RDA inactivity
                			   alarm (i.e., timer) has been registered. */ 

static int Inbuf_check_alarm_reg = 0; 	/* Indicates whether the RDA Input Buffer
                			   check alarm (i.e., timer) has been registered. */ 

static int Log_file_nmsgs = 1000;	/* Default size, in number of message, for Log 
                                           Error (LE) file. */

static Vol_stat_gsm_t Vs_gsm;           /* Volume Status Data. */

static int RDA_status = RS_STANDBY;     /* Set the default RDA Status to Standby. */

#ifdef SNR_TEST

#define SENSITIVITY_LOSS_RESP0 		412345
#define MINIMUM_REFLECTIVITY		 -33.0

static double Syscal = NOT_INIT;        /* System Gain Calibration constant.  Used for
                                           SNR thresholding. */

static double Atmos = NOT_INIT;         /* Atmospheric Attenuation Rate.  Used for
                                           SNR thresholding. */

static double Noise = NOT_INIT;         /* Horizontal channel Noise value.   Theoretically
                                           this value can change every radial. */

static double *Range_corr = NULL;       /* Range correction lookup table, 0.25 km resolution. 
                                           Used for SNR thresholding. */  

static double *Range_corr_1km = NULL;   /* Range correction lookup table, 1 km resolution. 
                                           Used for SNR thresholding. */  

static unsigned char *Message_31 = NULL; /* Buffer to hold Message 31 data to support 
                                           Level II recording. */

#endif

/* Local Function Prototypes. */
static int Process_a_radial( int msg_type, char *rda_msg, char *rpg_basedata );
static int Read_options( int argc, char **argv );
static int Check_rda_commands();
static void Commanded_restart( int parameter_1 );
static void Init_events();
static void Init_lb_notification();
static void Init_alarms();
static void Init_vsnum_wxmode( );
static void Check_input_buffer_load( int data_id, LB_id_t msg_id, int read_returned );
static int Cleanup_fxn( int signal, int status );
static void Open_lb();
static int Update_current_vcp_table( int vcp_num, Vcp_struct *vcp );
static char* Legacy_to_generic( char *rda_msg );
#ifdef SNR_TEST
static void SNR_build_lookup_tables();
static void SNR_build_range_corr_table( Base_data_header *rpg_hd );
static void SNR_apply_snr_thresholds( char *rpg_basedata );
static int  SNR_convert_to_msg31( char *buf, unsigned char *msg31_buf );
static void SNR_apply_speckle_filter( unsigned short *sref, int start_bin, 
                                      int end_bin );
static void SNR_1km_reflectivity( Base_data_header *bhd, unsigned short *ref,
                                  unsigned short *vel, double tz );
#endif

void Event_handler( EN_id_t evtcd, char *msg, int msglen, void *arg );
void Lb_notify_handler( int fd, LB_id_t msgid, int msg_info, void *arg );
void Alarm_handler( malrm_id_t alarm_id );


/*******************************************************************

   Description:   
      This is the main function responsible for initialization and 
      processing control.  
                  
      Initialization includes command line parsing, Log Event 
      service registration, event registration, alarm registration, 
      process termination handler and the initial read of scan 
      summary data if the RPG is restarting.  

      Processing control includes reading rda messages from the 
      wideband communication's managers response LB and processing 
      radial and status messages.  The radial messages are converted 
      into RPG format radial messages.  These messages are then written 
      to output PBD_radial_out_LB.  

      Processing control also involves radial validation checks.  
      This ensures a smooth sequence of radial messages passed 
      downstream.

      RDA commands via the RDA_COMMANDS LB are intercepted.  Those 
      involving elevation or volume restarts are processed first (i.e.,
      prior to) control_rda.  All other control commands are ignored.

   Input:         
      argc - number of command line arguments.
      argv - command line arguments.

   Returns:       
      There is no return value defined for this funtion.  Always 
      returns 0.
 
********************************************************************/
int main( int argc, char *argv[] ){

    static char *rda_msg = NULL; 
    static char *rpg_basedata = NULL;
    static int offset = CTM_HEADER_SIZE + (int) sizeof( CM_resp_struct );
  
    int retval;
    Siteadp_adpt_t site;

    /* Read command line options.  Task exits on failure. */
    if (Read_options (argc, argv) != 0)
	ORPGTASK_exit (GL_EXIT_FAILURE);

    /* Initialize Log-Error services. */
    if( ORPGMISC_init( argc, argv, Log_file_nmsgs, 0, -1, 0 ) < 0 ){

       LE_send_msg( GL_INFO, "ORPGMISC_init Failed\n" );
       ORPGTASK_exit(GL_EXIT_FAILURE);

    }

    /* Register termination handler. */
    ORPGTASK_reg_term_handler( Cleanup_fxn );

    /* Get my PID */
    PBD_pid = getpid();

    /* Initialize the radar data read buffer.  Exit on malloc failure. */
    rda_msg = (char *) malloc( INPUT_DATA_SIZE );
    if( rda_msg == NULL ){

       LE_send_msg( GL_INFO, "malloc Failed For %d Bytes\n", INPUT_DATA_SIZE );
       ORPGTASK_exit(GL_EXIT_FAILURE);

    }

    /* Initialize rpg format output buffer.  Exit on malloc failure. */
    rpg_basedata = (char *) calloc( 1, MAX_RPG_RADIAL_SIZE );
    if( rpg_basedata == NULL ){

       LE_send_msg( GL_INFO, "calloc Failed For %d Bytes\n", MAX_RPG_RADIAL_SIZE );
       ORPGTASK_exit(GL_EXIT_FAILURE);

    }
 
    /* Initialize flags (start of volume required, start of elevation
       not required, elevation and volume restarts not commanded,
       and aborted volume scan number). */
    PBD_start_volume_required = (PBD_DONT_SEND_RADIAL | 1);
    PBD_start_elevation_required = (PBD_PROCESS_NORMAL | 0);
    PBD_elevation_restart_commanded = 0;
    PBD_volume_restart_commanded = 0;
    PBD_aborted_volume = -1;
    
    PBD_old_weather_mode = -1;
    PBD_old_vcp_number = -1;
    PBD_split_cut = 0;

    PBD_is_rda = PBD_IS_REAL_RDA;

    /* Register for events. */
    Init_events();

    /* Open all LB's */
    Open_lb();

    /* Register for LB notification (ORPGDAT_RDA_COMMANDS data ID). */
    Init_lb_notification();

    /* Register alarms. */
    Init_alarms();

#ifdef AVSET_TEST
    /* One-time building of lookup tables. */
    AVSET_compute_area_lookup();
#endif

#ifdef SNR_TEST
    /* One-time building of lookup tables for SNR thresholding. */
    SNR_build_lookup_tables();

    /* Allocate Message 31 buffer space. */
    Message_31 = (unsigned char *) calloc( 1, INPUT_DATA_SIZE );
#endif

    /* Tell RPG manager pbd is ready. */
    ORPGMGR_report_ready_for_operation();

    /* Wait for operational mode before continuing with initialization. */
    if( ORPGMGR_wait_for_op_state( (time_t) 120 ) < 0 )
       LE_send_msg( GL_ERROR, "Waiting For RPG Operational State TIMED-OUT\n" );

    else
       LE_send_msg( GL_INFO, "The RPG is Operational\n" );

    /* Check the ORPG_NONOPERATIONAL environmental variable.  If set, then
       set PBD_is_rda to false. */
    if( !ORPGMISC_is_operational() )
       PBD_is_rda = PBD_IS_PLAYBACK;

    /* Initialize PBD_weather_mode and PBD_volume_scan_number. */
    Init_vsnum_wxmode();

    /* Initialize scan summary data. */
    SSS_init_read_scan_summary();

    /* Get the radar height and convert from ft to km. */
    if( (retval = ORPGSITE_get_site_data( &site ) < 0) ){

       LE_send_msg( GL_ERROR, "ORPGSITE_get_site_data Failed (%d)\n", retval );
       exit(0);

    }

    PBD_rda_height = (short) ((float) site.rda_elev)*FT_TO_M;
    PBD_latitude = (float) site.rda_lat / 1000.0f;
    PBD_longitude = (float) site.rda_lon / 1000.0f;
    memset( PBD_radar_name, 0, PBD_RADAR_NAME_LEN );
    memcpy( PBD_radar_name, site.rpg_name, PBD_RADAR_NAME_LEN );

    /* Do Forever .... */
    while (1) {

	Base_data_header *rpg_hd;
        CM_resp_struct *resp;
        char *radar_data;
	int len, read_returned, msg_type;

	/* Do Forever .... */
	while (1) {

            /* Check if any alarms should be raised. */
            ALARM_check_for_alarm();

            /* Set the RDA inactivity alarm, if necessary. */
            if( Reset_inactivity_alarm ){

               unsigned int alarm_activated_at;
               int retval;

               alarm_activated_at = (unsigned int) (MISC_systime(NULL) + 
                                    (time_t) PBD_INACTVTY_ALARM_INTERVAL );
               if( (retval = ALARM_set( (malrm_id_t) PBD_INACTVTY_ALRM_ID,
                                        alarm_activated_at, MALRM_ONESHOT_NTVL )) < 0 )
                  LE_send_msg( GL_ERROR, "RDA Inactivity Alarm Set Failed (%d)\n",
                               retval );

               Reset_inactivity_alarm = 0;

            }

            /* Check RDA Control Command LB. */
            if( Rda_control_command_received ){

               LE_send_msg( GL_INFO, "Servicing ORPGDAT_RDA_COMMAND Updated\n" );

               /* RDA Control Command(s) received.  Clear flag and process 
                  command(s) if required. */
               Rda_control_command_received = 0;
               Check_rda_commands();

            }

            /* Has a Volume Restart command been received? */
            if( PBD_volume_restart_commanded ){

               /* A volume restart has been commanded.  Process this command now. */
               PBD_volume_restart_commanded = 0;
               PH_process_restart_command( RESTART_VOLUME );

            }

            /* Has a Elevation Restart command been received? */
            if( PBD_elevation_restart_commanded ){

               /* An elevation restart has been command.  Process this command now. */
               PBD_elevation_restart_commanded = 0;
               PH_process_restart_command( RESTART_ELEVATION );

            }

            /* Check communication manager response LB */
	    read_returned = ORPGDA_read ( PBD_response_LB, rda_msg, INPUT_DATA_SIZE, 
			                  LB_NEXT );

            /* Check if it is time to determine Input Buffer load. */
            if( Reset_inbuf_check_alarm ){

               LB_id_t msg_id = 0xffffffff;
               int retval;
               unsigned int alarm_activated_at;

               /* If last read successful, get message ID.  Otherwise, set
                  msg_id to invalid id. */
               if( read_returned > 0 )
                  msg_id = ORPGDA_get_msg_id( PBD_response_LB );

               Check_input_buffer_load( PBD_response_LB, msg_id, read_returned );
               Reset_inbuf_check_alarm = 0;

               /* Reset the timer. */
               alarm_activated_at = (unsigned int) (MISC_systime(NULL) + 
                                    (time_t) (PBD_INBUF_CHECK_ALARM_INTERVAL));
               if( (retval = ALARM_set( (malrm_id_t) PBD_INBUF_CHECK_ALRM_ID, 
                                        alarm_activated_at, MALRM_ONESHOT_NTVL )) < 0 )
                  LE_send_msg( GL_ERROR, "RDA Input Buffer Check Alarm Set Failed (%d)\n", 
                               retval );

            }

            radar_data = rda_msg;

            /* The message read must be at least as large of the Comm
               manager response structure. */
            if( read_returned >= (int) sizeof(CM_resp_struct) ){
        
               short msg_len;
               int first_word;
               RDA_RPG_message_header_t *msg_header = NULL;

               /* Reset the RDA inactivity alarm. */
               if( Inactvty_alarm_reg ){

                  PBD_time_of_last_message = MISC_systime(NULL);

                  if( PBD_rda_comm_inactivity )
                     LE_send_msg( GL_INFO, 
                                  "Comm Manager Response Inactivity Alarm CLEARED\n" );

                  PBD_rda_comm_inactivity = 0; 

               }

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
                  if( resp->type != RQ_DATA )
                     continue;

                  /* Strip off the CM_resp_struct header and the CTM header. */
                  radar_data += offset;
               } 
               else{

                  /* Assume we are not talking to a real RDA.  Playback will not
                     have a comm manager header unless coming from wideband 
                     simulator. */
                  PBD_is_rda = PBD_IS_PLAYBACK;

               }

               /* Do ICD to local host conversion of the message header.   This 
                  does conversion from Big Endian to Little Endian if host machine 
                  is Little Endian.  The RDA data is assumed Big Endian format.  
                  Also floating point number conversion (Concurrent to IEEE 754) 
                  is done. */
               UMC_RDAtoRPG_message_header_convert( radar_data );

               /* Set the RDA Configuration if the new message changes the 
                  configuration. */
               ORPGRDA_set_rda_config( (void *) radar_data );

               /* Get message type. */
               msg_header = (RDA_RPG_message_header_t *) radar_data;
               msg_type = (int) (msg_header->type);

               /* Do a simple validation of radial messages. */
               if( (msg_type == DIGITAL_RADAR_DATA) 
                             ||
                   (msg_type == GENERIC_DIGITAL_RADAR_DATA) ){

                  /* Neither radial message types are segmented.  Check these
                     fields for possible corrupted radial messages. */
                  if( (msg_header->num_segs != 1) || (msg_header->seg_num != 1) ){

                     LE_send_msg( GL_INFO, "Bad # Segments/Segment # in Radial Message\n" );
                     continue;

                  } 

                  /* Ignore the radial if the data is not coming from a real RDA and the
                     RDA is not in OPERATE. */
                  if( (PBD_is_rda == PBD_IS_REAL_RDA) && (RDA_status != RS_OPERATE) ){

                     LE_send_msg( GL_INFO, "Radial Message But Not In OPERATE.\n" );
                     continue;

                  }


                  /* Convert the message data from external format to internal Endianess. */
                  if( UMC_RDAtoRPG_message_convert_to_internal( msg_type, radar_data ) < 0 )
                     continue;

                  break;

               }
               else if( (msg_type == RDA_STATUS_DATA) 
                                  ||
                        (msg_type == RDA_RPG_VCP) 
                                  ||
                        (msg_type == LOOPBACK_TEST_RDA_RPG) ){

                  /* RDA Status and VCP messages are validated elsewhere. */
                  if( UMC_RDAtoRPG_message_convert_to_internal( msg_type, radar_data ) < 0 )
                     continue;

                  break;

               }

            }

            /* Process various LB_read errors. */

            /* If data not currently available, wait a short period then try
               reading again.  Currently we wait 1 second. */
	    if (read_returned == LB_TO_COME)
		msleep (500);

            /* If the next message in LB has expired, LB is either sized too small
               or something is delaying processing.  In either case, restart 
               volume scan and set INPUT_BUFFER_LOADSHED alarm. */
	    else if (read_returned == LB_EXPIRED){

               /* If the restart already pending, just ignore this message. 
                  Otherwise, command a volume scan restart. */
               if( !PBD_start_volume_required ){

		  LE_send_msg( GL_ERROR, 
                               "Input Radial Read Returned MESSAGE EXPIRED\n" ); 
                  PH_send_rda_control_command( (short) 1, (int) CRDA_RESTART_VCP,
                                               (int) PBD_ABORT_RADIAL_MESSAGE_EXPIRED );

               }
            
            }

            /* All other read failures are (at this point in time) fatal.  In 
               the future we may wish to have special processing for specific 
               read errors or we may wish to ignore them. */
	    else if (read_returned < 0)
		PBD_abort ("ORPGDA_read %d failed (%d)\n", PBD_response_LB, read_returned);

	} 

	/* If digital radar data, generate the RPG radial message. */
	if( (msg_type == DIGITAL_RADAR_DATA)
                      ||
            (msg_type == GENERIC_DIGITAL_RADAR_DATA) ){

           int write_returned;
           static char *dest = NULL;
           static int dest_len = 0;

           /* If this is GENERIC_DIGITAL_RADAR_DATA, check if the message is internally
              compressed.   If so, need to decompress it. NOTE: We don't normally expect
              the data to be compressed.  Therefore we malloc a buffer only when we 
              determine this to be the case. */
           if( msg_type == GENERIC_DIGITAL_RADAR_DATA ){

              Generic_basedata_t *rec = (Generic_basedata_t *) radar_data;
              int method, ret;

              if( (rec->base.compress_type == M31_BZIP2_COMP )
                              ||
                  (rec->base.compress_type == M31_ZLIB_COMP ) ){

                 int src_len = rec->msg_hdr.size;

                 if( (dest == NULL) || (dest_len < rec->base.radial_length) ){

                    /* Allocate more space than necessary.  This hopefully
                       prevents a whole lot of reallocations.  This also 
                       prevents realloc from actually freeing the memory 
                       which is the behavior when dest_len = 0. */
                    dest_len = rec->base.radial_length + 1000;
                    dest = realloc( dest, dest_len );
                    if( dest == NULL ){

                       LE_send_msg( GL_INFO, "realloc Failed for %d Bytes\n", dest_len );
                       continue;

                    }

                 }

                 if( rec->base.compress_type == M31_ZLIB_COMP )
                    method = MISC_GZIP;

                 else if( rec->base.compress_type == M31_BZIP2_COMP )
                    method = MISC_BZIP2;
                     
                 else{

                    LE_send_msg( GL_INFO, "Unknown Compression Method: %d.  Skip Radial.\n",
                                 rec->base.compress_type );
                    continue;

                 }

                 /* The data that is compressed occurs after the Generic_basedata_header_t 
                    structure. */
                 memcpy( dest, radar_data, sizeof(Generic_basedata_t) );
                 src_len -= sizeof(Generic_basedata_t);
                 ret = MISC_decompress( method, radar_data + sizeof(Generic_basedata_t), 
                                        src_len, dest + sizeof(Generic_basedata_t), 
                                        dest_len );
                 if( ret < 0 ){

                    LE_send_msg( GL_INFO, "MISC_decompress Failed: %d.  Skip Radial.\n", ret );
                    continue;

                 }
           
                 /* Set "radar_data" pointer to destination buffer. */
                 radar_data = dest;

              }

           }

           /* If error occurred (function call returns negative value), do not write 
              RPG radial message to LB. Go to top of main processing loop. */ 
           if (Process_a_radial ( msg_type, (char *) radar_data, rpg_basedata) < 0)
	      continue;

	   /* Determine the size of the radial message, in bytes. */
	   rpg_hd = (Base_data_header *) rpg_basedata;
	   len = rpg_hd->msg_len*sizeof(short);	

           /*  Make sure length is multiple of ALIGNED_LENGTH ..... otherwise FORTRAN
               algorithms which subsequently read this data (and the data is shared
               memory) may fail. */
           len = ALIGNED_T_SIZE( len ) * ALIGNED_LENGTH;
           rpg_hd->msg_len = len / sizeof(short);

           /* If length is bad, this is (currently) considered a fatal error. */
	   if( (len <= 0) || (len > MAX_RPG_RADIAL_SIZE) ) 
	      PBD_abort( "Unexpected RPG Radial Length %d (Max: %d)\n",
                         len, MAX_RPG_RADIAL_SIZE );

#ifdef AVSET_TEST
           /* Read adaptation data at start of volume scan. */
           if( (rpg_hd->status == GOODBVOL) && (rpg_hd->elev_num == 1) )
              AVSET_read_adaptation_data();

           /* Compute area for this elevation cut. */
           AVSET_compute_area( rpg_basedata );
#endif
	   /* Write the RPG radial message to the output LB.  Write errors are
              (currently) considered fatal errors.  */
	   write_returned = ORPGDA_write (PBD_radial_out_LB, rpg_basedata, len, LB_ANY);
	   if (write_returned < 0)
	      PBD_abort ("ORPGDA_write (RPG basedata) Failed (%d)\n", write_returned);

        }
        else if( msg_type == RDA_STATUS_DATA ){

           ORDA_status_msg_t *rda_status = (ORDA_status_msg_t *) radar_data;

           /* Process the RDA Status message. */
           RDA_status = rda_status->rda_status;
           PBD_data_trans_enabled = rda_status->data_trans_enbld;
           LE_send_msg( GL_INFO, "Processing RDA Status Message ... RDA Status: %d, Data Trans Enbld: %u\n",
                        RDA_status, PBD_data_trans_enabled );

        }
        else if( msg_type == RDA_RPG_VCP ){

           /* Process the RDA/RPG VCP message. */
           LE_send_msg( GL_INFO, "Processing RDA/RPG VCP Message ...\n" );
           PD_process_rda_vcp_message( radar_data );
        
        }
        else if( msg_type == LOOPBACK_TEST_RDA_RPG ){

           /* Process the RDA/RPG Loopback message. */
           LE_send_msg( GL_INFO, "Processing RDA/RPG Loopback Message ...\n" );
           PD_process_rda_rpg_loopback_message( radar_data );

        }

    }
  
/* End of main() */
}

/*******************************************************************

   Description:   
      This function is the controlling module for the generation of 
      RPG base data structure from the RDA radial data. 

      This function also controls the generation of scan summary 
      data and the ORPGEVT_SCAN_INFO event.

      This function posts the ORPGEVT_START_OF_VOLUME event when 
      detected.  It delays sending radials through the RPG whenever 
      the weather mode changes or the VCP changes.  This allows the 
      routine product generator time to build the master product 
      generation list for the upcoming volume scan.

      This function also posts the ORPGEVT_END_OF_VOLUME event when 
      detected and when there is an unexpected start of volume.

   Inputs:        
      msg_type - type of message.
      rda_msg - RDA digital radar data.

   Outputs:       
      rpg_basedata - RPG base data message.

   Returns:       
      It returns 0 on success or -1 on failure.

*********************************************************************/
static int Process_a_radial( int msg_type, char *rda_msg, 
                             char *rpg_basedata ){

    int i, return_value, vol_aborted, unexpected_bov;
    time_t current_time = 0, previous_time = 0, wait_time;

    static orpgevt_scan_info_t scan_data;
    static int rpg_num_elev_cuts = 0;

    Generic_basedata_t *gbd = (Generic_basedata_t *) rda_msg;
    Base_data_header *rpg_hd = (Base_data_header *) rpg_basedata;

    /* For legacy radial message, convert to message 31 filling in as many
       message fields as possible. */
    if( msg_type == DIGITAL_RADAR_DATA ){

       gbd = (Generic_basedata_t *) Legacy_to_generic( rda_msg );
       if( gbd == NULL ){

          LE_send_msg( GL_INFO, "Bad Conversion Legacy To Generic\n" );
          return (-1);

       }

    }

    /* Initialize the azimuth resolution to 1 deg radials (the default
       value) and the maximum number of radials.  If the azimuth resolution 
       is different than the default, it and the maximum number of radials
       will be specified in the Generic Radial Header (Message 31). */
    PBD_max_num_radials = LR_MAXN_RADIALS;
    if( gbd->base.azimuth_res == HALF_DEG_RADIALS )
       PBD_max_num_radials = HR_MAXN_RADIALS;

    /* Set up cut and volume parameters for later processing. */
    if( PH_process_new_cut( gbd, &Vs_gsm, &rpg_num_elev_cuts) < 0 )
	return (-1);

    /* Do some radial validation. */
    vol_aborted = 0;
    unexpected_bov = 0;
    if( (return_value = PH_radial_validation( gbd, &vol_aborted, 
                                              &unexpected_bov) ) != PBD_NORMAL ){

       /* Continuing failures require immediate return.  New failures
          require this radial to be processed so that the algorithm
          control flag is set so algorithms can abort as required. */
       if( return_value == PBD_CONTINUING_FAILURE )
	  return (-1);

    }

    /* Process incomplete radial - save and fill reflectivity field.
       This call is only needed for split cut processing.  If radial is
       not part of a split cut, this function just returns normal.   
       Negative return values indicate error and this radial should not be
       processed. */
    if( PD_process_incomplete_radial( (char *) gbd ) < 0 )
	return (-1);

    /* Construct the RPG base data header. */
    if( PH_process_header( gbd, rpg_basedata ) < 0 )
	return (-1);

    /* Move digital radar data to the RPG basedata message. */
    if( PD_move_data( gbd, rpg_basedata ) < 0 )
	return (-1);

#ifdef SNR_TEST
    /* At start of volume, check the SNR thresholding flag and extract the SNR
       thresholds, if necessary. */
    if( (rpg_hd->status == GOODBVOL) && (rpg_hd->elev_num == 1) ){

        double dtemp;

        PBD_allow_snr_thresholding = 0;
        if( DEAU_get_values( "alg.pbd.allow_snr_thresholding", &dtemp, 1 ) >= 0 ){

            if( dtemp != 0 ){

                PBD_allow_snr_thresholding = 1;
                LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
                             "SNR Thresholding Enabled ....\n" );

            }
            else
                LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
                             "SNR Thresholding Disabled ....\n" );

        }
        else
            LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
                         "DEAU_get_values Failed .... SNR Thresholding Disabled\n" );

        /* Initialize the Z and Doppler SNR Thresholds to undefined.  These values will 
           be read and changed if the following DEAU call succeed. */
        PBD_sensitivity_loss[QRTKM] = 0;
        PBD_sensitivity_loss[ONEKM] = 0;
        PBD_apply_speckle_filter = 0;

        if( PBD_allow_snr_thresholding ){

            int n_cuts = 0;

            if( DEAU_get_values( "alg.pbd.sensitivity_loss_qrtkm", &dtemp, 1 ) >= 0 )
                PBD_sensitivity_loss[QRTKM] = dtemp;

            LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "Sensitivity Loss - 1/4 km(dB):  %7.1f\n",
                         PBD_sensitivity_loss[QRTKM] );

            if( DEAU_get_values( "alg.pbd.sensitivity_loss_onekm", &dtemp, 1 ) >= 0 )
                PBD_sensitivity_loss[ONEKM] = dtemp;

            LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "Sensitivity Loss - 1 km(dB):    %7.1f\n",
                         PBD_sensitivity_loss[ONEKM] );

            if( DEAU_get_values( "alg.pbd.apply_speckle_filter", &dtemp, 1 ) >= 0 ){

                if( dtemp != 0 ){

                    PBD_apply_speckle_filter = 1;
                    LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "Speckle Filter Enabled ...." );

                }
                else{

                    PBD_apply_speckle_filter = 0;
                    LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "Speckle Filter Disabled ...." );

                }

            }
            else
                LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
                             "DEAU_get_values Failed .... Speckle Filter Disabled ...." );


            /* Get the SNR thresholds for all cuts in the VCP. */
            n_cuts = ORPGVCP_get_num_elevations( rpg_hd->vcp_num );
            for( i = 0; i < n_cuts; i++ ){

                PBD_z_snr_threshold[i] = ORPGVCP_get_threshold( rpg_hd->vcp_num, i,
                                                                ORPGVCP_REFLECTIVITY );
 
                PBD_d_snr_threshold[i] = ORPGVCP_get_threshold( rpg_hd->vcp_num, i,
                                                                ORPGVCP_VELOCITY );

            }

        }

    }

    /* At beginning of elevation/volume, get the VCP defined SNR thresholds.   Adjust
       the moment SNR thresholds based on the sensitivity loss. */
    if( PBD_allow_snr_thresholding ){

        int ret, size;

        if( (rpg_hd->status == GOODBEL) || (rpg_hd->status == GOODBVOL) )
            LE_send_msg( GL_INFO, "From VCP Definition: Z SNR: %7.2f dB, V/W SNR: %7.2f dB\n",
                         PBD_z_snr_threshold[PBD_current_elev_num-1], 
                         PBD_d_snr_threshold[PBD_current_elev_num-1] );

        /* If SNR thresholding allowed, extract and/or derive information needed to apply
           the thresholding. */
 
        /* Extract the noise, syscal and atmos values.  Used for SNR Thresholding. */
        Noise = rpg_hd->horiz_noise;
        Atmos = 0.001f * rpg_hd->atmos_atten;
        Syscal = rpg_hd->calib_const - Noise; 

        /* Build the range correction table, if necessary. */
        SNR_build_range_corr_table( rpg_hd );

        /* Apply the SNR thresholds to Z, V and W or whichever fields are available. */
        SNR_apply_snr_thresholds( rpg_basedata );

        /* Convert to Message 31 to support Level II recording. */
        size = SNR_convert_to_msg31( rpg_basedata, Message_31 );

        /* Write Message 31 to LB. */
        ret = ORPGDA_write( SENSITIVITY_LOSS_RESP0, (char *) Message_31, size, LB_NEXT ); 
        if( ret < 0 )
           LE_send_msg( GL_INFO, "ORPGDA_write( SENSITIVITY_LOSS ... ) Return %d\n", ret );

    }
#endif

    /* Set scan summary data if beginning of volume or elevation. */
    if( (rpg_hd->status == GOODBEL) || (rpg_hd->status == GOODBVOL) ){

       SSS_set_scan_summary( rpg_hd->volume_scan_num, rpg_hd->weather_mode, 
                             rpg_num_elev_cuts, Vs_gsm.num_elev_cuts,
                             rpg_hd, gbd );

       /* Set the elevation cut number and scan date and time 
          for event notification. */
       scan_data.data.elev_cut_number = (int) rpg_hd->elev_num;
       scan_data.data.date = (int) rpg_hd->date;
       scan_data.data.time = (int) rpg_hd->time;

       /* Set the super resolution flag. */
       scan_data.data.super_res = PBD_super_res_this_elev;

       if( rpg_hd->status == GOODBVOL && rpg_hd->elev_num == 1 ){

          int ret;

          /* Set the volume scan number, VCP number, and key 
            for event notification. */
          scan_data.data.vol_scan_number = (int) rpg_hd->volume_scan_num;
          scan_data.data.vcp_number = (int) rpg_hd->vcp_num;
          scan_data.key = ORPGEVT_BEGIN_VOL;

          /* Set the previous volume scan aborted flag in the 
             volume-based general status message definitions. */
          if( vol_aborted )
             Vs_gsm.pv_status = 0;
          else
             Vs_gsm.pv_status = 1;
    
          /* If start of volume scan was unexpected, post END_OF_VOLUME event. */
          if( unexpected_bov ){

             orpgevt_end_of_volume_t eov;

             eov.vol_aborted = 1;
             eov.vol_seq_num = PBD_volume_seq_number - 1;
             eov.expected_vol_dur = 0;
             ret = EN_post( ORPGEVT_END_OF_VOLUME, (void *) &eov, 
                            ORPGEVT_END_OF_VOLUME_DATA_LEN,
                            (int) 0 );
             if( ret < 0 )
                LE_send_msg( GL_ERROR, "Event ORPGEVT_END_OF_VOLUME Failed (%d)\n", ret );

          }

          /* Set the volume scan sequence number of this volume scan.
             Reset initial volume flag. */
          Vs_gsm.volume_number = PBD_volume_seq_number;
          Vs_gsm.volume_scan = scan_data.data.vol_scan_number;
          Vs_gsm.initial_vol = 0;
          Vs_gsm.super_res_cuts = 0;
          Vs_gsm.dual_pol_expected = 0;

          /* Based on the RDA VCP data, set the cuts we expect Super Resolution this
             volume scan. */
          
          /* First verify whether the stored VCP data pattern number matches the
             actual pattern number in operation. */
          if( PBD_rda_vcp_data.vcp_msg_hdr.pattern_number == Vs_gsm.vol_cov_patt ){

             /* Do For All elevations in the VCP. */
             for( i = 0; i < PBD_rda_vcp_data.vcp_elev_data.number_cuts; i++ ){

                /* Test the elevation to see whether the "super res" flag is set. */
                if( (PBD_rda_vcp_data.vcp_elev_data.data[i].super_res & VCP_HALFDEG_RAD) != 0 ){

                   /* "super_res" flag is set for this elevation cut.  Set the 
                       corresponding bit in the volume status. */
                   int rpg_cut = Vs_gsm.elev_index[i] - 1;
          
                   Vs_gsm.super_res_cuts |= 1 << rpg_cut;

                }

                /* Test the elevation to see whether the "dual pol" flag is set. */
                if( (PBD_rda_vcp_data.vcp_elev_data.data[i].super_res & VCP_DUAL_POL_ENABLED) != 0 ){

                   /* "dual pol" flag is set for this elevation cut.  Set the 
                       word in the volume status. */
                   Vs_gsm.dual_pol_expected = 1;

                }

             }

          }

          /* Write out the volume-scan based general status message to
             linear buffer, then post event indicating start of volume
             scan. */
          EN_control( EN_SET_SENDER_ID, PBD_pid );
          ret = ORPGDA_write( ORPGDAT_GSM_DATA, (char *) &Vs_gsm,
                              sizeof( Vol_stat_gsm_t ), VOL_STAT_GSM_ID );

          if( ret < 0)
            LE_send_msg( GL_ERROR, "ORPGDAT_GSM_DATA write failed (%d)\n", ret );

          /* Post the event that the start of volume has begun. */
          ret = EN_post( ORPGEVT_SCAN_INFO, (void *) &scan_data, 
                         ORPGEVT_SCAN_INFO_DATA_LEN, (int) 0 );

          if( ret < 0 )
             LE_send_msg( GL_ERROR, 
                          "Event ORPGEVT_SCAN_INFO (key %d) Failed.  Ret = %d\n", 
                          scan_data.key, ret );
          
          else{

             time_t time_value;
             int ret, version, year, month, day, hours, minutes, seconds;

             /* Get LDM version number. */
             if( (version = ORPGMISC_get_LDM_version()) < 0 )
                version = 0;

             /* Write volume scan start message to system status log. */
             time_value = ((scan_data.data.date-1)*86400) + 
                          (scan_data.data.time/1000);
             ret = unix_time( &time_value, &year, &month, &day,
                              &hours, &minutes, &seconds );
             if( ret >= 0 ){

                if( year >= 2000 ) 
                   year = year - 2000;
                else
                   year = year - 1900;

                if( Vs_gsm.super_res_cuts == 0 ){

                   if( Vs_gsm.dual_pol_expected )
                      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                           "Vol: %d (Seq: %d) RDA Clock:%02d/%02d/%02d %02d:%02d:%02d VCP:%4d L2: %d DP\n", 
                           scan_data.data.vol_scan_number, PBD_volume_seq_number, month, day, 
                           year, hours, minutes, seconds, scan_data.data.vcp_number, version );

                   else
                      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                           "Vol: %d (Seq: %d) RDA Clock:%02d/%02d/%02d %02d:%02d:%02d VCP:%4d L2: %d\n", 
                           scan_data.data.vol_scan_number, PBD_volume_seq_number, month, day, 
                           year, hours, minutes, seconds, scan_data.data.vcp_number, version );
                }
                else{

                   if( Vs_gsm.dual_pol_expected )
                      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                           "Vol: %d (Seq: %d) RDA Clock:%02d/%02d/%02d %02d:%02d:%02d VCP:%4d L2: %d DP SR\n", 
                           scan_data.data.vol_scan_number, PBD_volume_seq_number, month, day, 
                           year, hours, minutes, seconds, scan_data.data.vcp_number, version );

                   else
                      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                           "Vol: %d (Seq: %d) RDA Clock:%02d/%02d/%02d %02d:%02d:%02d VCP:%4d L2: %d SR\n", 
                           scan_data.data.vol_scan_number, PBD_volume_seq_number, month, day, 
                           year, hours, minutes, seconds, scan_data.data.vcp_number, version );

                }

             }

             /* Clear the generation table set flag.  Let the product scheduler know  
                the start of volume has begun.   We need to wait at least some amount
                of time, otherwise product generation can occur before the routine
                scheduler is ready to handle the new volume's products. */
             Generation_tables_set = 0;
             ret = EN_post( ORPGEVT_START_OF_VOLUME, (void *) NULL, 
                            (size_t) 0, (int) 0 );

             if( ret < 0 )
                LE_send_msg( GL_ERROR, 
                   "Event ORPGEVT_START_OF_VOLUME Failed (%d)\n", ret );

             /* Wait for event from product scheduler that it is finished updating 
                the generation table and volume status for the upcoming volume
                scan. */
             previous_time = MISC_systime( NULL );
             wait_time = 0;
             while( !Generation_tables_set ){

                sleep( 1 );

                /* Compute the amount of time waited so far. */
                current_time = MISC_systime( NULL );
                wait_time += current_time - previous_time;
                previous_time = current_time;

                /*
                   If product scheduler fails to notify pbd that generation tables
                   are set, just continue. */
                if( !Generation_tables_set && (wait_time >= MAX_WAIT_TIME) ){

                   LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "Product Generation Tables Not Set\n" );
                   Generation_tables_set = 1;
                   break;

                }

             }

             /* Post an event after generation tables are set and before pbd
                starts pumping out data.  This ensures that listeners of this
                event who read the product generation tables will read the
                updated (for this volume scan) tables. */
             ret = EN_post( ORPGEVT_START_OF_VOLUME_DATA, (void *) NULL, 
                            (size_t) 0, (int) 0 );
             if( ret < 0 )
                LE_send_msg( GL_ERROR, 
                   "Event ORPGEVT_START_OF_VOLUME_DATA Failed (%d)\n", ret );

          }

          /* Update the old VCP number. */
          PBD_old_vcp_number = PBD_vcp_number;

          /* Update the old weather mode. */
          PBD_old_weather_mode = PBD_weather_mode;

       }
       else{

          int ret;

          /* Post a beginning of elevation event. */
          scan_data.key = ORPGEVT_BEGIN_ELEV;
          ret = EN_post( ORPGEVT_SCAN_INFO, (void *) &scan_data, 
                         ORPGEVT_SCAN_INFO_DATA_LEN, (int) 0 );
          if( ret < 0 )
             LE_send_msg( GL_ERROR, "Event ORPGEVT_SCAN_INFO (key %d) Failed.  Ret = %d\n", 
                          scan_data.key, ret );
       }

    }
    else if( rpg_hd->status == GENDVOL ){

       int ret;
       orpgevt_end_of_volume_t eov;

       /* Post a scan info event for end of volume scan. */
       scan_data.key = ORPGEVT_END_VOL;
       scan_data.data.date = (int) rpg_hd->date;
       scan_data.data.time = (int) rpg_hd->time;
       ret = EN_post( ORPGEVT_SCAN_INFO, (void *) &scan_data, 
                      ORPGEVT_SCAN_INFO_DATA_LEN, (int) 0 );
       if( ret < 0 )
          LE_send_msg( GL_ERROR, "Event ORPGEVT_SCAN_INFO (key %d) Failed.  Ret = %d\n", 
                       scan_data.key, ret );

       /* Post an end of volume event. */
       eov.vol_aborted = vol_aborted;
       eov.vol_seq_num = PBD_volume_seq_number;
       eov.expected_vol_dur = (unsigned int ) Vs_gsm.expected_vol_dur;
       ret = EN_post( ORPGEVT_END_OF_VOLUME, (void *) &eov, 
                      ORPGEVT_END_OF_VOLUME_DATA_LEN, (int) 0 );
       if( ret < 0 )
          LE_send_msg( GL_ERROR, "Event ORPGEVT_END_OF_VOLUME Failed.  Ret = %d\n", ret );

       
    }   
    else if( rpg_hd->status == GENDEL ){

       int ret;

       /* Post a beginning of elevation event. */
       scan_data.key = ORPGEVT_END_ELEV;
       scan_data.data.date = (int) rpg_hd->date;
       scan_data.data.time = (int) rpg_hd->time;
       ret = EN_post( ORPGEVT_SCAN_INFO, (void *) &scan_data, 
                      ORPGEVT_SCAN_INFO_DATA_LEN, (int) 0 );
       if( ret < 0 )
          LE_send_msg( GL_ERROR, "Event ORPGEVT_SCAN_INFO (key %d) Failed.  Ret = %d\n", 
                       scan_data.key, ret );
    }   

    /* Perform radial accounting... i.e., track pertinent information
       associated with elevation scan or volume scan. */
    PH_radial_accounting( rpg_hd, (char *) gbd );

    return (0);

/* End of Process_a_radial() */
}

/***********************************************************************

   Description:  
      This function services external events.

   Inputs:       
      evtcd - event code associated with the event.
      msg - message associated with this event.
      msglen - length of message associated with this
               event. 
      arg - optional argument

   Returns:      
      There is no return value defined for this function.

*********************************************************************/
void Event_handler( EN_id_t evtcd, char *msg, int msglen, void *arg ){

   /* Event generated by routine product scheduler indicating the master
      product generation tables have been built ....... pbd can now start
      data through the RPG is is hasn't aleady. */
   if( evtcd == ORPGEVT_PROD_GEN_CONTROL ){

      /* Set the Generation_tables_set flag. */
      Generation_tables_set = 1;

   }
   
/* End of Event_handler() */
}

/***********************************************************************

   Description:  
      This function services RDA control command LB update notification. 

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

   /* get the PID of the sender, if set. */
   pid_t sender_id = EN_sender_id();

   /* Check if this is an RDA Control Command. */
   if( fd == Rda_control_command_lbfd ){

      /* Check for valid argument before trying to dereference. */
      if( arg == ORPGDA_ARG_PUSHED_UNDEFINED ){

         LE_send_msg( GL_ERROR, "*arg Undefined In Lb_notify_handler\n" );
         return;

      }

      if( *((int *) arg) == ORPGDAT_RDA_COMMAND )
         Rda_control_command_received = 1;

   }

   /* Is this a Volume Status Update from someone else? */
   else if( (fd == Volume_status_lbfd) 
                    && 
            (sender_id != PBD_pid) )
      PBD_volume_status_updated = 1;

/* End of Lb_notify_handler() */
}

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

    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    PBD_verbose = 0;

    err = 0;
    while ((c = getopt (argc, argv, "hl:v")) != EOF) {
	switch (c) {

            /* Change the size of the task log file. */
            case 'l':
                Log_file_nmsgs = atoi( optarg );
                if( Log_file_nmsgs < 0 || Log_file_nmsgs > 5000 )
                   Log_file_nmsgs = 1000;
                break;

            /* Change the verbose level. */
            case 'v':
               PBD_verbose++;
               break;

            /* Print out the usage information. */
	    case 'h':
	    case '?':
		err = 1;
		break;
	}
    }

    if (err == 1) { 			/* Print usage message */

	printf ("Usage: %s [options]\n", argv[0]);
	printf ("       Options:\n");
	printf ("       -l Log File Number LE Messages (%d)\n", Log_file_nmsgs);
	ORPGTASK_exit (GL_EXIT_FAILURE);

    }

    return (0);

/* End of Read_options() */
}

#define MAX_MSG_SIZE	256
/********************************************************************

   Description:  
      This function sends an error message and terminates the process.

   Input:	  
      format - message format string;
      ... - list of variables whose values are to be printed.

********************************************************************/
void PBD_abort (char *format, ... ){

    char buf [MAX_MSG_SIZE];
    va_list args;

    /* If non-null format string, then .... */
    if (format != NULL && *format != '\0') {
	va_start (args, format);
	vsprintf (buf, format, args);
	va_end (args);
    }
    else
	buf [0] = '\0';

    /* Log error messages */
    LE_send_msg (GL_ERROR, buf);
    LE_send_msg (GL_TERM | GL_EXIT_FAILURE, "terminates\n");

    /* Terminate this process. */
    ORPGTASK_exit (GL_EXIT_FAILURE);

/* End of PBD_abort() */
}

/*************************************************************************
  
   Description:   
      Reads commands from the command Linear Buffer.  If the command is 
      an elevation or volume restart command, special processing is 
      required.  Otherwise, the command is ignored. 
  
   Returns:       
      Returns error is Linear Buffer Read failed; otherwise 0.
  
*************************************************************************/
static int Check_rda_commands( ){

   int ret;
   Rda_cmd_t rda_command;

   while(1){

      /* Read the RDA Commands Linear Buffer. */
      if( (ret = ORPGDA_read( ORPGDAT_RDA_COMMAND,
                              (char *) &rda_command,
                              (int) sizeof( Rda_cmd_t ),
                              LB_NEXT )) == LB_TO_COME ) break;
    
      if( ret < 0 ){

         LE_send_msg( GL_INFO, "ORPGDAT_RDA_COMMAND LB Read Failed (%d)\n", ret );
         return( ret );

      }

      /* The volume restart command and elevation restart command require
         special processing. */
      if( rda_command.cmd == COM4_RDACOM ){

         if( (rda_command.param1 == CRDA_RESTART_ELEV) 
                                 ||
             (rda_command.param1 == CRDA_RESTART_VCP) ){

            if( rda_command.line_num != PBD_INITIATED_RDA_CTRL_CMD ){

               /* On next radial processed, prepare for commanded restart of
                  volume or elevation scan.  In Commanded_restart we
                  check for LOCAL control or Archive II playback.  If either
                  true, the command is ignored. */
               Commanded_restart( rda_command.param1 );

            }
        
         }

      }
      else if( (rda_command.cmd == COM4_DLOADVCP)
                                &&
               (rda_command.line_num != PBD_INITIATED_RDA_CTRL_CMD) ){
            
         /* The VCP download command came from an application. */
         LE_send_msg( GL_INFO, "VCP Download Command Received ... param 1: %d, param 2: %d\n",
                      rda_command.param1, rda_command.param2 );
           
         /* Can only download VCP if RDA Control is not LOCAL. */
         if( PH_get_rda_status( RS_CONTROL_STATUS ) != RDA_CONTROL_LOCAL ){

            Vcp_struct *vcp_info = NULL;
            int passed_vcp_num = 0;

            if( rda_command.param2 == 1 ){

               vcp_info = (Vcp_struct *) rda_command.msg;
               passed_vcp_num = vcp_info->vcp_num;

            }

            /* If the commanded VCP is the same as the currently executing VCP,
               then we want to update "current". */
            if( (rda_command.param1 == PBD_vcp_number)
                                ||
                (passed_vcp_num == PBD_vcp_number) ){
               
               /* Did the Download Command come with the VCP definition? */ 
               if( rda_command.param2 == 1 ){

                  /* Update the "current" VCP with the passed data. */
                  LE_send_msg( GL_INFO, "--->Updating Current VCP Table With Application Data.\n" );
                  Update_current_vcp_table( PBD_vcp_number, vcp_info  );

               }
               else{
  
                  /* No VCP data was passed with command.  Update "current" with the Adaptation 
                     version. */
                  LE_send_msg( GL_INFO, "--->Updating Current VCP Table with Adaptation Data.\n" );
                  Update_current_vcp_table( PBD_vcp_number, NULL );
  
               }
                  
               /* Download the "current" VCP. */
               LE_send_msg( GL_INFO, "--->Commanding Download of VCP 0 (%d).\n", passed_vcp_num );
               if( ORPGRDA_send_cmd( COM4_DLOADVCP, PBD_INITIATED_RDA_CTRL_CMD, 0,
                                     0, 0, 0, 0, NULL ) < 0 )
                  LE_send_msg( GL_ERROR, "Unable to Command VCP Download\n" );
               
            }
            else{

               /* Command a download VCP.   The commanded VCP can be either "current" or
                  some other VCP.  The only thing we need to check in this case is whether
                  VCP data is passed with the command.  If passed, we want to pass it forward. */
               if( vcp_info != NULL ){

                  LE_send_msg( GL_INFO, "--->Commanding Download of VCP %d.\n", passed_vcp_num );
                  if( ORPGRDA_send_cmd( COM4_DLOADVCP, PBD_INITIATED_RDA_CTRL_CMD, passed_vcp_num,
                                        0, 0, 0, 0, (char *) vcp_info, sizeof(Vcp_struct)) < 0 )
                     LE_send_msg( GL_ERROR, "Unable to Command VCP Download\n" );

                  else if( PBD_verbose )
                     VM_write_vcp_data( vcp_info );

               }
               else{

                  LE_send_msg( GL_INFO, "--->Commanding Download of VCP %d.\n", rda_command.param1 );
                  if( ORPGRDA_send_cmd( COM4_DLOADVCP, PBD_INITIATED_RDA_CTRL_CMD, rda_command.param1,
                                        0, 0, 0, 0, NULL ) < 0 )
                     LE_send_msg( GL_ERROR, "Unable to Command VCP Download\n" );

               }
 
            }

         }

      }

   /* End of "while" loop. */
   }

   return (0);

/* End of Check_rda_commands() */
}

/*****************************************************************************

   Description:  
      This function sets a global flag indicating whether or not an 
      externally generated commanded restart of elevation scan or volume 
      scan was received.

   Inputs:       
      param - Control command (volume restart or elevation restart).

****************************************************************************/
static void Commanded_restart( int param ){

   int rda_status;

   /* Check if the RDA is not LOCAL control and not in PLAYBACK.  If one or 
      the other is false, just return. */
   if( (PH_get_rda_status( RS_CONTROL_STATUS ) == RDA_CONTROL_LOCAL)
                          ||
       ((rda_status = PH_get_rda_status( RS_RDA_STATUS )) & RDA_STATUS_PLAYBACK) ){

      LE_send_msg( GL_INFO, 
             "RDA Either LOCAL Control or PLAYBACK.  Ignore RESTART Command\n" );
      return;

   }

   if( param == CRDA_RESTART_ELEV ){ 

      /* Set flag indicating elevation restart commanded. */
      PBD_elevation_restart_commanded = 1;

      LE_send_msg( GL_INFO, "Restart Elevation Scan Command Received.\n" );

   }

   else if ( param == CRDA_RESTART_VCP ){

      /* Set flag indicating volume restart commanded. */
      PBD_volume_restart_commanded = 1;

      LE_send_msg( GL_INFO, "Restart Volume Scan Command Received.\n" );

   }

/* End of Commanded_restart() */
}

/*************************************************************************

   Description:   
      Registers for events and sets file pointers to LB's which are 
      read when an event is received.
  
      Currently, PBD registers for the following event(s).

         ORPGEVT_PROD_GEN_CONTROL

   Returns:    
      There is no return value defined for this function.
  
*************************************************************************/
static void Init_events(){

    int event_registered;

    /* Register for the PROD_GEN_CONTROL event.  This event is generated
       by the routine scheduler to indicate that the master generation 
       list has been built. */
    event_registered = EN_register( ORPGEVT_PROD_GEN_CONTROL, Event_handler );

    /* Error in event registration.  Write error message to task log file. */
    if( event_registered < 0 )
       LE_send_msg( GL_ERROR, 
                    "Unable to Register Event ORPGEVT_PROD_GEN_CONTROL (%d)\n",
                    event_registered );

/* End of Init_events() */
}

/*************************************************************************
  
     Description:   
        Registers for LB notification. 
  
*************************************************************************/
static void Init_lb_notification(){

   int ret;

   static int rda_control_command = ORPGDAT_RDA_COMMAND;

   /* Register for notification for ORPGDAT_RDA_COMMANDS. */
   ORPGDA_push_arg( &rda_control_command );
   if( (ret = ORPGDA_UN_register( ORPGDAT_RDA_COMMAND, LB_ANY, 
                                  Lb_notify_handler )) < 0 ){

      LE_send_msg( GL_ERROR, 
                   "RDA Control Command LB Notification Failed (%d)\n", ret );
      ORPGTASK_exit( GL_EXIT_FAILURE );

   }

   Rda_control_command_received = 0;
   Rda_control_command_lbfd = ORPGDA_lbfd( ORPGDAT_RDA_COMMAND );

   /* Register for Volume Status updates. */
   if( (ret = ORPGDA_UN_register( ORPGDAT_GSM_DATA, VOL_STAT_GSM_ID,
                                  Lb_notify_handler )) < 0 ){

      LE_send_msg( GL_ERROR,
                   "Volume Status LB Notification Failed (%d)\n", ret );
      ORPGTASK_exit( GL_EXIT_FAILURE );

   }

   PBD_volume_status_updated = 0;
   Volume_status_lbfd = ORPGDA_lbfd( ORPGDAT_GSM_DATA );

/* End of Init_lb_notification() */
}

/**************************************************************************************

   Description:
      Alarm (timer) initialization module. 

**************************************************************************************/
void Init_alarms(){

   int retval;

   /* Register and set RDA inactivity alarm. */
   if( (retval = ALARM_register( PBD_INACTVTY_ALRM_ID, Alarm_handler )) < 0 )
      LE_send_msg( GL_ERROR, "RDA Inactivity Alarm Registration Failed (%d)\n",
                   retval );

   else{

      time_t alarm_activated_at = MISC_systime(NULL) + 
                                  (unsigned int) PBD_INACTVTY_ALARM_INTERVAL;

      Inactvty_alarm_reg = 1;
      PBD_rda_comm_inactivity = 0;
      PBD_time_of_last_message = MISC_systime(NULL);
      if( (retval = ALARM_set( (malrm_id_t) PBD_INACTVTY_ALRM_ID, 
                               alarm_activated_at,
                               MALRM_ONESHOT_NTVL )) < 0 )
         LE_send_msg( GL_ERROR, "RDA Inactivity Alarm Set Failed (%d)\n", retval );

   }

   /* Register and set RDA Input Buffer Check alarm. */
   if( (retval = ALARM_register( PBD_INBUF_CHECK_ALRM_ID, Alarm_handler )) < 0 )
      LE_send_msg( GL_ERROR, "RDA Input Buffer Check Alarm Registration Failed (%d)\n",
                   retval );

   else{

      time_t alarm_activated_at = MISC_systime(NULL) + 
                                  (unsigned int) PBD_INBUF_CHECK_ALARM_INTERVAL;

      Inbuf_check_alarm_reg = 1;
      if( (retval = ALARM_set( (malrm_id_t) PBD_INBUF_CHECK_ALRM_ID, 
                               alarm_activated_at,
                               MALRM_ONESHOT_NTVL )) < 0 )
         LE_send_msg( GL_ERROR, "RDA Input Buffer Check Alarm Set Failed (%d)\n", 
                      retval );

   }

/* End of Init_alarms() */
}

/**************************************************************************************

   Description:
      Alarm handler for PBD_INBUF_CHECK_ALRM_ID.  A global variable is set to 
      indicate that the communication manager LB needs to checked for the percentage
      of unread messages.

      Alarm handler for PBD_ACTVTY_ALRM_ID.  Whenever PBD has not received any messages
      from the wideband comm manager within a specified period of time, the alarm is
      triggered.

   Inputs:
      alarm_id - alarm ID

***************************************************************************************/
void Alarm_handler( malrm_id_t alarm_id ){

   switch( alarm_id ){

      case PBD_INBUF_CHECK_ALRM_ID:
      {

         Reset_inbuf_check_alarm = 1;
         break;

      }

      case PBD_INACTVTY_ALRM_ID:
      {

         time_t current_time = MISC_systime(NULL);

         /* Send message to operator if necessary. */
         if( PBD_rda_comm_inactivity ){

            if( (current_time - PBD_time_of_last_message) >= PBD_INACTVTY_CONT )
               LE_send_msg( GL_INFO, "Comm Manager Response Inactivity Alarm CONTINUING\n" );

         }
         else{

            if( (current_time - PBD_time_of_last_message) >= PBD_INACTVTY_ALARM_INTERVAL ){

               /* Set the flag indicating active alarm and report to operator. */
               PBD_rda_comm_inactivity = 1;
               LE_send_msg( GL_INFO, "Comm Manager Response Inactivity Alarm ACTIVATED\n" );

            }

         }

         /* Set flag indicating alarm needs to be reset. */
         Reset_inactivity_alarm = 1;

         break;

      }

      default:
      {
         LE_send_msg( GL_INFO, "Unknown alarm_id Received (%d)\n", alarm_id );
         break;

      }

   /* End of "switch" */
   }

/* End of Alarm_handler() */
}

/**************************************************************************************

   Description:
      This functions checks the number of unread messages in data store "data_id" 
      and sets the ratio (number of unread messages) / ( maximum number of messages).  
      This number is then reported as a percent to ORPGLOAD for 
      LOAD_SHED_CATEGORY_RDA_RADIAL.
   
      If "msg_id" equals 0xffffffff (-1), it is assume unavailable and the load is
      reported as 0.

   Inputs:
      data_id - data store ID
      msg_id - ID of message last read.
      read_returned - return value from reading LB.

*************************************************************************************/
static void Check_input_buffer_load( int data_id, LB_id_t msg_id, int read_returned ){

   int ret;
   LB_status status;
   LB_attr attr;
   LB_info info;

   int max_msgs = 0;
   int load = 0; 
   int number_unread = 0;

   /* If "msg_id" is valid, do the following. */
   if( msg_id != 0xffffffff ){

      /* Get maximum number of messages in LB. */
      status.n_check = 0;
      status.attr = &attr;
      if( (ret = ORPGDA_stat( data_id, &status )) < 0 ){

         LE_send_msg( GL_ERROR, 
                      "Unable To Check RDA Input Buffer Load (%d)\n", ret );
         return;

      }
      max_msgs = attr.maxn_msgs;

      /* Find the number of unread messages in the LB. */
      if( (ret = ORPGDA_msg_info( data_id, LB_LATEST, &info )) < 0 ){

         LE_send_msg( GL_ERROR, 
                      "Unable To Check RDA Input Buffer Load (%d)\n", ret ); 
         return;

      }

      number_unread = info.id - msg_id;
      if( number_unread < 0 )
         number_unread = -number_unread;

   }

   /* Determine load (in percent) */
   if( number_unread > 0 ){

      if( max_msgs > 0 ){

         load = number_unread * 100;
         load = (int) ((float) load) / ((float) max_msgs);

      }

   }
   else{

      /* If "read_returned" value is LB_EXPIRED, then set load level to 100. 
         Otherwise, set to 0. */
      if( read_returned == LB_EXPIRED )
         load = 100;

      else
         load = 0;

   }

   /* Report the load. */
   ORPGLOAD_set_data( LOAD_SHED_CATEGORY_RDA_RADIAL,
                      LOAD_SHED_CURRENT_VALUE, load );
 
/* End of Check_inbut_buffer_load() */
}

/**************************************************************************************

   Description:
      Termination handler for pbd. 

   Inputs:
      see ORPGTASK man page.

   Returns:
      Always returns 0. 

*************************************************************************************/
static int Cleanup_fxn( int signal, int status ){

   /* Don't have anything to do.  Report signal received and return. */
   LE_send_msg( GL_INFO, "Signal %d Received\n", signal );
   return (0);

/* End of Cleanup_fxn() */
}

/*************************************************************************************

   Description:
      Open all LB's used by Process Base Data

**************************************************************************************/
static void Open_lb( ){

   int retval, data_id;
   Orpgtat_entry_t *task_entry = NULL;
   char task_name[ORPG_TASKNAME_SIZ];

   PBD_response_LB = -1;
   PBD_radial_out_LB = -1;

   /* Get my task name .... this will be used to access the task table entry. */
   if( (ORPGTAT_get_my_task_name( (char *) &task_name[0], ORPG_TASKNAME_SIZ ) >= 0)
                                   &&
       ((task_entry = ORPGTAT_get_entry( (char *) &task_name[0] )) != NULL) ){

      /* Check for match on input data name. */
      if( (data_id = ORPGTAT_get_data_id_from_name( task_entry, "RESPONSE_LB" )) >= 0){

         PBD_response_LB = data_id;
         if( PBD_response_LB < ORPGDAT_BASE ){

            /* The input ID is a link number.  Assume the data ID is link 
               number + ORPGDAT_CM_RESPONSE. */
            PBD_response_LB += ORPGDAT_CM_RESPONSE;

         }

         LE_send_msg( GL_INFO, 
                   "PBD_response_LB (%d) Defined By Task Attribute Table\n", 
                   PBD_response_LB );
      }

      /* Check for match on input data name. */
      if( (data_id = ORPGTAT_get_data_id_from_name( task_entry, "RAWDATA" )) >= 0 ){

         PBD_radial_out_LB = data_id;
         LE_send_msg( GL_INFO,
                   "PBD_radial_out_LB (%d) Defined By Task Attribute Table\n",
                   PBD_radial_out_LB );

      }

      if( task_entry != NULL )
         free( task_entry );

   }

   /* If PBD_response_LB not defined in TAT, there is nothing for pbd to do. */
   if( PBD_response_LB < 0 ){

      LE_send_msg( GL_ERROR, "RESPONSE_LB Not Defined in TAT\n" );
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

   /* If PBD_radial_out_LB not defined in TAT, there is nothing for pbd to do. */
   if( PBD_radial_out_LB < 0 ){

      LE_send_msg( GL_ERROR, "RAWDATA Not Defined in TAT\n" );
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

   /* Open LB's for READ access. */
   if( (retval = ORPGDA_open( PBD_response_LB, LB_READ )) < 0 ){

      LE_send_msg( GL_ERROR, "ORPGDA_open for READ Access Failed (%d)\n", retval );
      ORPGTASK_exit( GL_EXIT_FAILURE );

   }

   /* Open LB's for WRITE access. */
   if( (retval = ORPGDA_open( ORPGDAT_RDA_COMMAND, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_GSM_DATA, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_RDA_VCP_DATA, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_ACCDATA, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_LOAD_SHED_CAT, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( PBD_radial_out_LB, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_SCAN_SUMMARY, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_ADAPTATION, LB_WRITE )) < 0 ){

      LE_send_msg( GL_ERROR, "ORPGDA_open for WRITE Access Failed (%d)\n", retval );
      ORPGTASK_exit( GL_EXIT_FAILURE );

   }
   
/* End of Open_lb() */
}


/********************************************************************

   Description:   
      Initializes volume scan number, volume scan sequence number,
      volume coverage pattern and weather mode from data read from 
      Volume Status (VOL_STAT_GSM_ID) in LB ORPGDAT_GSM_DATA. 

********************************************************************/
static void Init_vsnum_wxmode( ){

   int ret;

   /* Read in the volume status message from the General Status LB.  
      Need to initialize the volume sequence number. */
   ret = ORPGDA_read( ORPGDAT_GSM_DATA, (char *) &Vs_gsm, 
                      sizeof(Vol_stat_gsm_t), VOL_STAT_GSM_ID );

   /* Some error occurred? */
   if( ret < 0 ){

      /* Re-initialize the volume sequence number on read failure. */
      LE_send_msg( GL_ERROR, "ORPGDAT_GSM_DATA Read Failed (%d)\n", ret );
      PBD_volume_seq_number = 0;

   }
   else
      PBD_volume_seq_number = Vs_gsm.volume_number;

   /* Initialize the volume scan number. */
   PBD_volume_scan_number = PBD_volume_seq_number % PBD_MAX_SCANS;
   if( (PBD_volume_seq_number != 0)
                 && 
       (PBD_volume_scan_number == 0) )
      PBD_volume_scan_number = PBD_MAX_SCANS;

   PBD_weather_mode = Vs_gsm.mode_operation;
   PBD_vcp_number = Vs_gsm.vol_cov_patt;

   /* Set PBD_old_weather_mode and PBD_old_vcp_number. */
   if( (!Vs_gsm.initial_vol)
              && 
       (Vs_gsm.volume_number != 0) ){

      PBD_old_weather_mode = PBD_weather_mode;
      PBD_old_vcp_number = Vs_gsm.vol_cov_patt;

   }

   return;

/* End of Init_vsnum_wxmode() */
}

/*********************************************************************

   Description:  
      The current vcp table is updated with adaptation data version
      or with version passed in argument vcp if not NULL.
  
   Inputs:  
      vcp_num - Volume Coverage Pattern to validate.
      vcp - volume coverage pattern data (optional)
  
   Returns:   
      -1 on error, or 0 otherwise.
  
   Design Decision:  
      An issue was submitted requesting the current vcp table 
      be updated whenever the user downloads a VCP.  This could 
      have been done in control_rda as well.  It was decided that
      the functionality be in pbd since I only desire one writer of
      the VCP data.  

      Furthermore, the update is only performed if the adaptation
      data version of the current VCP is downloaded.  If the
      VCP changes, the update would have occurred anyway at the 
      start of volume.
            
***********************************************************************/
static int Update_current_vcp_table( int vcp_num, Vcp_struct *vcp ){

   int ret, vcp_ind;

   /* If VCP number is not zero, validate selection.  If data is not 
      passed, then data needs to be extracted from adaptation data.  The
      VCP data then needs to be copied to current. */
   if( vcp_num != 0 ){

      if( (vcp_ind = ORPGVCP_index( vcp_num )) < 0 ){

         LE_send_msg( GL_ERROR, "Requested VCP For Download Unrecognized\n" );
         return ( -1 );

      }

      /* If VCP data not passed, find vcp in adaptation data. */
      if( vcp == NULL ){

         /* Adjust pointer to point to VCP data. */
         vcp = (Vcp_struct *) ORPGVCP_ptr( vcp_ind );

      }
      else
         vcp_ind = -1;

      /* Do structure copy. */
      memcpy( (void *) &Vs_gsm.current_vcp_table, vcp, sizeof(Vcp_struct) );
          
      /* Write VCP data to LB. If write fails, return error. */
      EN_control( EN_SET_SENDER_ID, PBD_pid );
      if( (ret = ORPGDA_write( ORPGDAT_GSM_DATA, (char*) &Vs_gsm, 
                               sizeof( Vol_stat_gsm_t ), VOL_STAT_GSM_ID )) < 0 ){

         LE_send_msg( GL_ERROR, "Write of ORPGDAT_GSM_DATA Failed (%d)\n", ret );
         return ( -1 );

      }

      if( PBD_verbose )
         VM_write_vcp_data( &Vs_gsm.current_vcp_table );

   }

   return ( 0 );

/* End of Update_current_vcp_table() */
}

/******************************************************************

   Description:
      Converts message 1 radial data to message 31. 

   Inputs:
      msg_header - RDA message 1.

   Returns:
      Generic radial message (Message 31).

   Note:  
      This is a slightly modified version of UMC_convert_to_31() 
      which can be found in orpgumc_rda.c.

******************************************************************/
static char* Legacy_to_generic( char *rda_msg ){

    static int buf_size = 0;
    static char *buf = NULL;

    ORDA_basedata_header *d_hd = NULL;	
    RDA_RPG_message_header_t *msg_hd = NULL;

    Generic_basedata_header_t *gbhd = NULL;
    Generic_vol_t *vol_hd = NULL;
    Generic_elev_t *elev_hd = NULL;
    Generic_rad_t *rad_hd = NULL;
    Generic_moment_t *refhd = NULL, *velhd = NULL, *spwhd = NULL;
    int n_bins, size, cnt;

    d_hd = (ORDA_basedata_header *) rda_msg;

    /* Allocate/reallocate the buffer for the message 31 radial */
    n_bins = d_hd->n_surv_bins;
    if( d_hd->n_dop_bins > n_bins )
	n_bins = d_hd->n_dop_bins;

    if (n_bins > buf_size) {

	if (buf != NULL)
	    free (buf);

	buf = MISC_malloc( sizeof (RDA_RPG_message_header_t) + 
                           sizeof (Generic_basedata_header_t) + 64 +
                           sizeof (Generic_vol_t) + sizeof (Generic_elev_t) + 
                           sizeof (Generic_rad_t) + 
                           3 * (sizeof (Generic_moment_t) + n_bins + 4));

	buf_size = n_bins;

    }

    /* Verify a buffer was allocated.   If number of bins is 0, a buffer is
       not allocated.   Need to exit module in this case (or if MISC_malloc 
       fails to allocated a buffer.) */
    if( buf == NULL ){

       LE_send_msg( GL_INFO, "Legacy_to_generic: # Bins == 0 OR MISC_malloc Failed!\n" );
       return NULL;

    }

    /* Fill out the message header */
    memcpy (buf, rda_msg, sizeof (RDA_RPG_message_header_t));
    msg_hd = (RDA_RPG_message_header_t *)buf;
    msg_hd->type = GENERIC_DIGITAL_RADAR_DATA;
    msg_hd->num_segs = 1;
    msg_hd->seg_num = 1;

    /* Fill out the generic basedata header */
    gbhd = (Generic_basedata_header_t *)
				(buf + sizeof(RDA_RPG_message_header_t) );
    strncpy( gbhd->radar_id, PBD_radar_name, 4 );
    gbhd->time = d_hd->time;
    gbhd->date = d_hd->date;
    gbhd->azi_num = d_hd->azi_num;
    gbhd->azimuth = ORPGVCP_ICD_angle_to_deg( ORPGVCP_AZIMUTH_ANGLE, 
					      d_hd->azimuth );
    gbhd->compress_type = 0;
    gbhd->azimuth_res = ONE_DEGREE_AZM;
    gbhd->status = d_hd->status;
    gbhd->elev_num = d_hd->elev_num;
    gbhd->sector_num = d_hd->sector_num;
    gbhd->elevation = ORPGVCP_ICD_angle_to_deg( ORPGVCP_ELEVATION_ANGLE, 
						d_hd->elevation );
    gbhd->spot_blank_flag = d_hd->spot_blank_flag;
    gbhd->azimuth_index = 0;

    /* Sets pointers to other headers */
    size = ALIGNED_SIZE( sizeof(Generic_basedata_header_t) + 6 * sizeof(int) );
    gbhd->data[0] = size;

    vol_hd = (Generic_vol_t *) ((char *) gbhd + size );
    size += ALIGNED_SIZE ( sizeof(Generic_vol_t) );
    gbhd->data[1] = size;

    elev_hd = (Generic_elev_t *) ((char *) gbhd + size );
    size += ALIGNED_SIZE ( sizeof(Generic_elev_t) );
    gbhd->data[2] = size;

    rad_hd = (Generic_rad_t *) ((char *) gbhd + size );
    size += ALIGNED_SIZE ( sizeof(Generic_rad_t) );

   cnt = 3;
   refhd = velhd = spwhd = NULL;

   if( d_hd->ref_ptr > 0 ){

      gbhd->data[cnt] = size;
      refhd = (Generic_moment_t *) ((char *) gbhd + size );
      size += ALIGNED_SIZE( sizeof (Generic_moment_t) + d_hd->n_surv_bins );
      cnt++;

   }

   if( d_hd->vel_ptr > 0 ){

      gbhd->data[cnt] = size;
      velhd = (Generic_moment_t *) ((char *) gbhd + size );
      size += ALIGNED_SIZE( sizeof (Generic_moment_t) + d_hd->n_dop_bins );
      cnt++;

   }

   if( d_hd->spw_ptr > 0 ){

      gbhd->data[cnt] = size;
      spwhd = (Generic_moment_t *) ((char *) gbhd + size );
      size += ALIGNED_SIZE( sizeof (Generic_moment_t) + d_hd->n_dop_bins );
      cnt++;

   }

   gbhd->no_of_datum = cnt;
   msg_hd->size = (size + sizeof(RDA_RPG_message_header_t)) / sizeof (short);
   gbhd->radial_length = size;

   /* Fill out the volume header */
   strcpy (vol_hd->type, "RVOL");
   vol_hd->len = sizeof(Generic_vol_t);
   vol_hd->major_version = 1;
   vol_hd->minor_version = 0;

   /* Get the latitude/longitude and radar height from RPG adaptation data. */
   vol_hd->lat = PBD_latitude; 
   vol_hd->lon = PBD_longitude; 
   vol_hd->height = PBD_rda_height;

   /* Assume a 20 m tower ..... */
   vol_hd->feedhorn_height = 20.0f;

   vol_hd->calib_const = d_hd->calib_const;
   vol_hd->horiz_shv_tx_power = 0.0f;
   vol_hd->vert_shv_tx_power = 0.0f;
   vol_hd->sys_diff_refl = 0.0f;
   vol_hd->sys_diff_phase = 0.0f;
   vol_hd->vcp_num = d_hd->vcp_num;
   vol_hd->sig_proc_states = 0;
    
   /* Fill out the elevation header */
   strcpy (elev_hd->type, "RELV");
   elev_hd->len = sizeof(Generic_elev_t);
   elev_hd->atmos = d_hd->atmos_atten;
   elev_hd->calib_const = d_hd->calib_const;
    
   /* Fill out the radial header */
   strcpy (rad_hd->type, "RRAD");
   rad_hd->len = sizeof(Generic_rad_t);
   rad_hd->unamb_range = d_hd->unamb_range;
   rad_hd->horiz_noise = 0.0f;
   rad_hd->vert_noise = 0.0f;
   rad_hd->nyquist_vel = d_hd->nyquist_vel;
   rad_hd->spare = 0;

   /* Fill out the moments */

   /* Fill the Reflectivity moment header. */
   if( refhd ){

      char *data = ((char *) d_hd) + sizeof (RDA_RPG_message_header_t) + d_hd->ref_ptr;

      memcpy( refhd->name, "DREF", 4 );
      refhd->info = 0;
      refhd->no_of_gates = d_hd->n_surv_bins;
      refhd->first_gate_range = d_hd->surv_range;
      refhd->bin_size = d_hd->surv_bin_size;
      refhd->tover = d_hd->threshold_param;
      refhd->SNR_threshold  = (short) (ORPGVCP_get_threshold( vol_hd->vcp_num,
                                                              gbhd->elev_num, 
                                                              ORPGVCP_REFLECTIVITY ) * 8.0);
      refhd->control_flag = 0;
      refhd->data_word_size = 8;
      refhd->scale = 2.0f;
      refhd->offset = 66.0f;
      memcpy( refhd->gate.b, data, refhd->no_of_gates );

   }

   /* Fill the Velocity moment header. */
   if( velhd ){

      float scale;

      char *data = ((char *) d_hd) + sizeof (RDA_RPG_message_header_t) + d_hd->vel_ptr;

      if( d_hd->vel_resolution == 2 )
         scale = 2.f;

      else
         scale = 1.f;

      memcpy( velhd->name, "DVEL", 4 );
      velhd->info = 0;
      velhd->no_of_gates = d_hd->n_dop_bins;
      velhd->first_gate_range = d_hd->dop_range;
      velhd->bin_size = d_hd->dop_bin_size;
      velhd->tover = d_hd->threshold_param;
      velhd->SNR_threshold  = (short) (ORPGVCP_get_threshold( vol_hd->vcp_num,
                                                              gbhd->elev_num, 
                                                              ORPGVCP_VELOCITY ) * 8.0);
      velhd->control_flag = 0;
      velhd->data_word_size = 8;
      velhd->scale = scale;
      velhd->offset = 129.0f;
      memcpy( velhd->gate.b, data, velhd->no_of_gates );

    }

    /* Fill the Spectrum Width moment header. */
    if( spwhd ){

      char *data = ((char *) d_hd) + sizeof (RDA_RPG_message_header_t) + d_hd->spw_ptr;

      memcpy( spwhd->name, "DSW ", 4 );
      spwhd->info = 0;
      spwhd->no_of_gates = d_hd->n_dop_bins;
      spwhd->first_gate_range = d_hd->dop_range;
      spwhd->bin_size = d_hd->dop_bin_size;
      spwhd->tover = d_hd->threshold_param;
      spwhd->SNR_threshold  = (short) (ORPGVCP_get_threshold( vol_hd->vcp_num,
                                                              gbhd->elev_num, 
                                                              ORPGVCP_SPECTRUM_WIDTH ) * 8.0);

      spwhd->control_flag = 0;
      spwhd->data_word_size = 8;
      spwhd->scale = 2.0f;
      spwhd->offset = 129.0f;
      memcpy( spwhd->gate.b, data, spwhd->no_of_gates );

    }

    return (buf);

/* End of Legacy_to_generic(). */
}

#ifdef AVSET_TEST
/*\////////////////////////////////////////////////////////////////////////////////////

   Description:
      Build area lookup table and get adaptation data to support AVSET.

////////////////////////////////////////////////////////////////////////////////////\*/
int AVSET_compute_area_lookup(){

   double bincnst, bincnst1, bincnst2, bincnst3, bincnst4, bincnst5;
   double rng;
   int n;

   /* For 1 km reflectivity data. */
   bincnst1 = 10.0;
   bincnst2 = bincnst1*bincnst1;
   bincnst3 = 2.0*bincnst1;

   /* Compute Bin Area table (in tenths of kilometers squared). */
   bincnst = 3.14159/360.0;
   bincnst4 = bincnst * bincnst2;
   bincnst5 = bincnst * bincnst3;
   rng = 0.0;

   /* For all bins out to 460. */
   for( n = 0; n < 460; n++ ){

      Bintbl[n] = (float) (bincnst4 + (rng*bincnst5));
      rng += bincnst1;

   }

   /* Do the same thing, but this time for 0.25 km reflectivity data. */
   bincnst1 /= 4.0;
   bincnst2 = bincnst1*bincnst1;
   bincnst3 = 2.0*bincnst1;

   /* Compute Bin Area table (in tenths of kilometers squared). */
   bincnst4 = bincnst * bincnst2;
   bincnst5 = bincnst * bincnst3;
   rng = 0.0;

   /* For all bins out to 460. */
   for( n = 0; n < 1840; n++ ){

      Bintbl_sr[n] = (float) (bincnst4 + (rng*bincnst5));
      rng += bincnst1;

   }

   return 0;

/* End of AVSET_compute_area_lookup(). */
}

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Update AVSET adaptation data.

/////////////////////////////////////////////////////////////////////////\*/
void AVSET_read_adaptation_data(){

   double dtemp;
   float low_ref, high_ref, low_ref_2nd_pass, high_ref_2nd_pass;

   /* Get AVSET enable/disable switch. */
   if( DEAU_get_values( "alg.avset.enable_avset", &dtemp, 1 ) >= 0 )
      Avset_enabled = (int) dtemp;

   else
      Avset_enabled = 0;

   /* Get adaptation data for thresholding. */
   /* Low Reflectivity threshold. */
   if( DEAU_get_values( "alg.avset.low_ref_thresh", &dtemp, 1 ) >= 0 ){

      low_ref = (float) dtemp;
      Low_refl_thresh = (int) round( 2.0*dtemp + 64.0 );
      Low_refl_thresh += 2;

   }
   else{

      low_ref = 18.0;
      Low_refl_thresh = 102;

   }

   if( DEAU_get_values( "alg.avset.low_ref_thresh_2nd_pass", &dtemp, 1 ) >= 0 ){

      low_ref_2nd_pass = (float) dtemp;
      Low_refl_thresh_2nd_pass = (int) round( 2.0*dtemp + 64.0 );
      Low_refl_thresh_2nd_pass += 2;

   }
   else{

      low_ref_2nd_pass = 18.0;
      Low_refl_thresh_2nd_pass = 102;

   }

   /* High Reflectivity threshold. */
   if( DEAU_get_values( "alg.avset.high_ref_thresh", &dtemp, 1 ) >= 0 ){

      high_ref = (float) dtemp;
      High_refl_thresh = (int) round( 2.0*dtemp + 64.0 );
      High_refl_thresh += 2;

   }
   else{

      high_ref = 30.0;
      High_refl_thresh = 126;

   }

   if( DEAU_get_values( "alg.avset.high_ref_thresh_2nd_pass", &dtemp, 1 ) >= 0 ){

      high_ref_2nd_pass = (float) dtemp;
      High_refl_thresh_2nd_pass = (int) round( 2.0*dtemp + 64.0 );
      High_refl_thresh_2nd_pass += 2;

   }
   else{

      high_ref_2nd_pass = 25.0;
      High_refl_thresh_2nd_pass = 116;

   }

   /* Low Reflectivity Area threshold. */
   if( DEAU_get_values( "alg.avset.low_ref_area_thresh", &dtemp, 1 ) >= 0 )
      Low_refl_area_thresh = (float) dtemp;

   else
      Low_refl_area_thresh = 80;

   /* High Reflectivity Area threshold. */
   if( DEAU_get_values( "alg.avset.high_ref_area_thresh", &dtemp, 1 ) >= 0 )
      High_refl_area_thresh = (float) dtemp;

   else
      High_refl_area_thresh = 30;

   /* Low Reflectivity Area threshold. */
   if( DEAU_get_values( "alg.avset.low_ref_area_thresh_2nd_pass", &dtemp, 1 ) >= 0 )
      Low_refl_area_thresh_2nd_pass = (float) dtemp;

   else
      Low_refl_area_thresh_2nd_pass = 30;

   /* High Reflectivity Area threshold. */
   if( DEAU_get_values( "alg.avset.high_ref_area_thresh_2nd_pass", &dtemp, 1 ) >= 0 )
      High_refl_area_thresh_2nd_pass = (float) dtemp;

   else
      High_refl_area_thresh_2nd_pass = 10;

   /* Low Reflectivity Area increase from last scan. */
   if( DEAU_get_values( "alg.avset.area_increasee", &dtemp, 1 ) >= 0 )
      Area_increase = (float) dtemp;

   else
      Area_increase = 12;

   /* Maximum time difference between scans. */
   if( DEAU_get_values( "alg.avset.max_time_diff", &dtemp, 1 ) >= 0 )
      Max_time_difference = (int) (dtemp * 60);

   else
      Max_time_difference = 720;

   /* Maximum time difference between scans. */
   if( DEAU_get_values( "alg.avset.elev_tolerance", &dtemp, 1 ) >= 0 )
      Elev_tolerance = (int) round( (dtemp * 10.0) );

   else
      Elev_tolerance = 3;

   /* Initialize the Terminate_cut flag and area sums. */
   Terminate_cut = 0;
   Area_low_refl = 0.0;
   Area_high_refl = 0.0;

   LE_send_msg( GL_INFO, "AVSET Parameters:\n" );
   LE_send_msg( GL_INFO, "-->High Reflectivity Thresholds ... Z: %f dBZ, Area: %f km^2\n",
                high_ref, High_refl_area_thresh );
   LE_send_msg( GL_INFO, "-->High Reflectivity Thresholds (2nd Pass)... Z: %f dBZ, Area: %f km^2\n",
                high_ref_2nd_pass, High_refl_area_thresh_2nd_pass );
   LE_send_msg( GL_INFO, "-->Low Reflectivity Thresholds ... Z: %f dBZ, Area: %f km^2\n",
                low_ref, Low_refl_area_thresh );
   LE_send_msg( GL_INFO, "-->Low Reflectivity Thresholds (2nd Pass)... Z: %f dBZ, Area: %f km^2\n",
                low_ref_2nd_pass, Low_refl_area_thresh_2nd_pass );
   LE_send_msg( GL_INFO, "-->Area Increase: %f km^2\n", Area_increase );
   LE_send_msg( GL_INFO, "-->Elevation Tolerance (2nd Pass): %f deg\n", 
                (float) Elev_tolerance/10.0 );
   LE_send_msg( GL_INFO, "-->Maximum Time Difference: %d min\n", Max_time_difference/60 );

/* End of AVSET_read_adaptation_data() */
}

/*\///////////////////////////////////////////////////////////////////////////////

   Description:
      Compute area in support of AVSET.

///////////////////////////////////////////////////////////////////////////////\*/
int AVSET_compute_area( char *rpg_basedata ){

   Base_data_header *rpg_hd = (Base_data_header *) rpg_basedata;
   short *ref = NULL;
   int i;

   static float *lookup = NULL;

   /* At start of volume, initialize data. */
   if( rpg_hd->status == GOODBVOL )
      AVSET_initialize_this_volume( rpg_hd );

   /* If AVSET not enabled, nothing to do. */
   if( !Avset_enabled )
      return 0;

   /* If elevation angle less than 5.0 deg, nothing to do. */
   if( rpg_hd->target_elev < 50 )
      return 0;

   /* If there are no reflectivity bins in this radial, nothing to do. */
   if( rpg_hd->n_surv_bins == 0 )
      return 0;

   /* At start of elevation, reset the area accumulators. */
   if( rpg_hd->status == GOODBEL ){

      Area_low_refl = 0.0;
      Area_high_refl = 0.0;
      Area_low_refl_2nd_pass = 0.0;
      Area_high_refl_2nd_pass = 0.0;
       
      /* Set the table address depending on resolution of data. */
      if( rpg_hd->surv_bin_size == 250 )
         lookup = (float *) &Bintbl_sr[0];

      else
         lookup = (float *) &Bintbl[0];

   }

   /* Process the reflectivity data. */
   ref = (short *) (rpg_basedata + rpg_hd->ref_offset);
   for( i = 0; i < rpg_hd->n_surv_bins; i++ ){

      if( ref[i] >= Low_refl_thresh )
         Area_low_refl += lookup[i];

      if( ref[i] >= High_refl_thresh )
         Area_high_refl += lookup[i];

      /* If second pass needed, then compute areas using 2nd pass 
         thresholds. */
      if( Current_AVSET_state == AVSET_2ND_PASS_STATE ){

         if( ref[i] >= Low_refl_thresh_2nd_pass )
            Area_low_refl_2nd_pass += lookup[i];

         if( ref[i] >= High_refl_thresh_2nd_pass )
            Area_high_refl_2nd_pass += lookup[i];

      }

   }

   /* Compare area accumulations to area thresholds. */
   if( (rpg_hd->status == GENDEL) 
                   || 
       (Terminate_cut && (rpg_hd->status == GENDVOL)) ){

      int cut;

      Area_low_refl /= 100.0;
      Area_high_refl /= 100.0;
      Area_low_refl_2nd_pass /= 100.0;
      Area_high_refl_2nd_pass /= 100.0;

      /* Update the area information data. */
      cut = Area_info[Area_info_ind].num_cuts;
      Area_info[Area_info_ind].area[cut].target_elev = rpg_hd->target_elev;
      Area_info[Area_info_ind].area[cut].area = Area_low_refl;
      Area_info[Area_info_ind].num_cuts++;

      /* Test area thresholds.  Test dependent on current AVSET state. */
      AVSET_test_thresholds( rpg_hd );

   }

   return 0;

/* End of AVSET_compute_area(). */
}


/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Find closest match between current cut and cut in previous volume
      scan.

   Inputs:
      rpg_hd - radial header.

   Return: 
      Area for low reflectivity threshold.  Value can be -1.0 if no
      cut found that matches.

/////////////////////////////////////////////////////////////////////////\*/
float AVSET_find_closest_cut( Base_data_header *rpg_hd ){

   int prev_ind, elev_tol, min, min_ind, i;
   float prev_area_low_refl = -1.0;

   /* Determine the Area Information index for the previous volume 
      scan. */
   if( Area_info_ind == 0 )
      prev_ind = 1;

   else
      prev_ind = 0;

   /* Set the elevation tolerance for the 2nd pass processing.   If
      the VCPs are identical, there is no tolerance.   Otherwise set
      the value according to adaptation data. */
   if( rpg_hd->vcp_num == Area_info[prev_ind].vcp_num )
      elev_tol = 0;

   else
      elev_tol = Elev_tolerance;

   /* Cycle through elevations in previous volume, looking for 
      minimum elevation difference. */
   min = 100;
   min_ind = -1;
   for( i = 0; i < Area_info[prev_ind].num_cuts; i++ ){

      int diff = rpg_hd->target_elev - Area_info[prev_ind].area[i].target_elev;

      /* If difference less than current minimum, save minimum and index of of 
         elevation. */
      if( abs(diff) < min ){

         min = diff;
         min_ind = i;

      }

   }

   /* Check if the minimum elevation difference is less than threshold. */
   if( (min_ind >= 0) && (min < Elev_tolerance) )
      prev_area_low_refl = Area_info[prev_ind].area[min_ind].area;

   /* Return the area and the index for the previous volume scan. */
   return( prev_area_low_refl );

/* End of AVSET_find_closest_cut(). */
}

/*\///////////////////////////////////////////////////////////////////////

   Description:
      Initialization to be performed at the start of volume scan.

   Inputs:
      rpg_hd - radial header.

   Returns:
      There is no return value defined for this funciton.

///////////////////////////////////////////////////////////////////////\*/
void AVSET_initialize_this_volume( Base_data_header *rpg_hd ){

   /* Initialize time. */
   static int first_time = 1;

   /* One time initialization.    Set the current volume scan time. */
   if( first_time ){
   
      Current_volume_time = MISC_systime(NULL);
      first_time = 0;

   }
   
   /* Initialize Terminate Cut flag and area accumulators. */
   Terminate_cut = 0;
   Area_low_refl = 0.0; 
   Area_high_refl = 0.0;
   Area_low_refl_2nd_pass = 0.0;
   Area_high_refl_2nd_pass = 0.0;

   /* Initialize the area information data. */
   Area_info_ind = PBD_volume_scan_number % 2;
   Area_info[Area_info_ind].vcp_num = rpg_hd->vcp_num;
   Area_info[Area_info_ind].num_cuts = 0;
   memset( Area_info[Area_info_ind].area, 0, sizeof(Cut_info_t)*MAX_CUTS );

   /* Set the AVSET state to the initial state. */
   Current_AVSET_state = AVSET_INITIAL_STATE;

   /* Set the previous volume scan time to the current volume volume scan
      time.   Get the new volume scan time.   Note:   We use the time
      to determine time interval .... therefore the actual time is not
      important, only the time difference. */
   Previous_volume_time = Current_volume_time;
   Current_volume_time = MISC_systime(NULL);

/* End of AVSET_initialize_this_volume(). */
}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Tests computed areas against thresholds.   Sets the AVSET state according
      to test outcome.

   Inputs:
      rpg_hd - radial header.

   Returns:
      There is no return value defined for this function.

////////////////////////////////////////////////////////////////////////////\*/
void AVSET_test_thresholds( Base_data_header *rpg_hd ){

   float prev_area_low_refl;

   /* Process dependent on current AVSET state. */
   switch( Current_AVSET_state ){

      case AVSET_INITIAL_STATE:
      default:
      {

         /* Check areas against area thresholds. */
         if( (Area_low_refl <= Low_refl_area_thresh)
                             &&
             (Area_high_refl <= High_refl_area_thresh) ){

            /* Find closest cut in previous volume scan to the current cut.  We
               need the area of significant reflectivity for this cut to compare
               with the current cut. */
            prev_area_low_refl = AVSET_find_closest_cut( rpg_hd );

            /* If previous volume scan area for low reflectivity is undefined
               or the time difference between volume scans is too large,
               a second pass is not required. */
            if( ((Current_volume_time - Previous_volume_time) >= Max_time_difference)
                                   ||
                         (prev_area_low_refl < 0 ) ){

               Terminate_cut = 1;
               LE_send_msg( GL_INFO, "AVSET Terminating Cut. \n" );

            }
            else{

               /* Check if area of low threshold has increased by more than 
                  an adaptable number of square kilometers.  If so, set new 
                  AVSET state, otherwise set the Terminate Cut flag. */
               if( (prev_area_low_refl >= 0) 
                             &&
                   (Area_low_refl - prev_area_low_refl) >= Area_increase ){

                  Current_AVSET_state = AVSET_2ND_PASS_STATE;
                  LE_send_msg( GL_INFO, "AVSET Initiating 2nd Pass\n" );

               }
               else{

                  Terminate_cut = 1;
                  LE_send_msg( GL_INFO, "AVSET Terminating Cut. \n" );

               }

            }

            /* Write Information to task log. */
            LE_send_msg( GL_INFO, "-->Elevation Angle: %4.1f deg, RDA Cut #: %d\n",
                         rpg_hd->elevation, rpg_hd->elev_num );
            LE_send_msg( GL_INFO, "-->Low Refl Area: %9.2f km^2, High Refl Area: %9.2f km^2\n",
                         Area_low_refl, Area_high_refl );

         }

         break;

      }
      
      case AVSET_2ND_PASS_STATE:
      {
      
         /* Check areas against area thresholds. */
         if( (Area_low_refl_2nd_pass <= Low_refl_area_thresh_2nd_pass)
                             &&
             (Area_high_refl_2nd_pass <= High_refl_area_thresh_2nd_pass) ){

            Terminate_cut = 1;
            LE_send_msg( GL_INFO, "AVSET Terminating Cut ... 2nd Pass. \n" );

         }

         /* Write information to task log. */
         LE_send_msg( GL_INFO, "-->Elevation Angle: %4.1f deg, RDA Cut #: %d\n",
                      rpg_hd->elevation, rpg_hd->elev_num );
         LE_send_msg( GL_INFO, "-->2nd Pass ... Low Refl Area: %9.2f km^2, High Refl Area: %9.2f km^2\n",
                      Area_low_refl_2nd_pass, Area_high_refl_2nd_pass );

         break;

      }

   } /* End of switch(). */

/* End of AVSET_test_thresholds(). */
}
#endif

#ifdef SNR_TEST
/*\////////////////////////////////////////////////////////////////////////

   Description:
      Builds the -20log10(2) and DBZ lookup table used for SNR 
      thresholding. 

////////////////////////////////////////////////////////////////////////\*/
static void SNR_build_lookup_tables(){

    float r;
    int i;

    /* -20log10(R) table for 250 m bin size.  Assume the first bin 
        always starts at 0 km. */
    for( i = 0; i < MAX_BASEDATA_REF_SIZE; i++ ){

        r = 0.125 + (0.25* (float) i);
        PBD_log_range[i] = -20. * log10( r );

    }

    /* -20log10(R) for 1 km bin size.  Assume the first bin always 
       starts at 0 km. */
    for( i = 0; i < BASEDATA_REF_SIZE; i++ ){

        r = 0.5 + (float) i; 
        PBD_log_range_1km[i] = -20. * log10( r );

    }

    /* Data values 0 and 1 have special meaning.  Convert ICD units to
       (dBZ), i.e., Z (dBZ) = Z (ICD)/2 - 33.0. */
    for( i = 2; i < BASEDATA_INVALID; i++ )
        PBD_z_table[i] = ((float) i/2.0f) - 33.0;

    /* Set Below SNR and Range Folded to very low dBZ. */
    PBD_z_table[BASEDATA_RDBLTH] = MINIMUM_REFLECTIVITY;
    PBD_z_table[BASEDATA_RDRNGF] = MINIMUM_REFLECTIVITY;

} /* End of SNR_build_log_range_table(). */

/*\//////////////////////////////////////////////////////////////////////

   Description:
      Builds range correction tables to support SNR thresholding. 
      The table value is

        Range Correction = -syscal + (R*atmos) - 20*log10 (R) 

      where R is range to bin center, in km, and atmos is the 
      atmospheric attenuation rate in dB/km. 

/////////////////////////////////////////////////////////////////////\*/
static void SNR_build_range_corr_table( Base_data_header *rpg_hd ){

   static double atmos = NOT_INIT, syscal = NOT_INIT;
   double r;
   int i;

   /* Note:  Atmos can change with elevation. Syscal is assumed constant
             for the volume scan. (Refer to Message 31 in RDA/RPG ICD). */
   if( (Syscal == syscal)
              &&
       (Atmos == atmos) )
      return;

   LE_send_msg( GL_INFO, "Rebuilding Range Correction Table For Elevation: %8.4f\n",
                rpg_hd->elevation );
   LE_send_msg( GL_INFO, "--->Atmos: %10.4f, Syscal: %14.10f\n",
                Atmos, Syscal );
   LE_send_msg( GL_INFO, "------>dBZ0: %10.4f, Noise: %10.4f\n",
                rpg_hd->calib_const, rpg_hd->horiz_noise );

   /* First time buffer allocation for the Range Correction table
      for 250 m bin size.. */
   if( Range_corr == NULL )
       Range_corr = MISC_malloc( MAX_BASEDATA_REF_SIZE * sizeof (double) );

   for( i = 0; i < MAX_BASEDATA_REF_SIZE; i++ ){

       r = 0.125 + (0.25* (float) i); 
       Range_corr[i] = PBD_log_range[i] - Syscal + (r*Atmos);

   }

   /* First time buffer allocation for the Range Correction table 
      for 1 km bin size. */
   if( Range_corr_1km == NULL )
       Range_corr_1km = MISC_malloc( BASEDATA_REF_SIZE * sizeof (double) );

   for( i = 0; i < BASEDATA_REF_SIZE; i++ ){

       r = 0.5 + (float) i; 
       Range_corr_1km[i] = PBD_log_range_1km[i] - Syscal + (r*Atmos);

   }

   atmos = Atmos;
   syscal = Syscal;

} /* End of SNR_build_range_corr_table(). */

#define UNSIGNED_SHORT_BITS			16

/*\//////////////////////////////////////////////////////////////////////

   Description:
      Applies SNR thresholds to moment data. 

   Inputs:
      rpg_basedata - pointer to RPG internal base data radial.

   Notes:
      The following assumptions are made:
         1) The VCP defined SNR thresholds for V and W are the same.
         2) If V data is available, so is W data.

/////////////////////////////////////////////////////////////////////\*/
static void SNR_apply_snr_thresholds( char *rpg_basedata ){

    double tz[2] = { 0, 0 }, td = 0;
    int surv_bin_size = 1000;
    int start_bin, end_bin, i, j, k;

    Base_data_header *rpg_hd = (Base_data_header *) rpg_basedata;
    unsigned short *ref = NULL, *vel = NULL, *spw = NULL;
    unsigned short *sref = NULL;
    Generic_moment_t *save_mom = NULL;

    static unsigned short rtemp[MAX_BASEDATA_REF_SIZE]; 

    /* Assign pointers to moment fields. */ 
    if( rpg_hd->vel_offset != 0 )
        vel = (unsigned short *) (rpg_basedata + rpg_hd->vel_offset);

    if( rpg_hd->spw_offset != 0 )
        spw = (unsigned short *) (rpg_basedata + rpg_hd->spw_offset);

    if( rpg_hd->ref_offset != 0 )
        ref = (unsigned short *) (rpg_basedata + rpg_hd->ref_offset);

    /* Check for the existence of the supplemental reflectivity data. */
    if( rpg_hd->no_moments > 0 ){

        for( k = 0; k < rpg_hd->no_moments; k++ ){

            Generic_moment_t *mom = (Generic_moment_t *)
                                    (rpg_basedata + rpg_hd->offsets[k]);

            if( strncmp( mom->name, "DRF2", 4 ) == 0 ){

                /* The DRF2 field is produced by pbd and therefore is 
                   stored as unsigned chars.   We need to copy this data 
                   to temporary buffer since standard moments are stored 
                   as unsigned shorts. */

                int start = mom->first_gate_range/mom->bin_size;
                int end = start + mom->no_of_gates - 1;

                /* Initialize rtemp to Missing.  In the loop below,
                   we populate rtemp with the supplemental reflectivity
                   data. */
                memset( rtemp, 0, MAX_BASEDATA_REF_SIZE*sizeof(Moment_t) );

                /* Populate the temporary buffer with supplemental 
                   reflectivity buffer with having same word size as
                   standard reflectivity. */
                sref = rtemp;
                for( i = start, j = 0; i < end; i++, j++ )
                    sref[i] = (unsigned short) (mom->gate.b[j]);

            }

            /* Save the pointer to the supplement reflectivity so we can
               copy it back to the radial later. */
            save_mom = mom;

            break;

        } /* End of for( k = 0; ... ) loop. */

    } /* End of if( rpg_hd->no_moments > 0 )  */

    /* Determine effective signal power thresholds based on SNR thresholds.
       The sensitivity loss is effectively a rise to the Noise floor. */
    tz[QRTKM] = Noise + PBD_sensitivity_loss[QRTKM] + 
                 PBD_z_snr_threshold[PBD_current_elev_num-1];
    tz[ONEKM] = Noise + PBD_sensitivity_loss[ONEKM] + 
                 PBD_z_snr_threshold[PBD_current_elev_num-1];

    /* Thresholds based on waveform. */ 
    switch( PBD_waveform_type ){

        case VCP_WAVEFORM_CD:
        {
            /* For CD cut of split cut, the threshold for Doppler depends
               on whether the data is super res or not super res.  If not
               super res, td = MAX(Z SNR CS Cut, D SNR CD Cut) and power
               derived from CS Z must exceed td.

               For super res CD cut, the two-fold condition must be satisfied:

               CS cut Z > Z SNR from CS cut
               CD cut Z > Z SNR from CD cut.

               Here CD cut Z refers to the supplemental reflectivity data 
               used for recombination. */ 

            int sr = ORPGVCP_get_super_res( rpg_hd->vcp_num,
                                            PBD_current_elev_num-1 );

            /* Set the Doppler SNR threshold based on sensitivity loss and 
              SNR threshold from the VCP definition. */
            td = Noise + PBD_sensitivity_loss[QRTKM] +
                 PBD_d_snr_threshold[PBD_current_elev_num-1];

            /* We need to derive the Z SNR threshold based on the SNR threshold of
               the CS cut based on VCP definition.  Since this is a CD cut, we assume 
               the previous cut was a CS cut. */
            tz[QRTKM] = Noise + PBD_sensitivity_loss[QRTKM] + 
                         PBD_z_snr_threshold[PBD_current_elev_num-2];
            tz[ONEKM] = Noise + PBD_sensitivity_loss[ONEKM] + 
                         PBD_z_snr_threshold[PBD_current_elev_num-2];

            /* Not a super res cut. */
            if( !(sr & VCP_HALFDEG_RAD) || (sref == NULL) ){

                if( sr & VCP_QRTKM_SURV ){

                    if( tz[QRTKM] > td )
                        td = tz[QRTKM];

                }
                else {

                    if( tz[ONEKM] > td )
                        td = tz[ONEKM];

                }

            } 
            break;
        }

        case VCP_WAVEFORM_CS:
        case VCP_WAVEFORM_BATCH:
        case VCP_WAVEFORM_CDBATCH:
        default:
        {
            /* For most cases, the Doppler SNR comes directly from the VCP
               definition for that cut. */
            td = Noise + PBD_sensitivity_loss[QRTKM] + 
                 PBD_d_snr_threshold[PBD_current_elev_num-1];

            break;
        }

    } /* End of switch( PBD_waveform_type ) */

    /* Apply threshold to standard Z data. */
    if( ref != NULL  ){

        double z;

        /* Special processing for BATCH waveform. */
        if( rpg_hd->surv_bin_size == 1000 ){

            SNR_1km_reflectivity( rpg_hd, ref, vel, tz[ONEKM] );
            rpg_hd->surv_snr_thresh = (int) ((tz[ONEKM] - Noise)*8.0);

        }
        else{

            start_bin = rpg_hd->surv_range - 1;
            end_bin = start_bin + rpg_hd->n_surv_bins - 1;

            for( i = start_bin; i < end_bin; i++ ){

                /* Note: Standard Z data should not be range ambiguous. */
                if( (ref[i] > BASEDATA_RDRNGF) 
                           && 
                    (ref[i] < BASEDATA_INVALID) ){

                    z = PBD_z_table[ref[i]];

                    if( (z + Range_corr[i]) <= tz[QRTKM] )
                        ref[i] = BASEDATA_RDBLTH;

                } /* End of if (ref[i] > BASEDATA_RDRNGF) ..... ) */

            } /* End of for( i = start_bin; .... */

            /* Assign the SNR threshold in the radial header. */
            rpg_hd->surv_snr_thresh = (int) ((tz[QRTKM] - Noise)*8.0); 

            /* Apply speckle filter. */
            if( PBD_apply_speckle_filter )
                SNR_apply_speckle_filter( ref, start_bin, end_bin );

        } /* End of if( PBD_waveform_type == ... ) */

    } /* End of if( ref != NULL .... ) */

    /* Apply threshold to V and W data. */

    /* We need both reflectivity data and Doppler data to apply
       SNR thresholding. */
    if( (ref != NULL) && (vel != NULL) ){

        start_bin = rpg_hd->dop_range - 1;
        end_bin = start_bin + rpg_hd->n_dop_bins - 1;
        surv_bin_size = rpg_hd->surv_bin_size;

        for( i = start_bin; i < end_bin; i++ ){

            double z = 0.0, s, s1;
            double rng_corr = Range_corr[i];
            double thrz = 0.0;

            j = i;
            thrz = tz[QRTKM];
            if( surv_bin_size == 1000 ){

                j = (float) i*0.25;
                rng_corr = Range_corr_1km[j];
                thrz = tz[ONEKM];

            }

            /* Initialize the reflectivity value. */
            z = MINIMUM_REFLECTIVITY;

            /* Ensure there is a valid Z value to threshold V and W. */
            if( (ref[j] > BASEDATA_RDRNGF) 
                         && 
                (ref[j] < BASEDATA_INVALID) )
                z = PBD_z_table[ref[j]];

            /* Apply range correction to power and check against SNR threshold. */
            s = z + rng_corr;

            /* Threshold supplemental reflectivity.  This data has the same 
               range gate spacing as the Doppler data.  */ 
            s1 = s;
            if( sref != NULL ){

                /* Compute power from supplemental reflectivity. */
                if( (sref[i] > BASEDATA_RDRNGF) 
                            && 
                    (sref[i] < BASEDATA_INVALID) ){

                    s1 = PBD_z_table[sref[i]] + Range_corr[i];

                    if( s1 <= tz[QRTKM] )
                        sref[i] = BASEDATA_RDBLTH;

                }
                else if( sref[i] == BASEDATA_RDBLTH )
                   s1 = MINIMUM_REFLECTIVITY + Range_corr[i];

                if( s <= tz[QRTKM] )
                    sref[i] = BASEDATA_RDBLTH;
                                
                if( (ref[j] == BASEDATA_RDBLTH) 
                            && 
                    (sref[i] == BASEDATA_RDBLTH) ){

                    vel[i] = BASEDATA_RDBLTH;
                    spw[i] = BASEDATA_RDBLTH;

                }

            }

            /* Censor the velocity and spectrum width data if power is less than
               Doppler signal threshold. */
            if( (s <= td) || (s1 <= td) ){

                vel[i] = BASEDATA_RDBLTH;
                spw[i] = BASEDATA_RDBLTH;

            }

        } /* End of for( i == ... ) loop. */

        /* Assign SNR thresholds in radial header. */
        rpg_hd->vel_snr_thresh = (int) ((td - Noise)*8.0);
        rpg_hd->spw_snr_thresh = rpg_hd->vel_snr_thresh; 

        /* Apply speckle filter. */
        if( PBD_apply_speckle_filter ){

            SNR_apply_speckle_filter( vel, start_bin, end_bin );
            SNR_apply_speckle_filter( spw, start_bin, end_bin );

            if( sref != NULL ){

                SNR_apply_speckle_filter( sref, start_bin, end_bin );

                /* Restore the censored supplemental reflectivity data. */
                start_bin = save_mom->first_gate_range/save_mom->bin_size;
                end_bin = start_bin + save_mom->no_of_gates - 1;
                for( i = start_bin, j = 0; i < end_bin; i++, j++ )
                   save_mom->gate.b[j] = (unsigned char) sref[i]; 

            }

        }

    } /* End of if( ref != NULL ) && (vel != NULL) */

}/* End of SNR_apply_snr_thresholds(). */


#define MAX_SEQUENCE_NUM		65535
#define NUM_DATA_BLOCK_PTRS		    9

/*\/////////////////////////////////////////////////////////////////////////

   Description: 
      Convert internal RPG format to message 31 format.

   Inputs:
      buf - The RPG internal format message.

   Outputs:
      Message 31 format converted to external ICD format.
 
/////////////////////////////////////////////////////////////////////////\*/
static int SNR_convert_to_msg31( char *buf, unsigned char *msg31_buf ){

    RDA_RPG_message_header_t *msg_hdr;
    Generic_basedata_header_t *base_hdr;
    Generic_vol_t *vol_data;
    Generic_elev_t *elev_data;
    Generic_rad_t *rad_data;
    Generic_moment_t *ref_data;
    Generic_moment_t *vel_data;
    Generic_moment_t *spw_data;
    Base_data_header *m;
    int msg_size = 0;
    int temp_size = 0;
    int no_of_datum = 0;
    int word_align_bits = 0;
    int ref_bytes = 0;
    int vel_bytes = 0;
    int spw_bytes = 0;
    int waveform;
    int cut, i;
    Moment_t *data = NULL;

    static int sequence_num = 0;

    /* Cast buf to Base_data_header struct. */
    m = (Base_data_header *) buf;

    /* Fill msg31_buf with message header values. */
    msg_hdr = (RDA_RPG_message_header_t *) &msg31_buf[0];
    msg_hdr->rda_channel = 0;
    msg_hdr->type = GENERIC_DIGITAL_RADAR_DATA;
    msg_hdr->sequence_num = sequence_num;
    msg_hdr->julian_date=m->date;
    msg_hdr->milliseconds=m->time;
    msg_hdr->num_segs = 1;
    msg_hdr->seg_num = 1;
    msg_size+=sizeof(RDA_RPG_message_header_t);

    /* Increment sequence counter. */
    sequence_num = ((sequence_num+1)%MAX_SEQUENCE_NUM);

    /* Fill msg31_buf with generic basedata header values. */
    base_hdr = (Generic_basedata_header_t *) &msg31_buf[msg_size];
    strncpy( base_hdr->radar_id, m->radar_name, 4 );
    base_hdr->time = m->time;
    base_hdr->date = m->date;
    base_hdr->azi_num = m->azi_num;
    base_hdr->azimuth = m->azimuth;
    base_hdr->compress_type = 0;
    base_hdr->spare_17 = 0;
    base_hdr->azimuth_res = m->azm_reso;
    base_hdr->status = m->status;
    base_hdr->elev_num = m->elev_num;
    base_hdr->sector_num = m->sector_num;
    base_hdr->elevation = m->elevation;
    base_hdr->spot_blank_flag = m->spot_blank_flag;
    base_hdr->azimuth_index = m->azm_index;
    msg_size+=sizeof(Generic_basedata_header_t);

    /* Initialize data block pointers to zero. This will
       be filled in later with actual values. */
    memset( msg31_buf+msg_size, 0, NUM_DATA_BLOCK_PTRS*sizeof(int));
    msg_size+=NUM_DATA_BLOCK_PTRS*sizeof(int);

    /* Fill msg31_buf with volume data. */
    vol_data = (Generic_vol_t *) &msg31_buf[msg_size];
    memcpy( vol_data->type, "RVOL", 4 );
    vol_data->len = sizeof( Generic_vol_t );
    vol_data->major_version=1;
    vol_data->minor_version=0;
    vol_data->lat = m->latitude;
    vol_data->lon = m->longitude;
    vol_data->height = m->height;
    vol_data->feedhorn_height = m->feedhorn_height;
    vol_data->calib_const = m->calib_const;
    vol_data->horiz_shv_tx_power = m->horiz_shv_tx_power;
    vol_data->vert_shv_tx_power = m->vert_shv_tx_power;
    vol_data->sys_diff_refl = m->sys_diff_refl;
    vol_data->sys_diff_phase = m->sys_diff_phase;
    vol_data->vcp_num = m->vcp_num;
    vol_data->sig_proc_states = 0;
    msg_size+=sizeof(Generic_vol_t);

    /* Fill msg31_buf with elevation data. */
    elev_data = (Generic_elev_t *) &msg31_buf[msg_size];
    memcpy( elev_data->type, "RELV", 4 );
    elev_data->len = sizeof( Generic_elev_t );
    elev_data->atmos = m->atmos_atten;
    elev_data->calib_const = m->calib_const;
    msg_size+=sizeof(Generic_elev_t);

    /* Since this is recombined data, if the radial status is GOODBVOL
       and the elevation number is not 1, set the radial status to
       GOODBEL. */
    if( (base_hdr->status == GOODBVOL) && (base_hdr->elev_num != 1) )
       base_hdr->status = GOODBEL;

    /* Fill msg31_buf with radial data. */
    rad_data = (Generic_rad_t *) &msg31_buf[msg_size];
    memcpy( rad_data->type, "RRAD", 4 );
    rad_data->len = sizeof( Generic_rad_t );
    rad_data->unamb_range = m->unamb_range;
    rad_data->horiz_noise = m->horiz_noise;
    rad_data->vert_noise = m->vert_noise;
    rad_data->nyquist_vel = m->nyquist_vel;
    rad_data->spare = 0;
    msg_size+=sizeof(Generic_rad_t);

    /* Get the waveform type. */
    cut = (int) base_hdr->elev_num - 1;
    waveform = ORPGVCP_get_waveform( (int) vol_data->vcp_num, cut );

    /* Fill msg31_buf with reflectivity data (if present). */
    if( (m->n_surv_bins > 0) && (waveform != VCP_WAVEFORM_CD) ){

        ref_data = (Generic_moment_t *) &msg31_buf[msg_size];
        memcpy( ref_data->name, "DREF", 4 );
        ref_data->info = 0;
        ref_data->no_of_gates = m->n_surv_bins;
        ref_data->first_gate_range = m->range_beg_surv+(m->surv_bin_size/2);
        ref_data->bin_size = m->surv_bin_size;
        ref_data->tover = 0;
        ref_data->SNR_threshold = m->surv_snr_thresh;
        ref_data->control_flag = 3;
        ref_data->data_word_size = BYTE_MOMENT_DATA;
        ref_data->scale = 2.0;
        ref_data->offset = 66.0;

        data = (Moment_t *) (buf+m->ref_offset);
        for( i = 0; i < m->n_surv_bins; i++ )
            ref_data->gate.b[i] = data[i];

        word_align_bits = m->n_surv_bins%4;
        if( word_align_bits ){

            word_align_bits = 4-word_align_bits;
            memset( &ref_data->gate.b[m->n_surv_bins], 0, word_align_bits );

        }

        /* Determine number of bytes in data block. */
        ref_bytes = sizeof(Generic_moment_t)+m->n_surv_bins+word_align_bits;
        msg_size+=ref_bytes;

    }
    else if( waveform == VCP_WAVEFORM_CD ){

        /* Add Supplemental Reflectivity data if it exists. */
        if( m->no_moments > 0 ){

            for( i = 0; i < m->no_moments; i++ ){

                Generic_moment_t *mom = (Generic_moment_t *)
                                        (buf + m->offsets[i]);
                Generic_moment_t *tmom = NULL;
                int word_align_bytes = 0;

                /* Looking for Supplemental Reflectivity data. */
                if( strncmp( mom->name, "DRF2", 4 ) == 0 ){
                    
                    int msize = sizeof(Generic_moment_t) + mom->no_of_gates;

                    /* Copy the Supplemental Reflectivity data since it will be
                       needed for recombination. */
                    memcpy( &msg31_buf[msg_size], mom, msize );

                    /* Change the field name to "DREF" */
                    tmom = (Generic_moment_t *) &msg31_buf[msg_size];
                    memcpy( tmom->name, "DREF", 4 );

                    /* Adjust the message size for word alignment and exit loop. */
                    word_align_bytes = msize%4;
                    if( word_align_bytes ){

                        word_align_bytes = 4 - word_align_bytes;
                        memset( &tmom->gate.b[mom->no_of_gates], 0, word_align_bytes );

                        msize += word_align_bytes;

                    }

                    ref_bytes = sizeof(Generic_moment_t)+tmom->no_of_gates+word_align_bytes;
                    msg_size += msize;
                    break;

                }

            } 

        } 

    }

    /* Fill msg31_buf with velocity data (if present). */
    if( (m->n_dop_bins > 0) && (m->vel_offset != 0) ){

        vel_data = (Generic_moment_t *) &msg31_buf[msg_size];
        memcpy( vel_data->name, "DVEL", 4 );
        vel_data->info=0;
        vel_data->no_of_gates = m->n_dop_bins;
        vel_data->first_gate_range = m->range_beg_dop+(m->dop_bin_size/2);
        vel_data->bin_size = m->dop_bin_size;
        vel_data->tover = m->vel_tover;
        vel_data->SNR_threshold = m->vel_snr_thresh;
        vel_data->control_flag = 1;
        vel_data->data_word_size = BYTE_MOMENT_DATA;
        if( m->dop_resolution == 1 ) 
            vel_data->scale = 2.0; 
        else 
            vel_data->scale = 1.0; 
        vel_data->offset = 129.0;

        data = (Moment_t *) (buf+m->vel_offset);
        for( i = 0; i < m->n_dop_bins; i++ ){

            if( data[i] > 255 )
                vel_data->gate.b[i] = 255;
            else
                vel_data->gate.b[i] = data[i];
        }

        word_align_bits = m->n_dop_bins%4;
        if( word_align_bits ){

            word_align_bits = 4-word_align_bits;
            memset( &vel_data->gate.b[m->n_dop_bins], 0, word_align_bits );
        }

        /* Determine number of bytes in data block. */
        vel_bytes = sizeof(Generic_moment_t)+m->n_dop_bins+word_align_bits;
        msg_size+=vel_bytes;

    }

    /* Fill msg31_buf with spectrum data (if present). */
    if( (m->n_dop_bins > 0) && (m->spw_offset != 0) ){

        spw_data = (Generic_moment_t *) &msg31_buf[msg_size];
        memcpy( spw_data->name, "DSW ", 4 );
        spw_data->info = 0;
        spw_data->no_of_gates = m->n_dop_bins;
        spw_data->first_gate_range = m->range_beg_dop+(m->dop_bin_size/2);
        spw_data->bin_size = m->dop_bin_size;
        spw_data->tover = m->spw_tover;
        spw_data->SNR_threshold = m->spw_snr_thresh;
        spw_data->control_flag = 1;
        spw_data->data_word_size = BYTE_MOMENT_DATA;
        spw_data->scale = 2.0;
        spw_data->offset = 129.0;

        data = (Moment_t *) (buf+m->spw_offset);
        for( i = 0; i < m->n_dop_bins; i++ )
            spw_data->gate.b[i] = data[i];

        word_align_bits = m->n_dop_bins%4;
        if( word_align_bits ){

            word_align_bits = 4-word_align_bits;
            memset( &spw_data->gate.b[m->n_dop_bins], 0, word_align_bits );
        }

        /* Determine number of bytes in data block. */
        spw_bytes = sizeof(Generic_moment_t)+m->n_dop_bins+word_align_bits;
        msg_size+=spw_bytes;

    }



    /* Set variables we didn't know until now. */
    msg_hdr->size = msg_size/sizeof( short );
    base_hdr->radial_length = msg_size - sizeof(RDA_RPG_message_header_t);

    /* Set data block pointers. */
    temp_size = sizeof(Generic_basedata_header_t)+(NUM_DATA_BLOCK_PTRS*sizeof (int));
    base_hdr->data[0] = temp_size;
    temp_size+= sizeof (Generic_vol_t);
    base_hdr->data[1] = temp_size;
    temp_size+= sizeof (Generic_elev_t);
    base_hdr->data[2] = temp_size;
    temp_size+= sizeof (Generic_rad_t);

    /* Reset value to account for variable number of moments. */
    no_of_datum = 3; /* vol, elev, rad */
    if( ref_bytes > 0 ){

        base_hdr->data[no_of_datum] = temp_size;
        no_of_datum++;
        temp_size+= ref_bytes;
    }

    if( vel_bytes > 0 ){

        base_hdr->data[no_of_datum] = temp_size;
        no_of_datum++;
        temp_size+= vel_bytes;
    }

    if( spw_bytes > 0 ){

        base_hdr->data[no_of_datum] = temp_size;
        no_of_datum++;
        temp_size+= spw_bytes;
    }

    base_hdr->no_of_datum = no_of_datum;

    /* Byte-swap message header. */
    UMC_RDAtoRPG_message_header_convert( (char *) msg31_buf );

    /* Byte-swap rest of message. */
    UMC_RPGtoRDA_message_convert_to_external( GENERIC_DIGITAL_RADAR_DATA,
                                              (char *) msg31_buf);

    /* Return the size in bytes. */
    return( msg_size );

} /* End of SNR_convert_to_msg31(). */

/*\/////////////////////////////////////////////////////////////////////////////////

   Description:
      Apply speckle filter .... if( (data[i-/+1] == 0) And data[i] != 0 )
      set data[i] = 0.

   Inputs:
      data - data array.
      start_bin - starting bin index.
      end_bin - ending bin index. 

////////////////////////////////////////////////////////////////////////////////\*/
static void SNR_apply_speckle_filter( unsigned short *data, int start_bin, 
                                      int end_bin ){

    int i;

    /* Do For All bins (but ignoring the first and last bin). */
    for( i = start_bin + 1; i < end_bin - 1; i++ ){

        /* If the bins on either side of data[i] are below threshold, mark
           data[i] below threshold. */
        if( (data[i-1] == BASEDATA_RDBLTH) 
                       && 
            (data[i+1] == BASEDATA_RDBLTH) ){

            if( data[i] != BASEDATA_RDBLTH )
               data[i] = BASEDATA_RDBLTH;

        }

    }

} /* End of SNR_speckle_filter(). */
          

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Used for 1 km reflectivity thresholding.  The following psuedocode
      describes the processing:

      + Convert 1 km Z to 1/4 km Z by replicating 4 times.
      + Threshold 1/4 km Z on SNR.
      + Apply speckle filter on 1/4 km Z.
      + Average power over 4 1/4 km bins to derive 1 km Z.
         + if 1/4 km Z < SNR, power contribution to average is 0.
      + Threshold 1 km Z on SNR.

   Inputs:
      bhd - radial header.
      ref - pointer to reflectivity data.
      vel - pointer to velocity data.
      tz - surveillance power threshold, in dB.

   Returns:
      There is no return value defined for this function.

//////////////////////////////////////////////////////////////////////////\*/
static void SNR_1km_reflectivity( Base_data_header *bhd, unsigned short *ref,
                                  unsigned short *vel, double tz ){

    int start_bin, end_bin, v_start_bin, v_end_bin, i, j, k;
    double s, z;

    static unsigned short sref[MAX_BASEDATA_REF_SIZE];

    if( bhd->surv_bin_size == 250 )
        return;

    /* Set all bins to below threshold. */
    memset( sref, 0, MAX_BASEDATA_REF_SIZE*sizeof(unsigned short) );

    /* Convert 1 km reflectivity data to 0.25 ref data by replicating bins. */
    start_bin = bhd->surv_range - 1;
    end_bin = start_bin + bhd->n_surv_bins - 1;
    for( i = start_bin; i < end_bin; i++ ){

        k = i*4;
        sref[k] = ref[i];
        sref[k+1] = ref[i];
        sref[k+2] = ref[i];
        sref[k+3] = ref[i];

    }       

    /* Now censor the data. */
    v_start_bin = bhd->dop_range - 1;
    v_end_bin = v_start_bin + bhd->n_dop_bins - 1;
    for( i = v_start_bin; i < v_end_bin; i++ ){

        /* Note: Standard Z data should not be range ambiguous. */
        if( (sref[i] > BASEDATA_RDRNGF)
                   &&  
            (sref[i] < BASEDATA_INVALID) ){

            z = PBD_z_table[sref[i]];

            if( (vel != NULL) && (vel[i] <= BASEDATA_RDRNGF) )
                z -= 0.5;

            s = z + Range_corr[i];

            if( s < tz )
                sref[i] = BASEDATA_RDBLTH;

        } /* End of if (ref[i] > BASEDATA_RDRNGF) ..... ) */

    } /* End of for( i = start_bin; .... */

    /* Apply speckle filter. */
    if( PBD_apply_speckle_filter )
        SNR_apply_speckle_filter( sref, v_start_bin, v_end_bin );

    /* Convert 1/4 km reflectivity back to 1 km reflectivity. */
    for( i = start_bin; i < (end_bin - 1); i++ ){

        double p, sum = 0.0;

        /* 1/4 km bin index. */ 
        k = i*4;

        /* Sum the powers over 4 bins.  If reflectivity is missing,
           assume power is 0. */
        for( j = k; j <= (k + 3); j++ ){

            /* Power is valid ... i.e., not missing. */
            if( sref[j] > BASEDATA_RDBLTH ){

                z = PBD_z_table[sref[j]] + Range_corr[j];
                sum += exp10( z/10.0 ); 

            }            

        }

        /* Average power over 4 bins, then convert to dB. */
        p = sum/4.0;

        /* The log of 0 is -inf number.  In this case, assign p to the
           tz threshold -1 to ensure p is always less than tz. */
        if( p > 0.0 )
            p = 10.0*log10(p);

        else
            p = tz - 1.0;

        /* Do one last SNR thresholding. */
        if( p < tz )
            ref[i] = BASEDATA_RDBLTH;

        else{

            /* Convert power in dB to Z in dB, then to ICD units. */
            ref[i] = (unsigned short) round(2.0 *((p - Range_corr_1km[i]) + 32.0)) + 2;
            if( ref[i] <= BASEDATA_RDRNGF )
                ref[i] = BASEDATA_RDBLTH;

            else if( ref[i] >= BASEDATA_INVALID )
                ref[i] = 255;
        }

    }

} /* End of SNR_1km_reflectivity(). */

#endif
