/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/13 19:51:08 $
 * $Id: prfselect.c,v 1.32 2014/03/13 19:51:08 steves Exp $
 * $Revision: 1.32 $
 * $State: Exp $
 */
#define PRFSELECT_C
#include <prfselect.h>

/* Function prototypes. */
static int Prfselect_update_vol_status();
void Handle_notification( int data_id, LB_id_t msg_id );
static int Read_commands();


/*\/////////////////////////////////////////////////////////////////////

   Description:
     This file defines the "main" PRF selection routine.

     This PRF selection routine supports the legacy PRF selection,
     storm-based PRF selection and cell-based PRF selection.

////////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){

   /* Initialize log error services */
   RPGC_init_log_services( argc, argv );

   /* Register Inputs and Outputs. */
   RPGC_reg_io( argc, argv );

   /* Register for ITC A3CD09 input. */
   RPGC_data_access_open( A3CD09_DATAID, LB_WRITE );
   RPGC_data_access_UN_register( A3CD09_DATAID, LBID_A3CD09, 
                                 Handle_notification );

   /* Initialize data/service that supports reading A3CD09. */
   Read_itc = 0;
   Read_prf_command = 1;
   MISC_systime( NULL );

   /* Register summary array. */
   RPGC_reg_scan_summary();

   /* Register volume status. */
   RPGC_reg_volume_status( &Vol_stat );

   /* Register for site adaptation data. */
   RPGC_reg_site_info( &PS_site_info );

   /* Initialize this task. */
   RPGC_task_init( VOLUME_BASED, argc, argv );

   /* Register for updates to PRF Selection Command Buffer. */
   RPGC_UN_register( ORPGDAT_PRF_COMMAND_INFO, ORPGDAT_PRF_COMMAND_MSGID, 
                     Handle_notification );

   /* Perform some initialization. */
   CF_init();
   
   /* Read the A3cd09 ITC. */
   memset( &A3cd09, 0, sizeof(a3cd09) );
   RPGC_data_access_read( A3CD09_DATAID, &A3cd09, sizeof(a3cd09), A3CD09_MSGID );

   /* Check if ORPG is operational .... (i.e., not development environment). */
   Operational = ORPGMISC_is_operational();

   if( Operational )
      RPGC_log_msg( GL_INFO, "RPG is Operational\n" );

   else
      RPGC_log_msg( GL_INFO, "RPG is Non-Operational\n" );

   /* Done with initialization. */

   /* Waiting for activation. */
   while(1){

      /* This won't return until driving input at the driving
         time is available. */
      RPGC_wait_act( WAIT_DRIVING_INPUT );

      /* Update local Volume Status items. */
      if( Prfselect_update_vol_status() < 0 ){

         RPGC_log_msg( GL_INFO, "RPGC_get_volume_status() Failed\n" );
         RPGC_abort();

         /* Go back to sleep. */
         continue;

      }

      /* Check if the PRF Command buffer was updated. */
      if( Read_prf_command ){

         Read_prf_command = 0;
         Read_commands();

      }

      RPGC_log_msg( GL_INFO, "Calling A3053A_buffer_control()\n" );
      RPGC_log_msg( GL_INFO, "--->Storm_based_PRF_selection:         %d\n",
                    Storm_based_PRF_selection );
      RPGC_log_msg( GL_INFO, "--->Local_storm_based_PRF_selection:   %d\n",
                    Local_storm_based_PRF_selection );
      RPGC_log_msg( GL_INFO, "--->Cell_based_PRF_selection:          %d\n",
                    Cell_based_PRF_selection );

      /* Process input(s) and produce output(s). */
      A3053A_buffer_control();

   } /* End of while(1) */

   return 0;

/* End of main(). */
}

/*\/////////////////////////////////////////////////////////////////////

   Description:
     Updates Volume Status items.

   Returns:
      Function returns -1 on error, or 0 otherwise.

////////////////////////////////////////////////////////////////////\*/
static int Prfselect_update_vol_status(){

   int hour, minute, second, mills;
   int year, month, day;
   int rda_defined = 0;

   PS_rpgvcpid = Vol_stat.rpgvcpid - 1;
   PS_rpgwmode = Vol_stat.mode_operation;

   PS_curr_vcp_tab = Vol_stat.current_vcp_table;

   /* The Volume Status VCP definition comes from the RDA. In general
      this is not an issue.  If, however, this VCP is site-specific
      at the RPG but not at the RDA, then get the VCP definition from
      the RPG.  Note:  This is "special case" processing ... */
   rda_defined = ORPGVCP_get_where_defined( Vol_stat.vol_cov_patt );

   /* This VCP is defined at the RDA .... */
   if( (rda_defined & ORPGVCP_RDA_DEFINED_VCP) ){

      int is_rpg_ss = ORPGVCP_is_vcp_site_specific( Vol_stat.vol_cov_patt,
                                                    ORPGVCP_RPG_DEFINED_VCP );
      int is_rda_ss = ORPGVCP_is_vcp_site_specific( Vol_stat.vol_cov_patt,
                                                    ORPGVCP_RDA_DEFINED_VCP );

      /* Is this a RPG Site-Specific VCP and not defined at the RDA? */
      if( (is_rpg_ss == ORPGVCP_RPG_DEFINED_VCP) 
                         && 
          (is_rda_ss != ORPGVCP_RDA_DEFINED_VCP) ){

         VCP_ICD_msg_t *vcp = (VCP_ICD_msg_t *) ORPGVCP_ptr( PS_rpgvcpid );
         if( vcp != NULL ){

            int size = vcp->vcp_msg_hdr.msg_size * sizeof(short);

            /* Copy Site-specific VCP. */
            RPGC_log_msg( GL_INFO, "Updating Local VCP Definition Because: \n");
            RPGC_log_msg( GL_INFO, "--->RPG definition is Site-Specific (size: %d bytes)\n",
                          size );
            memcpy( &PS_curr_vcp_tab, vcp, size );

         }

      }

   }

   RPGC_log_msg( GL_INFO, "The following Items Define the Volume Status\n" );
   RPGC_log_msg( GL_INFO, "--->Volume Seq #: %ld,  Volume Scan #: %d\n",
                 Vol_stat.volume_number, Vol_stat.volume_scan );

   /* For human-readable output. */
   RPGCS_convert_radial_time( Vol_stat.cv_time, &hour, &minute, &second, &mills );
   RPGCS_julian_to_date( Vol_stat.cv_julian_date, &year, &month, &day );

   /* Get the 2 digit year. */
   if( year >= 2000 )
      year -= 2000;

   if( year >= 1900 )
      year -= 1900;

   RPGC_log_msg( GL_INFO, "--->Volume Time: %02d:%02d:%02d,  Volume Date: %02d/%02d/%02d\n",
                 hour, minute, second, month, day, year );

   RPGC_log_msg( GL_INFO, "--->VCP: %d,  VCP ID: %d, Mode: %d\n",
                 Vol_stat.vol_cov_patt, Vol_stat.rpgvcpid, 
                 Vol_stat.mode_operation );
   RPGC_log_msg( GL_INFO, "--->Prev Volume Status: %d,  Expected Vol Duration %d\n",
                 Vol_stat.pv_status, Vol_stat.expected_vol_dur );

   return 0;

/* End of Prfselect_update_vol_status(). */
}

/*\////////////////////////////////////////////////////////////////////////

   Description:
      Notification handler called when data_id and msg_id have been 
      updated.  When called, Read_itc flag is set.

   Inputs:
      data_id - Data ID of LB 
      msg_id - Message ID within LB.

   
///////////////////////////////////////////////////////////////////////\*/
void Handle_notification( int data_id, LB_id_t msg_id ){

   /* Verify the data ID .... */
   if( (data_id == A3CD09_DATAID) && (msg_id == LBID_A3CD09) ){

      RPGC_log_msg( GL_INFO, "!!! LBID_A3CD09 Updated !!!\n" );
      Read_itc = 1;

   }
   else if( data_id == ORPGDAT_PRF_COMMAND_INFO ){

      RPGC_log_msg( GL_INFO, "!!! PRF Command Updated !!!\n" );
      Read_prf_command = 1;

   }

} /* End of Handle_notification(). */

/*\////////////////////////////////////////////////////////////////////////

   Description:
      Reads the PRF Command LB and processes accordingly.

///////////////////////////////////////////////////////////////////////\*/
static int Read_commands(){

   int ret;
   unsigned char flag = 0;

   /* Initialize the number of cells.  We do this because we assume if we 
      get a notification to read the command buffer, even if Cell-based
      PRF selection is selected, we need to find a new cell anyway. */
   Num_cells = 0;

   /* Initialize the Storm/Cell based PRF Selection flags.  Presumably the 
      appropriate flag will be set if need be before leaving this function. */
   Cell_based_PRF_selection = 0;
   Storm_based_PRF_selection = 0;
   Local_storm_based_PRF_selection = 0;

   /* Check if Auto PRF Selection. */  
   flag = (unsigned char) ORPGINFO_is_prf_select();

   /* Read the command buffer. */
   ret = RPGC_data_access_read( ORPGDAT_PRF_COMMAND_INFO, (char *) &Prf_command,
                                sizeof(Prf_command_t), ORPGDAT_PRF_COMMAND_MSGID );

   /* On bad return, initialize PRF Selection to the Legacy Algorithm. */
   if( ret <= 0 ){

      RPGC_log_msg( GL_INFO, "RPGC_data_access_read(ORPGDAT_PRF_COMMAND_INFO) Failed\n" );
      RPGC_log_msg( GL_INFO, "--->ORPGDAT_PRF_COMMAND_MSGID Error:  %d\n", ret );

      Prf_command.command = PRF_COMMAND_STORM_BASED;
      if( !flag )
         Prf_command.command = PRF_COMMAND_AUTO_PRF;

      memset( &Prf_command.storm_id[0], 0, MAX_CHARS );

      RPGC_log_msg( GL_INFO, 
          "RPGC_data_access_read( ORPGDAT_PRF_COMMAND_INFO ) Failed: %d\n", ret );

      /* Set error code for status. */
      Prf_status.state = Prf_command.command;
      Prf_status.error_code = PRF_STATUS_COMMAND_FAILED;

      if( flag )
         Storm_based_PRF_selection = 1;

      /* Write information to task log file. */
      CF_write_informational_messages( "Read Command Buffer: FAILED" );

      /* Return error. */
      return -1;

   }
   
   /* Do some validation based on command. */

   /* If cell-based PRF selection, it is assumed any previous cell tracked
      will require a new cell to be tracked. */
   if( Prf_command.command == PRF_COMMAND_CELL_BASED ){

      RPGC_log_msg( GL_INFO, "Cell-based PRF Selection Selected ....\n" );

      /* Check if PRF Selection is active.  If not, make it active. */
      if( !flag ){

         if( (ret = ORPGINFO_set_prf_select()) < 0 )
            RPGC_log_msg( GL_INFO, "ORPGINFO_set_prf_select() Failed\n" );

      }

      /* Validate the storm ID. */
      if( strlen( &Prf_command.storm_id[0] ) <= 0 ){

         RPGC_log_msg( GL_STATUS,
                       "Missing Storm ID For Cell-Based PRF Selection\n" );

         Prf_command.command = PRF_COMMAND_STORM_BASED;
         memset( &Prf_command.storm_id[0], 0, MAX_CHARS );

         /* Set error code for status. */
         Prf_status.state = Prf_command.command;
         Prf_status.error_code = PRF_STATUS_COMMAND_INVALID;

         /* Set the flag for Storm-based PRF Selection. */
         Storm_based_PRF_selection = 1;

         /* Write information to task log file. */
         CF_write_informational_messages( "Read Command Buffer: Missing Storm ID" );

         /* Return error. */
         return -1;

      }
      else{

         /* Set the flag for Cell-based PRF Selection. */
         Cell_based_PRF_selection = 1;

      }

   }
   else{

      /* Set the flag for Storm-based PRF selection. */
      if( Prf_command.command == PRF_COMMAND_STORM_BASED ){

         RPGC_log_msg( GL_INFO, "Storm-based PRF Selection Selected ....\n" );

         /* Check if PRF Selection is active.  If not, make it active. */
         if( !flag ){

            if( (ret = ORPGINFO_set_prf_select()) < 0 )
               RPGC_log_msg( GL_INFO, "ORPGINFO_set_prf_select() Failed\n" );

         }

         Storm_based_PRF_selection = 1;
         Local_storm_based_PRF_selection = Storm_based_PRF_selection;

      }
      else if( Prf_command.command == PRF_COMMAND_AUTO_PRF ){


         RPGC_log_msg( GL_INFO, "Auto PRF (Legacy) Selected ....\n" );

         /* Check if PRF Selection is active.  If not, make it active. */
         if( !flag ){

            if( (ret = ORPGINFO_set_prf_select()) < 0 )
               RPGC_log_msg( GL_INFO, "ORPGINFO_set_prf_select() Failed\n" );

         }

      }
      else if( Prf_command.command == PRF_COMMAND_MANUAL_PRF ){

         RPGC_log_msg( GL_INFO, "Manual PRF Selected ....\n" );

         /* If Auto PRF is active, disable it. */
         if( flag ){

            if( (ret = ORPGINFO_clear_prf_select()) < 0 )
               RPGC_log_msg( GL_INFO, "ORPGINFO_clear_prf_select() Failed\n" );

         }

      }

      /* Set error code for status. */
      Prf_status.state = Prf_command.command;
      Prf_status.error_code = PRF_STATUS_NO_ERRORS;

   }

   /* Write information to task log file. */
   CF_write_informational_messages( "Read Command Buffer: SUCCESS" );

   /* Return normal. */
   return 0;

/* End of Read_commands(). */
}

