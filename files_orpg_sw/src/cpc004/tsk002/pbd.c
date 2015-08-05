/*******************************************************************

	Main module for pre-processing the RDA base data and creating 
	the RPG base data.

*******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/09 23:01:40 $
 * $Id: pbd.c,v 1.251 2014/12/09 23:01:40 steves Exp $
 * $Revision: 1.251 $
 * $State: Exp $
 */  

#define GLOBAL_DEFINED
#include <pbd.h>

#define SOV_MSG_SIZE		  128
#define MAX_STATUS_LENGTH          64
#define BITS_IN_SHORT		   16

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


static int Generation_tables_set;	/* Flag, when set, indicates product generation
                                           has completed set-up of master generation
                                           list of products to be generated. */

static int Rda_control_command_received; /* Flag, when set, indicates an RDA command
                                            command was received. */

static int Rda_control_command_lbfd;	/* File descriptor of the RDA Control Command LB. */

static int Volume_status_lbfd;          /* LB fd for the ORPGDAT_GSM_DATA. */

static int RPG_info_lbfd;		/* LB fd for the ORPGDAT_RPG_INFO. */

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

static int Initial_volume = -1;

static int Site_specific_vcp = 0;

static char Task_name[ORPG_TASKNAME_SIZ];


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
static int Service_vcp_download( Rda_cmd_t *rda_command );
static int Update_current_vcp_table( int vcp_num, Vcp_struct *vcp,
                                     Vcp_struct **download_vcp );
static int Write_gsm( char* gsm );
static char* Legacy_to_generic( char *rda_msg );
static void Check_sig_proc_states( Base_data_header *rpg_hd );
static void Process_sig_proc_state( char **buf, int *len, char *field_id,
                                    char *field_val );

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
  
    int n_sails_cuts, retval;
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
    PBD_last_commanded_vcp = -1;
    PBD_split_cut = 0;

    PBD_is_rda = PBD_IS_REAL_RDA;
    PBD_avset_status = AVSET_DISABLED;
    PBD_last_elevation_angle = PBD_UNDEF_LAST_ELEV_ANG;
    PBD_last_ele_flag = 0;

    PBD_use_locally_defined_VCP = 1;
    PBD_supplemental_scan_vcp = 0;
    PBD_sails_enabled = 0;
    PBD_N_sails_cuts = PBD_DEFAULT_N_SAILS_CUTS;
    PBD_N_sails_cuts_this_vol = 0;
    PBD_test_SZ2_PRF_selection = 0;
    PBD_sig_proc_states = 0xffff;

    /* For interference detection. */
    ID_init_interference_data( -1 );

    /* We initialize these to the value we expect them to be.
       It is possible that RDA Adaptation Data is not available
       when radials start to be processed because control_rda
       posts the adaptation data .... pbd does not explicitly
       processs this message. */
    PBD_vel_tover = 50.0f;
    PBD_spw_tover = 50.0f;

    /* Register for events. */
    PBD_DEBUG = 0;
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

    /* Tell RPG manager pbd is ready. */
    ORPGMGR_report_ready_for_operation();

    /* Wait for operational mode before continuing with initialization. */
    if( ORPGMGR_wait_for_op_state( (time_t) 120 ) < 0 )
       LE_send_msg( GL_ERROR, "Waiting For RPG Operational State TIMED-OUT\n" );

    else
       LE_send_msg( GL_INFO, "The RPG is Operational\n" );

    /* Check the ORPG_NONOPERATIONAL environmental variable.  If set, then
       set PBD_is_rda to false. */
    if( !ORPGMISC_is_operational() ){

       PBD_is_rda = PBD_IS_PLAYBACK;
       LE_send_msg( GL_INFO, "PBD: Playback\n" );

    }

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

    /* Post the initial SAILS state. */
    if( (retval = ORPGSAILS_get_status() ) < 0 )
        LE_send_msg( GL_INFO, "ORPGSAILS_get_status() Failed: %d\n", retval );

    else{
 
        int max_site = ORPGSAILS_get_site_max_cuts();

        if( retval != 0 )
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "SAILS State=ON\n" );

        else
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "SAILS State=OFF\n" );

        LE_send_msg( GL_INFO, "The Site Max For SAILS Cuts is: %d\n", max_site );

    }

    /* Do an initial read for the number of sails cuts allowed.  The 
       default is 1 if the following function call fails. */
    if( (n_sails_cuts = ORPGSAILS_get_req_num_cuts()) < 0 ){

       LE_send_msg( GL_ERROR, "ORPGSAILS_get_req_num_cuts() Failed\n" );
       PBD_N_sails_cuts = PBD_DEFAULT_N_SAILS_CUTS;

    }    
    else
       PBD_N_sails_cuts = n_sails_cuts;

    LE_send_msg( GL_INFO, "Initial Value --> PBD_N Sails Cuts: %d\n", PBD_N_sails_cuts );

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
                  msg_id = ORPGDA_get_msg_id();

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

               /* Get message type. */
               msg_header = (RDA_RPG_message_header_t *) radar_data;
               msg_type = (int) (msg_header->type);

               /* Set the RDA Configuration if the new message changes the 
                  configuration.  Note: Some messages can have a message code
                  of 0 which means these are invalid messages.  Should only 
                  see these with Level 2 playback.  Ignore invalid messages. */
               if( msg_type > 0 )
                  ORPGRDA_set_rda_config( (void *) radar_data );

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
                        (msg_type == PERFORMANCE_MAINTENANCE_DATA) 
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

           PBD_avset_status = rda_status->avset;
           if( (PBD_avset_status != AVSET_ENABLED)
                                 &&
               (PBD_avset_status != AVSET_DISABLED) )
              PBD_avset_status = AVSET_DISABLED;

           Site_specific_vcp = 0;
           if( rda_status->vcp_num > 0 ){
     
              /* This is an Remote (RPG defined) VCP. */
              if( ORPGVCP_is_vcp_site_specific( (int) rda_status->vcp_num, 
                                                ORPGVCP_RPG_DEFINED_VCP ) > 0 )
                 Site_specific_vcp = 1;

           }
           else{
          
              /* This is a Local (RDA defined) VCP. */
              if( ORPGVCP_is_vcp_site_specific( (int) rda_status->vcp_num, 
                                                ORPGVCP_RDA_DEFINED_VCP ) > 0 )
                 Site_specific_vcp = 1;

           }

#ifdef AVSET_TEST
           /* If AVSET is enabled, then set the RDA Status flag to indicate AVSET
              is enabled. */
           if( Avset_enabled )
               PBD_avset_status = AVSET_ENABLED;
#endif
           LE_send_msg( GL_INFO, "Processing RDA Status Message ... \n" );
           LE_send_msg( GL_INFO, "--->RDA Status: %d, Data Trans Enbld: %u, Avset: %d\n",
                        RDA_status, PBD_data_trans_enabled, PBD_avset_status );

        }
        else if( msg_type == RDA_RPG_VCP ){

           /* Process the RDA/RPG VCP message. */
           LE_send_msg( GL_INFO, "Processing RDA/RPG VCP Message ...\n" );
           PD_process_rda_vcp_message( radar_data );
        
        }
        else if( msg_type == PERFORMANCE_MAINTENANCE_DATA ){

           /* Extract fields from PMD to support detection of interference
              radials. */
           LE_send_msg( GL_INFO, "Processing RDA Perf/Maint Data\n" );
           ID_process_rda_perf_maint_msg( radar_data );

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

    int i, sails_cut, return_value, vol_aborted, unexpected_bov;
    int p_indx, elev1_indx, elevation_indx, elev1, elevation;
    time_t current_time = 0, previous_time = 0, wait_time;
    double dtemp = 0;

    static orpgevt_scan_info_t scan_data;
    static orpgevt_start_of_volume_t sov_data;
    static int rpg_num_elev_cuts = 0;
    static int last_elevation_angle = PBD_UNDEF_LAST_ELEV_ANG;

    static char sov_message[SOV_MSG_SIZE];

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
    last_elevation_angle = PBD_last_elevation_angle;
    if( PH_process_header( gbd, rpg_basedata ) < 0 )
	return (-1);

    /* Move digital radar data to the RPG basedata message. */
    if( PD_move_data( gbd, rpg_basedata ) < 0 )
	return (-1);

    /* Are we at beginning of elevation or volume? */
    if( (rpg_hd->status == GOODBEL) || (rpg_hd->status == GOODBVOL) ){

       int p_n_sails_cuts = PBD_N_sails_cuts;
       short vcp_flag = ORPGVCP_get_vcp_flags( rpg_hd->vcp_num );

       /* Read adaptation data to determine the number of SAILS cut occurrences
          need to be added.  The default is 1. */
       if( (PBD_N_sails_cuts = ORPGSAILS_get_req_num_cuts()) < 0 ){

          LE_send_msg( GL_ERROR, "ORPGSAILS_get_req_num_cuts() Failed\n" );
          PBD_N_sails_cuts = PBD_DEFAULT_N_SAILS_CUTS;

       }

       if( vcp_flag & VCP_FLAGS_ALLOW_SAILS ){

          int max_n_sails_cuts = 
              (vcp_flag & VCP_MAX_SAILS_CUTS_MASK) >> VCP_MAX_SAILS_CUTS_SHIFT;

          if( PBD_N_sails_cuts > max_n_sails_cuts ){

             LE_send_msg( GL_INFO, "PBD_N_sails_cuts Truncated to %d From %d For VCP: %d\n", 
                          max_n_sails_cuts, PBD_N_sails_cuts, rpg_hd->vcp_num );
             PBD_N_sails_cuts = max_n_sails_cuts;

          }

       }

       /* This should not happen ... if PBD_N_SAILS_cuts is 0, then
          PBD_sails_enabled should be 0. */
       if( (PBD_N_sails_cuts == 0) && (PBD_sails_enabled) ){

          LE_send_msg( GL_INFO, "PBD_N_sails_cuts == 0 but SAILS Enabled\n" );
          PBD_sails_enabled = 0;

       }

       if( PBD_sails_enabled && (PBD_N_sails_cuts != p_n_sails_cuts) )
          LE_send_msg( GL_INFO, "N Sails Cuts: %d\n", PBD_N_sails_cuts );

       /* Set scan summary data if beginning of volume or elevation. */
       SSS_set_scan_summary( rpg_hd->volume_scan_num, rpg_hd->weather_mode, 
                             rpg_num_elev_cuts, Vs_gsm.num_elev_cuts,
                             last_elevation_angle, rpg_hd, gbd );

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

             /* Output interference message which didn't get output for last volume */
             ID_output_interference_msg( (char *) rpg_hd );

          }

          /* Set the volume scan sequence number of this volume scan.
             Reset initial volume flag. */
          Vs_gsm.volume_number = PBD_volume_seq_number;
          Vs_gsm.volume_scan = scan_data.data.vol_scan_number;
          Vs_gsm.initial_vol = 0;
          Vs_gsm.super_res_cuts = 0;
          Vs_gsm.dual_pol_expected = 0;
          memset( &Vs_gsm.sails_cut_seq[0], 0, VCP_MAXN_CUTS );

          /* Based on the RDA VCP data, set the cuts we expect Super Resolution this
             volume scan. */
          
          /* First verify whether the stored VCP data pattern number matches the
             actual pattern number in operation. */
          if( PBD_rda_vcp_data.vcp_msg_hdr.pattern_number == Vs_gsm.vol_cov_patt ){

             elev1 = Vs_gsm.elevations[0];
             elev1_indx = Vs_gsm.elev_index[0];
             p_indx = elev1_indx;
             sails_cut = 0;

             /* Do For All elevations in the VCP. */
             for( i = 0; i < PBD_rda_vcp_data.vcp_elev_data.number_cuts; i++ ){

                /* Test the elevation to see whether the "super res" flag is set. */
                if( (PBD_rda_vcp_data.vcp_elev_data.data[i].super_res & VCP_HALFDEG_RAD) != 0 ){

                   /* "super_res" flag is set for this elevation cut.  Set the 
                       corresponding bit in the volume status. */
                   int rpg_cut = Vs_gsm.elev_index[i] - 1;
          
                   /* Since the "super_res_cuts" field is a short, we need
                      to limit the cut number to the lowest 16 cuts. */
                   if( rpg_cut < BITS_IN_SHORT ) Vs_gsm.super_res_cuts |= 1 << rpg_cut;

                }

                /* Test the elevation to see whether the "dual pol" flag is set. */
                if( (PBD_rda_vcp_data.vcp_elev_data.data[i].super_res & VCP_DUAL_POL_ENABLED) != 0 ){

                   /* "dual pol" flag is set for this elevation cut.  Set the 
                       word in the volume status. */
                   Vs_gsm.dual_pol_expected = 1;

                }

                /* Set the SAILS cuts sequence number. */
                if( PBD_N_sails_cuts_this_vol > 0 ){

                   /* Get the elevation angle/RPG elevation index of the current elevation. */
                   elevation_indx = Vs_gsm.elev_index[i];
                   elevation = Vs_gsm.elevations[i];

                   /* If the elevation angle matches the elevation angle of the first
                      cut and the RPG elevation index is not the same as the first 
                      cut, increment the SAILS cut sequence number. */
                   if( (elev1 == elevation)
                              && 
                       (elevation_indx >= p_indx) ){
  
                         /* Increment the SAILS cut sequence number when the 
                            RPG elevation index changes. */
                         if( elevation_indx > p_indx )
                            sails_cut++;

                         Vs_gsm.sails_cut_seq[i] = sails_cut;

                   }

                   /* Save previous index. */
                   p_indx = elevation_indx;
  
                }

             }
               
             /* Set the number of SAILS cuts.  The number of SAILS cuts should have been set
                based on the RDA VCP definition. */
             Vs_gsm.n_sails_cuts = PBD_N_sails_cuts_this_vol;
             LE_send_msg( GL_INFO, "Vs_gsm.n_sails_cuts: %d\n", PBD_N_sails_cuts_this_vol );

          }

          /* Set VCP Supplemental flags. */

          /* Avset enabled/disabled. */
          Vs_gsm.avset_term_ang = 0;
          if( PBD_avset_status == AVSET_ENABLED ){

             Vs_gsm.vcp_supp_data |= VSS_AVSET_ENABLED;
             Vs_gsm.avset_term_ang = last_elevation_angle; 

          }
          else
             Vs_gsm.vcp_supp_data &= ~VSS_AVSET_ENABLED;

          /* SAILS active/inactive for this VCP. */
          if( PBD_supplemental_scan_vcp )
             Vs_gsm.vcp_supp_data |= VSS_SAILS_ACTIVE;

          else
             Vs_gsm.vcp_supp_data &= ~VSS_SAILS_ACTIVE;

          /* Site-Specific VCP? */
          if( Site_specific_vcp )
             Vs_gsm.vcp_supp_data |= VSS_SITE_SPECIFIC_VCP;

          else
             Vs_gsm.vcp_supp_data &= ~VSS_SITE_SPECIFIC_VCP;

          /* Output the Signal Processing States if they have changed.  A 
             state value of 0xffff assumes undefined. */
          PBD_RxRN_state = 0;
          PBD_CBT_state = 0;
          if( PBD_sig_proc_states != 0xffff ){

             Check_sig_proc_states( rpg_hd );
             PBD_sig_proc_states = rpg_hd->sig_proc_states;

          }

          /* Check if RxRN is enabled. */
          if( PBD_RxRN_state )
             Vs_gsm.vcp_supp_data |= VSS_RXRN_ENABLED;

          else
             Vs_gsm.vcp_supp_data &= ~VSS_RXRN_ENABLED;

          /* Check if CBT is enabled. */
          if( PBD_CBT_state )
             Vs_gsm.vcp_supp_data |= VSS_CBT_ENABLED;

          else
             Vs_gsm.vcp_supp_data &= ~VSS_CBT_ENABLED;

          /* Set the flag for SAILS enabled/disabled.  If it is decided to set
             a bit in the GSM VCP supplemental data, we need to do it here. */
          PBD_sails_enabled = ORPGINFO_is_sails_enabled();

          /* Set the flag for Auto PRF active/inactive. */
          PBD_auto_prf = ORPGINFO_is_prf_select();

          LE_send_msg( GL_INFO, "PBD_sails_enabled: %d, PBD_auto_prf: %d\n", 
                       PBD_sails_enabled, PBD_auto_prf );

          /* Read adaptation data checking to see if SZ2 PRF Selection is Operational. */
          PBD_test_SZ2_PRF_selection = 0;
          if( (ret = DEAU_get_values( "alg.Nonoperational.test_sz2prf", &dtemp, 1 )) < 0 )
             LE_send_msg( GL_INFO,
                          "DEAU_get_values( alg.Nonoperational.test_sz2prf ) Failed (%d)\n",
                          ret );
          else
             PBD_test_SZ2_PRF_selection = (int) dtemp;

          /* Tell the operator if SZ2 PRF Selection allowed. */
          if( PBD_test_SZ2_PRF_selection )
             LE_send_msg( GL_INFO, "Test SZ2 PRF Selection Allowed\n" );

          /* Write out the volume-scan based general status message to
             linear buffer, then post event indicating start of volume
             scan. */
          Write_gsm( (char *) &Vs_gsm );

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

                memset( &sov_message[0], 0, SOV_MSG_SIZE );
                sprintf( &sov_message[0], 
                         "Vol: %d (Seq: %d) RDA Clock:%02d/%02d/%02d %02d:%02d:%02d VCP:%4d L2: %d", 
                         scan_data.data.vol_scan_number, PBD_volume_seq_number, month, day, 
                         year, hours, minutes, seconds, scan_data.data.vcp_number, version );

                if( Vs_gsm.dual_pol_expected )
                   strcat( &sov_message[0], " DP" );

                if( Vs_gsm.super_res_cuts )
                   strcat( &sov_message[0], " SR" );

                if( PBD_supplemental_scan_vcp )
                   strcat( &sov_message[0], " SAILS" );

                LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "%s\n", &sov_message[0] );

             }

             /* If this VCP has supplemental scans, set flag in ORPGEVT_START_OF_VOLUME
                amplification data. */
             sov_data.flags = 0;
             if( PBD_supplemental_scan_vcp ){

                LE_send_msg( GL_INFO, "Setting VCP Has Supplement Scans bit for SOV Event\n" );
                sov_data.flags |= SOV_VCP_SUPPL_SCANS;

             }

             /* Initialize the site data for determine Sun position. */
             ID_site_init( (char *) rpg_hd );

             /* Initialize data for interference detection. */
             ID_init_interference_data( scan_data.data.vcp_number );

             /* Clear the generation table set flag.  Let the product scheduler know  
                the start of volume has begun.   We need to wait at least some amount
                of time, otherwise product generation can occur before the routine
                scheduler is ready to handle the new volume's products. */
             Generation_tables_set = 0;
             ret = EN_post( ORPGEVT_START_OF_VOLUME, (void *) &sov_data, 
                            (size_t) ORPGEVT_START_OF_VOLUME_DATA_LEN, (int) 0 );

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

          /* Update the old VCP number and the last commanded VCP. */
          PBD_last_commanded_vcp = PBD_vcp_number;

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
          
          /* The following call checks if the VCP needs to be updated when
             SAILS is active. */
          HS_update_vcp( &Vs_gsm, last_elevation_angle );

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

       /* Reset the PBD_use_locally_defined_VCP to default. */
       PBD_use_locally_defined_VCP = 1;
       PBD_supplemental_scan_vcp = 0;

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
    else {

       /* Check whether SAILS Enabled/Disabled has changed. */
       if( PBD_RPG_info_updated ){

          int sails_state = 0;

          /* Clear the PBD_RPG_info_updated flag. */
          PBD_RPG_info_updated = 0;
          LE_send_msg( GL_INFO, "RPG INFO Updated ... check for SAILS state change.\n" );

          /* The following call checks if the VCP needs to be updated when
             SAILS is active. */
          sails_state = (int) ORPGINFO_is_sails_enabled();
          if( sails_state != PBD_sails_enabled )
             HS_update_vcp( &Vs_gsm, last_elevation_angle );
          
       }

    }

    /* Check for interference this radial. */
    ID_check_interference( (char *) rpg_hd );

    /* Output interference message at end of volume() */
    if( rpg_hd->status == GENDVOL )
       ID_output_interference_msg( (char *) rpg_hd );

    /* Perform radial accounting... i.e., track pertinent information
       associated with elevation scan or volume scan. */
    PH_radial_accounting( rpg_hd, (char *) gbd, &Vs_gsm );

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
   
   /* PBD DEBUG toggle.  Used for additional task log messages. */
   else if( evtcd == ORPGEVT_PBD_DEBUG ){

      PBD_DEBUG = 1;

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

   /*Check if the RPG Info has been updated. */
   else if( fd == RPG_info_lbfd ){

      LE_send_msg( GL_INFO, "PBD_RPG_info_updated\n" );
      PBD_RPG_info_updated = 1;

   }

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
    int vsn;

    PBD_verbose = 0;
    PBD_sun_spike_msgs = 0;
    Initial_volume = -1;
    Task_name[0] = '\0';

    err = 0;
    while ((c = getopt (argc, argv, "hl:vs:V:T:")) != EOF) {
	switch (c) {

            /* Change the size of the task log file. */
            case 'l':
                Log_file_nmsgs = atoi( optarg );
                if( Log_file_nmsgs < 0 || Log_file_nmsgs > 5000 )
                   Log_file_nmsgs = 2500;
                break;

            /* Change the verbose level. */
            case 'v':
               PBD_verbose++;
               break;

            /* Allow Sun Spike messages to RPG Status log. */
            case 's':
               PBD_sun_spike_msgs++;
               break;

            /* Allow the task to loaded as some other binary. */
            case 'T':
               strncpy( &Task_name[0], optarg, ORPG_TASKNAME_SIZ );
               break;

            case 'V':
               vsn = atoi( optarg );
               if( (vsn > 0) && (vsn <= 80) )
                  Initial_volume = vsn;
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
        printf ("       -v verbose mode\n" );
        printf ("\nNote: PBD Verbose can also be enabled/disabled by posting\n");
        printf ("      event ORPGEVT_PBD_DEBUG (110)\n");
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
   static Rda_cmd_t rda_command;

   /* Initialize the RDA Command Buffer. */
   memset( &rda_command, 0, sizeof(Rda_cmd_t) );

   /* Read and process commands we care about. */
   while(1){

      /* Read the RDA Commands Linear Buffer. */
      if( (ret = ORPGDA_read( ORPGDAT_RDA_COMMAND,
                              (char *) &rda_command,
                              (int) sizeof( Rda_cmd_t ),
                              LB_NEXT )) == LB_TO_COME ) break;
    
      /* An LB error occurred or the message is smaller than expectedr. */
      if( ret < sizeof(Rda_cmd_t) ){

         LE_send_msg( GL_INFO, "ORPGDAT_RDA_COMMAND LB Read Failed (%d)\n", ret );

         /* If error is LB_EXPIRED, seek to the first unread message. */
         if( ret == LB_EXPIRED )
            ORPGDA_seek( ORPGDAT_RDA_COMMAND, 0, LB_FIRST, NULL );

         /* Return to caller. */ 
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
               LE_send_msg( GL_INFO, "--->Restart Elev/VCP Commanded\n" );
               Commanded_restart( rda_command.param1 );

            }
        
         }
         else if( rda_command.param1 == CRDA_SELECT_VCP ){

            /* Don't bother saving the commanded VCP if the RDA is in LOCAL control. */
            if( PH_get_rda_status( RS_CONTROL_STATUS ) != RDA_CONTROL_LOCAL ){

               /* Interrogate the Select VCP command.  Save the pattern
                  number is something other than USE_REMOTE_PATTERN.  This
                  will be used later if SAILS is active and the VCP needs
                  to be updated based on a different AVSET termination angle. 
                  If a VCP number is being selected that is not the same as
                  the current VCP number, then the VCP should not be updated 
                  at the start of the last elevation angle when SAILS is 
                  active and AVSET has changed the termination angle. */
               if( rda_command.param2 != RCOM_USE_REMOTE_PATTERN ){

                  unsigned char flag = 0;

                  LE_send_msg( GL_INFO, "--->Select VCP (%d) Commanded\n", rda_command.param2 );
                  PBD_last_commanded_vcp = rda_command.param2;

                  /* Pause the Auto PRF function if Auto PRF is active. */
                  if( PBD_auto_prf 
                           && 
                      (rda_command.line_num == HCI_VCP_INITIATED_RDA_CTRL_CMD) ){
  
                     if( (ret = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_PRFSF_PAUSED,
                                                       ORPGINFO_STATEFL_SET,
                                                       &flag )) >= 0 )
                        LE_send_msg( GL_INFO, 
                                     "--->PRF Selection Paused Owing to Select VCP Command From HCI\n" );

                  }

               }
               else
                  LE_send_msg( GL_INFO, "--->Select VCP (%d) Commanded\n", rda_command.param2 );

            }

         }

      }
      else if( rda_command.cmd == COM4_WMVCPCHG ){

         double value = 0.0;

         /* Don't bother saving the commanded VCP if the RDA is in LOCAL control. */
         if( PH_get_rda_status( RS_CONTROL_STATUS ) != RDA_CONTROL_LOCAL ){

            /* On a weather mode change, we want to determine which pattern
               number is to be commanded. */
            if( rda_command.param1 == PRECIPITATION_MODE ){

               if( DEAU_get_values( "site_info.def_mode_A_vcp", &value, 1) > 0 ){

                  PBD_last_commanded_vcp = (int) value;
                  LE_send_msg( GL_INFO, "Commanded WX Change to VCP %d\n", PBD_last_commanded_vcp );

               }

            }
            else if( rda_command.param1 == CLEAR_AIR_MODE ){

              if( DEAU_get_values( "site_info.def_mode_B_vcp", &value, 1) > 0 ){

                 PBD_last_commanded_vcp = (int) value;
                 LE_send_msg( GL_INFO, "Commanded WX Change to VCP %d\n", PBD_last_commanded_vcp );

              }

            }

         }

      }
      else if( (rda_command.cmd == COM4_DLOADVCP)
                                &&
               (rda_command.line_num != PBD_INITIATED_RDA_CTRL_CMD) ){
            
         /* Service the VCP download command. */
         Service_vcp_download( &rda_command );

      }
      else{

         if( rda_command.line_num != PBD_INITIATED_RDA_CTRL_CMD )
            LE_send_msg( GL_INFO, "--->RDA Command Ignored\n" );
            LE_send_msg( GL_INFO, "------>cmd: %d, line_num: %d, param 1: param 2: param 3: %d\n", 
                    rda_command.cmd, rda_command.line_num, rda_command.param1, rda_command.param2, 
                    rda_command.param3 );

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

    /* Register the PBD_DEBUG event. */
    event_registered = EN_register( ORPGEVT_PBD_DEBUG, Event_handler );

    /* Error in event registration.  Write error message to task log file. */
    if( event_registered < 0 )
       LE_send_msg( GL_ERROR, 
                    "Unable to Register Event ORPGEVT_PBD_DEBUG (%d)\n",
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

   /* Register for Volume Status updates. */
   if( (ret = ORPGDA_UN_register( ORPGDAT_RPG_INFO, ORPGINFO_STATEFL_SHARED_MSGID,
                                  Lb_notify_handler )) < 0 ){

      LE_send_msg( GL_ERROR,
                   "RPG INFO LB Notification Failed (%d)\n", ret );
      ORPGTASK_exit( GL_EXIT_FAILURE );

   }

   PBD_RPG_info_updated = 0;
   RPG_info_lbfd = ORPGDA_lbfd( ORPGDAT_RPG_INFO );

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

   PBD_response_LB = ORPGCMI_rda_response();
   PBD_radial_out_LB = -1;

   /* Check if Task_name is not an empty string. */
   if( strlen( &Task_name[0] ) > 0 )
      strncpy( &task_name[0], &Task_name[0], ORPG_TASKNAME_SIZ );

   /* Get my task name .... this will be used to access the task table entry. */
   if( (ORPGTAT_get_my_task_name( (char *) &task_name[0], ORPG_TASKNAME_SIZ ) >= 0)
                                   &&
       ((task_entry = ORPGTAT_get_entry( (char *) &task_name[0] )) != NULL) ){

      /* Check for match on input data name. */
      if( (data_id = ORPGTAT_get_data_id_from_name( task_entry, "RESPONSE_LB" )) >= 0){

         PBD_response_LB = data_id;
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

      LE_send_msg( GL_ERROR, "RESPONSE_LB Not Defined\n" );
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

   /* If PBD_radial_out_LB not defined in TAT, there is nothing for pbd to do. */
   if( PBD_radial_out_LB < 0 ){

      LE_send_msg( GL_ERROR, "RAWDATA Not Defined\n" );
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
   else{

      if( Initial_volume > 0 )
         Vs_gsm.volume_number = Initial_volume;

      PBD_volume_seq_number = Vs_gsm.volume_number;

   }

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


/**********************************************************************************
   
   Description:
      Services the VCP download command. 

   Inputs:
      rda_command - RDA Command.

   Returns:
      Always returns 0.

**********************************************************************************/
static int Service_vcp_download( Rda_cmd_t *rda_command ){

   int param3 = rda_command->param3;

   LE_send_msg( GL_INFO, "VCP Download Command Received ... param 1: %d, param 2: %d\n",
                rda_command->param1, rda_command->param2 );

   /* Did the VCP download command come from an application? */
   if( rda_command->line_num == HCI_VCP_INITIATED_RDA_CTRL_CMD )
      param3 = VCP_DO_NOT_TRANSLATE;

   /* If the download came from an HCI application, we want to pause PRF
      Selection since manual VCP downloads override PRF Selection/MSF. */
   if( (rda_command->line_num == HCI_VCP_INITIATED_RDA_CTRL_CMD)
                              || 
       (rda_command->line_num == HCI_INITIATED_RDA_CTRL_CMD) ){

      unsigned char flag = 0;
      int ret = 0;

      /* Flag is set if automatic PRF selection is active. */
      if( PBD_auto_prf ){

         if( (ret = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_PRFSF_PAUSED,
                                    ORPGINFO_STATEFL_SET,
                                    &flag )) >= 0 )
            LE_send_msg( GL_INFO, 
                         "PRF Selection Paused Owing to HCI VCP Download Command\n" );

      }

   }

   /* Can only download VCP if RDA Control is not LOCAL. */
   if( PH_get_rda_status( RS_CONTROL_STATUS ) != RDA_CONTROL_LOCAL ){

      Vcp_struct *vcp_info = NULL, *download_vcp = NULL;
      int passed_vcp_num = rda_command->param1;

      /* "param2" indicates whether VCP information passed with the command. If 1, 
         VCP data passed with the command. */
      if( rda_command->param2 == 1 ){

         vcp_info = (Vcp_struct *) rda_command->msg;
         passed_vcp_num = vcp_info->vcp_num;

      }

      /* If the commanded VCP is the same as the currently executing VCP,
         then we want to update "current". */
      if( (rda_command->param1 == PBD_vcp_number)
                          ||
          (passed_vcp_num == PBD_vcp_number) ){

         /* Did the Download Command come with the VCP definition? */
         if( rda_command->param2 == 1 ){

            /* Update the "current" VCP with the passed data. */
            LE_send_msg( GL_INFO, "--->Updating Current VCP Table With Application Data.\n" );

         }
         else{

            /* No VCP data was passed with command.  Update "current" with the Adaptation 
               version. */
            LE_send_msg( GL_INFO, "--->Updating Current VCP Table with Adaptation Data.\n" );

         }

         /* If VCP information not passed with the command, vcp_info will be NULL.  In 
            this case, the vcp buffer will be filled by the Adaptation Data version
            of the VCP as a result of the following function call. */
         Update_current_vcp_table( PBD_vcp_number, vcp_info, &download_vcp  );

         /* Download the "current" VCP. */
         LE_send_msg( GL_INFO, "--->Commanding Download of VCP 0 (%d).\n", passed_vcp_num );

         /* Check if download_vcp is not NULL ... in this case, send the VCP definition with
            the command.  Note: downlaod_vcp is not NULL if VCP modified for SAILS. */
         if( download_vcp != NULL ){

            /* Pass the VCP definition. */
            if( ORPGRDA_send_cmd( COM4_DLOADVCP, PBD_INITIATED_RDA_CTRL_CMD, 0,
                                   1, param3, 0, 0, (char *) download_vcp, sizeof(Vcp_struct) ) < 0 )
               LE_send_msg( GL_ERROR, "Unable to Command VCP Download\n" );

            else{

               /* Set the last commanded VCP. */
               PBD_last_commanded_vcp = PBD_vcp_number;

            }

            free(download_vcp);
            download_vcp = NULL;

         }
         else{
            
            /* Download the "current" VCP. */
            if( ORPGRDA_send_cmd( COM4_DLOADVCP, PBD_INITIATED_RDA_CTRL_CMD, 0,
                                   0, param3, 0, 0, NULL ) < 0 )
               LE_send_msg( GL_ERROR, "Unable to Command VCP Download\n" );

            else{

               /* Set the last commanded VCP. */
               PBD_last_commanded_vcp = PBD_vcp_number;

            }

         }

      }
      else{

         /* Handle the case where the VCP to download is different than the "current".  This
            can be because the VCP number is different or the VCP is 0 (or current). */
         Vcp_struct *data = NULL;

         /* Command a download VCP which is not current. We need to check in this case is whether
            VCP data is passed with the command.  If passed, we want to pass it forward. */
         if( vcp_info != NULL ){

            LE_send_msg( GL_INFO, "--->1: Commanding Download of VCP %d.\n", passed_vcp_num );

            /* Initialize data for RDA command. */
            data = vcp_info;

            /* Check if SAILS should be active this VCP. */
            HS_check_sails( passed_vcp_num, vcp_info, (Vcp_struct **) &download_vcp );

            /* If download_vcp != NULL, send this modified VCP. */
            if( download_vcp != NULL )
               data = download_vcp;

            /* Issue command. */
            if( ORPGRDA_send_cmd( COM4_DLOADVCP, PBD_INITIATED_RDA_CTRL_CMD, passed_vcp_num,
                                  1, param3, 0, 0, (char *) data, sizeof(Vcp_struct)) < 0 )
               LE_send_msg( GL_ERROR, "Unable to Command VCP Download\n" );

            else{

               /* Set the last commanded VCP. */
               PBD_last_commanded_vcp = passed_vcp_num;

                if( PBD_verbose )
                   VM_write_vcp_data( vcp_info );

            }

            /* Free the download_vcp buffer if not NULL. */
            if( download_vcp != NULL )
               free(download_vcp);

         }
         else{

            /* Commanded VCP is not 0 but the VCP definition not passed with command,
               then get data from adaptation data. */
            if( passed_vcp_num != 0 ){

               int pos = ORPGVCP_index( passed_vcp_num );

               /* Check for error and return if VCP not found. */
               if( pos < 0 ){

                  LE_send_msg( GL_ERROR, "ORPGVCP_index(%d) Failed\n", passed_vcp_num );
                  return 0;

               }

               /* Get pointer to the VCP data. */
               data = vcp_info = (Vcp_struct *) ORPGVCP_ptr( pos );
               if( vcp_info != NULL ){

                  /* Check if SAILS should be active this VCP. */
                  HS_check_sails( passed_vcp_num, vcp_info, (Vcp_struct **) &download_vcp );

                  /* If download_vcp != NULL, send this modified VCP. */
                  if( download_vcp != NULL )
                     data = download_vcp;

                  /* Issue command. */
                  LE_send_msg( GL_INFO, "--->2.1: Commanding Download of VCP %d.\n", passed_vcp_num );
                  if( ORPGRDA_send_cmd( COM4_DLOADVCP, PBD_INITIATED_RDA_CTRL_CMD, passed_vcp_num,
                                        1, param3, 0, 0, (char *) data, sizeof(Vcp_struct)) < 0 )
                     LE_send_msg( GL_ERROR, "Unable to Command VCP Download\n" );

                  else{

                     /* Set the last commanded VCP. */
                     PBD_last_commanded_vcp = passed_vcp_num;

                  }

                  /* Free the download_vcp buffer if not NULL. */
                  if( download_vcp != NULL )
                     free(download_vcp);

               }

            }
            else{

               Vol_stat_gsm_t *vol_status = NULL;
               int ret = 0;

	       /* VCP number is 0 and VCP data not passed.  Read the VCP data from Volume Status. */
               if( (ret = ORPGDA_read( ORPGDAT_GSM_DATA, (char *) &vol_status, LB_ALLOC_BUF,
                                       VOL_STAT_GSM_ID ) < 0) ){
               
                  LE_send_msg( GL_INFO, "--->2.2: ORPGDA_read(ORPGDA_GSM_DATA, VOL_STAT_GSM_ID) Failed: %d\n", ret );
                  LE_send_msg( GL_INFO, "------>Commanding Download of VCP %d.\n", rda_command->param1 );
                  if( ORPGRDA_send_cmd( COM4_DLOADVCP, PBD_INITIATED_RDA_CTRL_CMD, rda_command->param1,
                                        0, param3, 0, 0, NULL ) < 0 )
                     LE_send_msg( GL_ERROR, "Unable to Command VCP Download\n" );

                  else{

                     /* Set the last commanded VCP. */
                     PBD_last_commanded_vcp = rda_command->param1;

                  }

               }
               else{

                  /* Data read .... */
                  passed_vcp_num = vol_status->vol_cov_patt;
                  data = vcp_info = &vol_status->current_vcp_table;

                  /* Check if SAILS should be active this VCP. */
                  HS_check_sails( passed_vcp_num, vcp_info, (Vcp_struct **) &download_vcp );

                  /* If download_vcp != NULL, send this modified VCP.  Otherwise the VCP
                     that was read from Volume Status will be sent. */
                  if( download_vcp != NULL )
                     data = download_vcp;

                  /* Issue command. */
                  LE_send_msg( GL_INFO, "--->2.3: Commanding Download of VCP %d.\n", passed_vcp_num );
                  if( ORPGRDA_send_cmd( COM4_DLOADVCP, PBD_INITIATED_RDA_CTRL_CMD, passed_vcp_num,
                                        1, param3, 0, 0, (char *) data, sizeof(Vcp_struct) ) < 0 )
                     LE_send_msg( GL_ERROR, "Unable to Command VCP Download\n" );

                  else{

                     /* Set the last commanded VCP. */
                     PBD_last_commanded_vcp = rda_command->param1;

                     /* Free memory. */
                     if( vol_status != NULL )
                        free(vol_status);

                     if( download_vcp != NULL )
                        free(download_vcp);

                  }

               }

            }

         }

      }

   }

   /* Return to caller. */
   return 0;

/* End of Service_vcp_download(). */
}

/*********************************************************************

   Description:  
      The current vcp table is updated with adaptation data version
      or with version passed in argument vcp if not NULL.
  
   Inputs:  
      vcp_num - Volume Coverage Pattern to validate.
      vcp - volume coverage pattern data (optional)

   Outputs:
      download_vcp - set to non-NULL if the VCP to be download was 
                     modified, e.g., SAILS is enabled and the VCP
                     is allowed to be modifed.
  
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
static int Update_current_vcp_table( int vcp_num, Vcp_struct *vcp, 
                                     Vcp_struct **download_vcp ){

   int vcp_ind;

   /* Initialize the download_vcp to NULL. */
   *download_vcp = NULL;

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

      /* Check if SAILS is allowed and SAILS is active. */
      HS_check_sails( vcp_num, vcp, download_vcp );

      /* Do structure copy. */
      memcpy( (void *) &Vs_gsm.current_vcp_table, vcp, sizeof(Vcp_struct) );
          
      /* Write VCP data to LB. If write fails, return error. */
      Write_gsm( (char *) &Vs_gsm );

      /* Write out the VCP data ... */
      if( PBD_verbose ){

         if( *download_vcp == NULL )
            VM_write_vcp_data( &Vs_gsm.current_vcp_table );

         else
            VM_write_vcp_data( *download_vcp );

      }

   }

   return ( 0 );

/* End of Update_current_vcp_table() */
}


/******************************************************************

   Description:
      Writes GSM data.

   Inputs:
      gsm - pointer to GSM data to write.

   Returns:
      Function returns status of thye write operation.

******************************************************************/
static int Write_gsm( char* gsm ){

   int ret;

   /* Write out the volume-scan based general status message to
      linear buffer, then post event indicating start of volume
      scan. */
   EN_control( EN_SET_SENDER_ID, PBD_pid );
   ret = ORPGDA_write( ORPGDAT_GSM_DATA, (char *) &Vs_gsm,
                       sizeof( Vol_stat_gsm_t ), VOL_STAT_GSM_ID );

   if( ret < 0)
      LE_send_msg( GL_ERROR, "ORPGDAT_GSM_DATA write failed (%d)\n", ret );

   return ret;

/* End of Write_gsm(). */
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


/*\///////////////////////////////////////////////////////////////////////////
 
   Description:
      Checks the signal processing state variable and outputs an RPG Status
      Log message when the state changes.

   Inputs:
      rpg_hd - address of RPG basedata header.

///////////////////////////////////////////////////////////////////////////\*/
static void Check_sig_proc_states( Base_data_header *rpg_hd ){

   /* Allocate a state buffer. */
   int len = 0, prev = 0;
   char *buf = calloc( (MAX_STATUS_LENGTH+1), sizeof(char) );

   if( buf == NULL )
      LE_send_msg( GL_INFO, "Unable to Process RDA Status Log Message\n" );

   else {

      /* Initialization. */
      PBD_RxRN_state = 0;
      PBD_CBT_state = 0;

      /* Place header string in buffer. */
      strcat( buf, "RDA STATUS:" );
      len = strlen( buf );

      /* Check if anything has changed. */
      PBD_RxRN_state = (int) (rpg_hd->sig_proc_states & SPS_RXRN_STATE);
      prev = (int) (PBD_sig_proc_states & SPS_RXRN_STATE);
      if( (PBD_RxRN_state != prev) 
               ||
          (PBD_sig_proc_states == PBD_UNKNOWN_SIG_PROC_STATE) ){

         /* State has changed. */
         if( PBD_RxRN_state )
            Process_sig_proc_state( &buf, &len, "RxRN=", "ON" );

         else
            Process_sig_proc_state( &buf, &len, "RxRN=", "OFF" );

      }

      PBD_CBT_state = rpg_hd->sig_proc_states & SPS_CBT_STATE;
      prev = PBD_sig_proc_states & SPS_CBT_STATE;
      if( (PBD_CBT_state != prev)
               ||
          (PBD_sig_proc_states == PBD_UNKNOWN_SIG_PROC_STATE) ){

         /* State has changed. */
         if( PBD_CBT_state )
            Process_sig_proc_state( &buf, &len, "CBT=", "ON" );

         else
            Process_sig_proc_state( &buf, &len, "CBT=", "OFF" );

      }

      /* Write out last status message. */
      if( (buf != NULL)
               &&
          (strcmp( buf, "RDA STATUS:" ) != 0) )
         LE_send_msg( GL_STATUS, "%s\n", buf );

      if( buf != NULL )
         free(buf);

   }

} /* End of Check_sig_proc_states() */


/*\///////////////////////////////////////////////////////////////////////////
 
   Description:
      Controls writing RDA status data to the system log file.  If status
      to be written to status buffer causes the buffer to overflow, the buffer
      is written to the status log before the new data is copied.

   Inputs:
      buf - address of status buffer.
      len - address of length the status buffer string.
      field_id - string containing the ID of the field to write to status
                 buffer.
      field_val - string containing the field value.

   Outputs:
      buf - status buffer containing new status data.
      len - length of the string with new status data.

   Returns:
      There is no return value define for this function.

///////////////////////////////////////////////////////////////////////////\*/
static void Process_sig_proc_state( char **buf, int *len, char *field_id,
                                    char *field_val ){

   int comma_and_space;

   /* If there is previous status in the buffer, will need to
      append a comma and a space. */
   if( strcmp( *buf, "RDA STATUS:") != 0 )
      comma_and_space = 2;
   else
      comma_and_space = 0;

   /* Is the buffer large enoough to accommodate the new status. */
   if( (*len + strlen(field_id) + strlen(field_val) + comma_and_space) >
       MAX_STATUS_LENGTH ){

      /* Buffer not large enough.  Write the message to the status log. */
      LE_send_msg( GL_STATUS, "%s\n", *buf );

      /* Initialize status buffer. */
      *buf = memset( *buf, 0, (MAX_STATUS_LENGTH+1) );

      /* Place header string in buffer. */
      strcat( *buf, "RDA STATUS:" );
      *len = strlen( *buf );
      comma_and_space = 0;

   }

   /* Append a comma and space, if required. */
   if( comma_and_space )
      strcat( *buf, ", " );

   /* Append status ID and status value to buffer. */
   strcat( *buf, field_id );
   strcat( *buf, field_val );

   /* Determine new length of status buffer. */
   *len = strlen( *buf );

/* End of Process_sig_proc_states() */
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
   float low_ref, high_ref, low_ref_2nd_pass;
   float high_ref_2nd_pass, small_core_ref;

   /* Get AVSET enable/disable switch. */
   if( DEAU_get_values( "alg.avset.enable_avset", &dtemp, 1 ) >= 0 )
      Avset_enabled = (int) dtemp;

   else
      Avset_enabled = 1;

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

   if( DEAU_get_values( "alg.avset.small_core_ref_thresh", &dtemp, 1 ) >= 0 ){

      small_core_ref = (float) dtemp;
      Small_core_refl_thresh = (int) round( 2.0*dtemp + 64.0 );
      Small_core_refl_thresh += 2;

   }
   else{

      small_core_ref = 38.0;
      Small_core_refl_thresh = 142;

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

   /* Small core Reflectivity Area threshold. */
   if( DEAU_get_values( "alg.avset.small_core_ref_area_thresh", &dtemp, 1 ) >= 0 )
      Small_core_refl_area_thresh = (float) dtemp;

   else
      Small_core_refl_area_thresh = 8;

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
   LE_send_msg( GL_INFO, "-->Small Core Reflectivity Thresholds... Z: %f dBZ, Area: %f km^2\n",
                small_core_ref, Small_core_refl_area_thresh );
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
      Area_small_core_refl = 0.0;
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

      if( ref[i] >= Small_core_refl_thresh )
         Area_small_core_refl += lookup[i];

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
       (rpg_hd->status == GENDVOL) ){

      int cut;

      Area_low_refl /= 100.0;
      Area_high_refl /= 100.0;
      Area_small_core_refl /= 100.0;
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
   Area_small_core_refl = 0.0;
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
   int current_state = Current_AVSET_state;

   /* Process dependent on current AVSET state. */
   switch( Current_AVSET_state ){

      case AVSET_INITIAL_STATE:
      default:
      {

         LE_send_msg( GL_INFO, "---> Initial Pass <----\n" );

         /* Check areas against area thresholds. */
         if( (Area_low_refl <= Low_refl_area_thresh)
                             &&
             (Area_high_refl <= High_refl_area_thresh) 
                             &&
             (Area_small_core_refl <= Small_core_refl_area_thresh) ){

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

               /* Since this logic works on all cuts, we do not want to set the 
                  Terminate_cut flag unless it is 0.  If not zero, then we leave
                  in alone since it affects processing in other modules, particulary
                  PH_process_new_cut(). */
               if( !Terminate_cut )
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

                  /* Since this logic works on all cuts, we do not want to set the 
                     Terminate_cut flag unless it is 0.  If not zero, then we leave
                     in alone since it affects processing in other modules, particulary
                     PH_process_new_cut(). */
                  if( !Terminate_cut )
                     Terminate_cut = 1;

                  LE_send_msg( GL_INFO, "AVSET Terminating Cut. \n" );

               }

            }

         }

         break;

      }
      
      case AVSET_2ND_PASS_STATE:
      {
      
         LE_send_msg( GL_INFO, "---> Second Pass <----\n" );

         /* Check areas against area thresholds. */
         if( (Area_low_refl_2nd_pass <= Low_refl_area_thresh_2nd_pass)
                                     &&
             (Area_high_refl_2nd_pass <= High_refl_area_thresh_2nd_pass) 
                                     &&
             (Area_small_core_refl <= Small_core_refl_area_thresh) ){

            /* Since this logic works on all cuts, we do not want to set the 
               Terminate_cut flag unless it is 0.  If not zero, then we leave
               in alone since it affects processing in other modules, particulary
               PH_process_new_cut(). */
            if( !Terminate_cut )
               Terminate_cut = 1;

            LE_send_msg( GL_INFO, "AVSET Terminating Cut ... 2nd Pass. \n" );

         }

         break;

      }

   } /* End of switch(). */

   if( current_state == AVSET_INITIAL_STATE ){

      /* Write Information to task log. */
      LE_send_msg( GL_INFO, "Initial Pass ---->\n" );
      LE_send_msg( GL_INFO, "-->Elevation Angle: %4.1f deg, RDA Cut #: %d\n",
                   rpg_hd->elevation, rpg_hd->elev_num );
      LE_send_msg( GL_INFO, 
             "-->Low Refl Area: %9.2f km^2, High Refl Area: %9.2f km^2, Small Core Area: %9.2f km^2\n",
             Area_low_refl, Area_high_refl, Area_small_core_refl );

   }
   else {

      /* Write information to task log. */
      LE_send_msg( GL_INFO, "Second Pass ---->\n" );
      LE_send_msg( GL_INFO, "-->Elevation Angle: %4.1f deg, RDA Cut #: %d\n",
                   rpg_hd->elevation, rpg_hd->elev_num );
      LE_send_msg( GL_INFO, "-->2nd Pass ... Low Refl Area: %9.2f km^2, High Refl Area: %9.2f km^2\n",
                   Area_low_refl_2nd_pass, Area_high_refl_2nd_pass );
      LE_send_msg( GL_INFO, 
                            "            ... Small Core Area: %9.2f km^2\n", Area_small_core_refl );

   }
      
/* End of AVSET_test_thresholds(). */
}
#endif
