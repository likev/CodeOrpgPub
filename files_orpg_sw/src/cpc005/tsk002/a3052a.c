/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/10/26 21:46:09 $
 * $Id: a3052a.c,v 1.28 2012/10/26 21:46:09 steves Exp $
 * $Revision: 1.28 $
 * $State: Exp $
 */
#define A3052A_C
#include <pcipdalg.h>
#include <prfselect_buf.h>

#define PROCESS                1
#define STOP                   3
#define ABORT                  4

#define SECS_IN_DAY            86400

/* Global Variables */
extern pid_t Pid;

/* Static Global Variables. */
static Vcpinfo_t Vcp_info;
static Prfselect_t Vcp_data;

/* Static function prototypes. */
static int A3052B_get_buffers( Prcipmsg_t **psmptr );

/*\////////////////////////////////////////////////////////////////////

   Description:
      Buffer control routine for the Mode Selection Function.

   Input:
      event - Event ID (should always be INPUT_AVAILABLE)

////////////////////////////////////////////////////////////////////\*/
void A3052A_buffer_control( int event ){

   int wb_status, rda_control, catflag, status, istat, *inbuf = NULL;
   int psmbuf, type, volnum, update_status;
   unsigned char prfselect_on = 0, prfselect_paused = 0;
   float area;
   Prcipmsg_t *outbuf = NULL;

   static int wxstatus_deselect = -1;
   static int hybrscan_vol_num = 0, prfsel_vol_num = 0;

   /* Make sure this function is called only for INPUT_AVAILABLE event. */
   if( event != INPUT_AVAILABLE )
      return;

   LE_send_msg( GL_INFO, "Processing INPUT_AVAILABLE Event .... \n" );

   /* Acquire input buffer. */
   inbuf = (int *) RPGC_get_inbuf_any( &type, &istat );
   
   /* Was acquisition NORMAL? */
   if( (istat == NORMAL) && (inbuf != NULL) ){

      /* Process Hybrid scan ..... */
      if( type == Hybrscan_id ){

         /* Variable initializations. */
         status = 0;
         psmbuf = 0;
         area = 0.0;
         catflag = CTGRY0;
 
         /* Update the adaptation data for MSF. */
         RPGC_read_ade( &Msf_ade_id, &update_status );

         /* Extract the input buffer's volume scan number.   We use 
            this for VCP download processing. */
         hybrscan_vol_num = RPGC_get_buffer_vol_num( inbuf );

         /* Get output buffer for precipitation status message. */
         psmbuf = A3052B_get_buffers( &outbuf );
 
         /* If psmbuf equals 0, buffers could not be obtained in a3052b_....
          and RPGC_abort was called to abort processing. */
         if( psmbuf ){
 
            Scan_Summary *scansum;

            /* Get Volume Number, VCP, Wx Status (Wx Mode) and Current Date & Time.
               This information will be used to populate ITCs and Status messages. */
            volnum = RPGC_get_buffer_vol_num( inbuf );
            scansum = RPGC_get_scan_summary( volnum );

            if( scansum == NULL ){
               
               RPGC_log_msg( GL_ERROR, "RPGC_get_scan_summary( %d ) Failed\n", volnum );
               RPGC_rel_inbuf( inbuf );
               status = ABORT;

            }
            else{

               Mode_select_status.a3052t.curr_time = (scansum->volume_start_date - 1)*SEC_IN_DAY +
                                                      scansum->volume_start_time;

               memset( &Vcp_info, 0, sizeof(Vcpinfo_t) );
               Vcp_info.rpgvcp = scansum->vcp_number;
               Vcp_info.rpgwmode = scansum->weather_mode; 
               Vcp_info.rpgvsnum = volnum;         
               LE_send_msg( GL_INFO, "Scan Summary:  \n" );
               LE_send_msg( GL_INFO, "---->rpgvcp: %d, rpgwmode: %d, rpgvsnum: %d\n",
                            Vcp_info.rpgvcp, Vcp_info.rpgwmode, Vcp_info.rpgvsnum );
             

               /* Make local copies of adaptation data; determine area of each sample
                  bin of radial & set the Program Status Flag to "Process". */
               A3052E_init_get_adapt( );
               status = PROCESS;

            }
 
         }
         else{
 
            /* Output buffer acquisition failed .... set ABORT flag. */
            status = ABORT;

            /* Release input buffer since we are not going to need it. */
            RPGC_rel_inbuf( inbuf );

         }
 
         /* Begin of processing of Hybrid Scan Reflectivity. */
         if( status == PROCESS ){
 
            /* Get the Precip Table data. */
            A3052F_comp_tables( );
 
            /* Sum areas of bins above Rate Threshold. */
            A3052G_chekrate_sumareas( inbuf, &area );
 
            /* No longer need the input buffer so release it. */
            RPGC_rel_inbuf( inbuf );
 
            /* Update the precipitation categories. */
            A3052H_precip_cats( &catflag );
 
         } 
 
         /* Make final determination of Precip Category, change Weather Status
            to Precipitation if Category has just changed from #0 to #1, 
            & fill and release the output buffer. */
         if( psmbuf ){
 
            wxstatus_deselect = 0;
            A3052I_setmode_setobuf( outbuf, catflag, &Vcp_info, &wxstatus_deselect );

            /* Write out the Precipitation Status Message to Gage Database. */
            Write_gagedb( (int *) outbuf );
 
            /* Write the Wx Status Message. */
            Write_wx_status( area, wxstatus_deselect );

            /* Release the Precipitation Status Message. */
            RPGC_rel_outbuf( outbuf, FORWARD );
            psmbuf = 0;

         }

         /* ABORT condition processing; DESTROY buffers not yet released. */
         if( status == ABORT ){
 
            if( psmbuf )
               RPGC_rel_outbuf( outbuf, DESTROY );

            RPGC_abort();
            return;

         }

      } /* End of processing for Hybrscan_id. */

      /* Process Prfsel_id ..... */
      else if( type == Prfsel_id ){

         Prfselect_t *prfsel_buf = (Prfselect_t *) inbuf;

         /* Save the disposition.  This will be used later to determine whether
            or not a VCP download is commanded. */
         Vcp_data.disposition = prfsel_buf->disposition;

         /* Extract the input buffer's volume scan number.   We use 
            this for VCP download processing. */
         prfsel_vol_num = RPGC_get_buffer_vol_num( inbuf );

         /* Save size and change_to_vcp elements. */
         Vcp_data.size = prfsel_buf->size;
         Vcp_data.change_to_vcp = prfsel_buf->change_to_vcp;

         /* Make local copy of the vcp table from the PRF selection algorithm. */
         memcpy( (void*) &Vcp_data.vcp, (void *) &prfsel_buf->vcp, prfsel_buf->size );

         RPGC_rel_inbuf( inbuf );

      } /* End of processing for Prfsel_id. */

   }
   else{
 
      /* Input buffer not successfully acquired, abort processing for
         volume scan. */
      RPGC_abort();
      return;

   }
 
   /* Check if we need to download the VCP from PRF Selection.  We need to 
      do this if: 

      1) mode selection is not changing the weather mode, and
      2) this function has obtained both Prfsel_id and Hybrscan_id input 
         buffers for the current volume scan, and
      3) The PRF selection disposition is FORWARD, and
      4) The PRF selection flag is either ON or PAUSED. 

   */
   prfselect_on = ORPGINFO_is_prf_select();
   prfselect_paused = ORPGINFO_is_prf_select_paused();

   if( (wxstatus_deselect == 0) && (prfsel_vol_num == hybrscan_vol_num) 
                                &&
                   (Vcp_data.disposition == FORWARD) ){

      /* If the wideband is connected and the RDA is in REMOTE Control,
         download VCP.  Also command download if RPG is running in a
         non-operational (test) environment. */
      wb_status = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );
      rda_control = ORPGRDA_get_status( RS_CONTROL_STATUS );

      if( (((wb_status == RS_CONNECTED) && (rda_control != CS_LOCAL_ONLY))
                                   ||
                      (!ORPGMISC_is_operational())) ){

         /* Has Auto PRF been turned off or paused? */
         if( (prfselect_on) && (!prfselect_paused) ){

            /* If current VCP is the same as what we wish to change to, command
               Download VCP. */
            if( Vcp_info.rpgvcp == Vcp_data.change_to_vcp ){

               /* Copy the VCP data from prfselect to the Vcp_info.current_vcp_table. */
               memcpy( (void*) &Vcp_info.current_vcp_table, (void *) &Vcp_data.vcp, 
                       Vcp_data.size );

               ORPGRDA_send_cmd( COM4_DLOADVCP, MSF_INITIATED_RDA_CTRL_CMD,
                                 0,   /* Indicates current VCP. */
                                 1,   /* Indicates VCP data follows in command. */
                                 0, 0, 0, (void *) &Vcp_data.vcp, Vcp_data.size );

               RPGC_log_msg( GL_INFO, "RDA Command (Download VCP):\n" );
               RPGC_log_msg( GL_INFO, "--->VCP: %4d, Weather Status: %2d, Volume Scan #: %3d\n",
                             Vcp_info.rpgvcp, Vcp_info.rpgwmode, Vcp_info.rpgvsnum );
        
            }
            else{

               /* Current VCP is different from what we wish to change to.
                  Command Download VCP. */
               if( Vcp_data.size > 0 )
                  ORPGRDA_send_cmd( COM4_DLOADVCP, MSF_INITIATED_RDA_CTRL_CMD,
                                    (int) Vcp_data.change_to_vcp,
                                    1, /* Indicates VCP data follows in command. */
                                    0, 0, 0, (void *) &Vcp_data.vcp, Vcp_data.size );
               else
                  ORPGRDA_send_cmd( COM4_DLOADVCP, MSF_INITIATED_RDA_CTRL_CMD,
                                    (int) Vcp_data.change_to_vcp,
                                    0,
                                    0, 0, 0, (void *) NULL, 0 );

               RPGC_log_msg( GL_INFO, "RDA Command (Download Adaptation VCP %4d):\n",
                             Vcp_data.change_to_vcp );
               if( Vcp_data.size > 0 )
                  RPGC_log_msg( GL_INFO, "--->VCP Data Imbedded in Command.\n" );

            }

         }
         else{

            RPGC_log_msg( GL_INFO, "VCP Not Downloaded to RDA Because:\n" );
            if( !prfselect_on )
               RPGC_log_msg( GL_INFO, "--->Auto PRF is OFF\n" );

            else
               RPGC_log_msg( GL_INFO, "--->Auto PRF is PAUSED\n" );

         }

      }
      else{

         RPGC_log_msg( GL_INFO, "VCP Not Downloaded to RDA Because:\n" );
         if( wb_status != RS_CONNECTED )
            RPGC_log_msg( GL_INFO, "--->Wideband is Not Connected\n" );

         else
            RPGC_log_msg( GL_INFO, "--->RDA is in LOCAL Control\n" );

      }

      wxstatus_deselect = -1;
      hybrscan_vol_num = prfsel_vol_num = 0;

   }
   else if ( Vcp_data.disposition == DESTROY ){

      RPGC_log_msg( GL_INFO, "VCP Not Downloaded to RDA Because:\n" );
      RPGC_log_msg( GL_INFO, "--->VCP Disposition: DESTROY\n" );

   }

   return;

} /* End of A3052A_buffer_control(). */

/*\////////////////////////////////////////////////////////////////////

   Description:
      Routine gets the output buffer for this function.  If cannot 
      successfully acquire this buffer, the task will abort.

   Inputs:
      psmptr - pointer to receive the PSM (output buffer).

   Returns:
      1 is PSM buffer successfully acquired, or 0 otherwise.

////////////////////////////////////////////////////////////////////\*/
static int A3052B_get_buffers( Prcipmsg_t **psmptr ){

   int psmbuf, ostat;

   psmbuf = 1;
 
   /* Get output buffer for Precipitation Status Message. */
   *psmptr = (Prcipmsg_t  *) RPGC_get_outbuf_by_name( "PRCIPMSG", 
                                          sizeof(Prcipmsg_t), &ostat );
 
   /* Check for ABORT condition. */
   if( ostat == NO_MEM ){
 
      RPGC_abort_because( PROD_MEM_SHED );
      psmbuf = 0;

   }
   else if( ostat != NORMAL ){
 
      RPGC_abort();
      psmbuf = 0;

   }
   else{
 
      /* Initialize the buffer to 0. */
      memset( (void *) *psmptr, 0, sizeof( Prcipmsg_t ) );

   }

   return (psmbuf);

} /* End of A3052B_get_buffers(). */

/*\////////////////////////////////////////////////////////////////////

   Description:
      Routine services the ORPGEVT_START_OF_VOLUME_DATA event.         

   Input:
      event - Event ID (should always be ORPGEVT_START_OF_VOLUME_DATA)

////////////////////////////////////////////////////////////////////\*/
void A3052C_start_of_volume( int event ){

   int ret, cmode, cvcp, cdate, cvtime;

   /* Make sure this function is called only for ORPGEVT_START_OF_VOLUME_DATA 
      event. */
   if( event != ORPGEVT_START_OF_VOLUME_DATA )
      return;

   LE_send_msg( GL_INFO, "Processing ORPGEVT_START_OF_VOLUME_DATA Event .... \n" );

   cmode = ORPGVST_get_mode();
   cvcp = ORPGVST_get_vcp();

   /* If the current weather mode does not equal the current weather status,
      change the current weather status. */
   if( cmode != Mode_select_status.current_wxstatus ){

      Wx_status_t *wxstatus;
      char *buf;

      LE_send_msg( GL_INFO, "Current Wx Mode: %d != Previous Volume Wx Mode: %d\n",
                   cmode, Mode_select_status.current_wxstatus );

      /* Read the weather status. */
      if( (ret = ORPGDA_read( ORPGDAT_GSM_DATA, &buf, LB_ALLOC_BUF,
                              WX_STATUS_ID )) < 0 ){

         LE_send_msg( GL_ERROR,
                      "ORPGDA_read( ORPGDAT_GSM_DATA, WX_STATUS_ID ) Failed (%d)\n",
                      ret );
         return;

      }

      wxstatus = (Wx_status_t *) buf;

      /* Set the current_wxstatus to the current weather mode, the wxstatus
         time to the volume scan start time, and the current VCP to the 
         VCP currently in operation. */
      wxstatus->current_wxstatus = cmode;

      cdate = ORPGVST_get_volume_date() - 1;
      cvtime = ORPGVST_get_volume_time() / 1000;
      wxstatus->current_wxstatus_time = cdate*SECS_IN_DAY + cvtime;

      wxstatus->current_vcp = cvcp;

      /* If current weather mode is the recommended mode, clear the wstatus_deselect
         flag. */
      if( wxstatus->recommended_wxstatus == cmode )
         wxstatus->wxstatus_deselect = 0;

      /* Write the weather status.  Make sure the sender ID is set so that we don't
         re-read the weather status at start of volume. */
      EN_control( EN_SET_SENDER_ID, Pid );
      if( (ret = ORPGDA_write( ORPGDAT_GSM_DATA, buf, sizeof( Wx_status_t ),
                              WX_STATUS_ID )) < 0 )
         LE_send_msg( GL_ERROR,
                      "ORPGDA_write( ORPGDAT_GSM_DATA, WX_STATUS_ID ) Failed (%d)\n",
                      ret );
   }

   return;

} /* End of A3053C_start_of_volume() */

