/*************************************************************************

    Maintenance Task: Product Distribution Tasks and Datastores

    Routines for managing the General Status Message (GSM) - related items.

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/09 22:03:49 $
 * $Id: mnttsk_pd_gsm.c,v 1.53 2014/07/09 22:03:49 steves Exp $
 * $Revision: 1.53 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>            /* strncpy()                               */
#include <fcntl.h>
#include <sys/types.h>

#define	STARTUP		1
#define	RESTART		2
#define	CLEAR		3

#include <gen_stat_msg.h>
#include <a309.h>
#include <rpg_port.h>
#include <prod_user_msg.h>
#include <orpg.h>
#include <orpgsite.h>
#include <orpgadpt.h>
#include <hci.h>
#include <precip_status.h>
#include <orpgsails.h>


/*
 * Static Global Variables
 */
static RDA_status_msg_t default_rda_status =  { { 0, 0, 0, 0, 0, 0, 0, 0 },
                                                0,OS_INDETERMINATE, 0, 0, 0, 0, BD_ENABLED_NONE, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0,
                                                0xffff, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, { 0, 0, 0, 0, 0, 0,   
                                                0, 0, 0, 0, 0, 0, 0, 0 } };

static int Comms_default = 0;

static RDA_RPG_comms_status_t default_comms_status[2] = { { RS_DISCONNECTED_CM,
                                                            RS_OPERABILITY_STATUS },
                                                          { RS_CONNECTED,
                                                            RS_OPERABILITY_STATUS } };

static Previous_state_t Default_previous_state = { RS_STANDBY,
                                                   BD_REFLECTIVITY | BD_VELOCITY | BD_WIDTH ,
                                                   CA_NO_ACTION,
                                                   ISU_ENABLED,
                                                   OP_OPERATIONAL_MODE,
                                                   SB_NOT_INSTALLED,
                                                   -21,
                                                   RDA_IS_NON_CONTROLLING,
                                                   {0*sizeof(Vcp_struct)} };


static Vol_stat_gsm_t Vol_stat;

static int Def_vcp = 21;

static int Def_wxmode = ORPGSITE_PRECIP_MODE;



/*
 * Static Function Prototypes
 */
static void Write_rda_status(void) ;
static void Write_previous_rda_state(void) ;
static  int Update_vol_stat_gsm(void);
static  int Write_vol_stat_gsm(void) ;
static void Write_scan_summary(void) ;
static void Write_wx_status(void) ;
static void Write_sails_status() ;
static  int Round ( float r );

/**************************************************************************

   Description:
      Driver module for the initialization of GSM related items.
      The initialization actions depend on the "startup_action" input
      argument.

   Inputs:
      startup_action - either RESTART, STARTUP, or CLEAR

   Outputs:

   Returns:
      If the startup_action is CLEAR, exits with non-zero exit code
      if ORPGDA_clear on the ORPGDAT_GSM_DATA fails, otherwise exits 
      with 0 exit code.  For all other startup_actions, returns 0.

   Notes:

**************************************************************************/
int MNTTSK_PD_GSM_maint(int startup_action){

   int retval ;

   /* Open the GSM LB with write permission. */
   ORPGDA_write_permission( ORPGDAT_GSM_DATA );

   /* Perform action. */
   if( startup_action == CLEAR ){
       
      LE_send_msg( GL_INFO, "\t1. Remove All GSM Messages.") ;
      retval = ORPGDA_clear( ORPGDAT_GSM_DATA, LB_ALL );
      if( retval < 0 )
         exit(1);

      Precip_status_t ps;

      ps.current_precip_status = PRECIP_STATUS_UNKNOWN;
      ps.rain_area_trend = TREND_UNKNOWN;
      ps.time_last_exceeded_raina = TIME_LAST_EXC_RAINA_UNKNOWN;
      ps.time_remaining_to_reset_accum = RESET_ACCUM_UNKNOWN;
      ps.rain_area = PRECIP_AREA_UNKNOWN;
      ps.rain_area_diff = PRECIP_AREA_DIFF_UNKNOWN;
      ps.rain_dbz_thresh_rainz = RAIN_DBZ_THRESH_UNKNOWN;
      ps.rain_area_thresh_raina = RAIN_AREA_THRESH_UNKNOWN;
      ps.rain_time_thresh_raint = RAIN_TIME_THRESH_UNKNOWN;

      /* Because this LB contains information required to initial the
         ORPGDAT_HCI_DATA, HCI_PRECIP_STATUS_MSG_ID, write initialized
         message to this LB.  Normally we would let hci_agent do this
         but hci_agent is killed at shutdown. */
      LE_send_msg( GL_INFO, "\t2. Initialize Precip Status.") ;
      retval = ORPGDA_write( ORPGDAT_HCI_DATA, (char *) &ps,  
                             sizeof(Precip_status_t), HCI_PRECIP_STATUS_MSG_ID );
      if( retval < 0 )
         exit(1);

      exit(0);

   }

   LE_send_msg( GL_INFO,
                "\t1. Initialization of the GSM Volume Status Message.") ;

   if ( (startup_action == STARTUP) || (startup_action == RESTART) ){

      retval = Update_vol_stat_gsm() ;
      if (retval != 0) {

         LE_send_msg( GL_INFO, "Update of Volume Status Failed.  Initializing Message.",
                      ORPGDAT_GSM_DATA) ;
         if( Write_vol_stat_gsm() < 0 )
            exit(1);

      }

   }

   if ((startup_action == RESTART) || (startup_action == STARTUP)) {

      LE_send_msg( GL_INFO,
                   "\t2. Initialize the GSM RDA Status Message.") ;
      Write_rda_status() ;

      LE_send_msg( GL_INFO, 
                   "\t3. Initialize the GSM RDA Previous State Message.") ;
      Write_previous_rda_state() ;

      LE_send_msg( GL_INFO, 
                   "\t4. Initialize the GSM Weather Status Message.") ;
      Write_wx_status() ;

      LE_send_msg( GL_INFO, 
                   "\t5. Initialize the GSM SAILS Status Message.") ;
      Write_sails_status() ;

   }
     
   if( startup_action == STARTUP ){

      LE_send_msg( GL_INFO,
                   "\t5. Initialize the GSM Scan Summary Message.") ;
      Write_scan_summary() ;

   }

   return(0) ;

/*END of MNTTSK_PD_GSM_maint()*/
}


/*\////////////////////////////////////////////////////////////////////
//
//  Description:
//     Writes the RDA status message of the General Status Message 
//     Linear Buffer.  The RDA status is initialized with default
//     values defined at the top of this file.
//
//  Inputs:
//
//  Outputs:
//
//  Returns:
//     There are no return values defined for this function.
//
//  Globals:
//     default_rda_status
//     default_comms_status
//
//  Notes:
//
////////////////////////////////////////////////////////////////////\*/
static void Write_rda_status(void){

   RDA_status_t *buf = NULL ;
   int buflen ;
   LB_id_t id = RDA_STATUS_ID ;
   int ret ;

   buflen = sizeof(RDA_status_t) ;

   buf = (RDA_status_t *) calloc((size_t) 1, (size_t) buflen) ;
   if (buf == NULL) {
      (void) perror("calloc") ;
      return ;
   }

   (void) memcpy(&buf->status_msg,
                 &default_rda_status,
                 sizeof(RDA_status_msg_t)) ; 

   (void) memcpy(&buf->wb_comms,
                 &default_comms_status[Comms_default],
                 sizeof(RDA_RPG_comms_status_t)) ; 

   ret = ORPGDA_write(ORPGDAT_GSM_DATA, (char *) buf, buflen, id) ;
   if( ret != buflen ) {
      LE_send_msg( GL_ERROR,
                   "ORPGDA_write (RDA_STATUS_ID) failed: %d\n", ret);
    }
    else {
        LE_send_msg(GL_INFO,
                    "Data ID %d: wrote %d bytes to RDA_STATUS_ID (%d)",
                    ORPGDAT_GSM_DATA, ret, RDA_STATUS_ID) ;
    }

   /*
     Free memory.
   */
   if( buf != NULL )
      free( buf );

   return ;

/*END of Write_rda_status()*/
}

/*\////////////////////////////////////////////////////////////////////
//
//  Description:
//     Writes the RDA Previous State message of the General Status
//     Message Linear Buffer.  The RDA Previous State is initialized
//     with default values defined at the top of this file if the
//     message does not exist or it is a 0 length message.
//
//  Inputs:
//
//  Outputs:
//
//  Returns:
//     There are no return values defined for this function.
//
//  Globals:
//
//  Notes:
//
////////////////////////////////////////////////////////////////////\*/
static void Write_previous_rda_state(void){

   LB_id_t id = PREV_RDA_STAT_ID ;
   char *buf = NULL;
   int ret ;

   ret = ORPGDA_read( ORPGDAT_GSM_DATA, (char *) &buf, LB_ALLOC_BUF, id) ;
   if( ret == sizeof(Previous_state_t) ){

      /* The message data exists.  Free the buffer and return. */
      if( buf != NULL )
         free(buf);

      LE_send_msg( GL_INFO, "Previous RDA State Exists.  Data Not Updated\n" );
      return;

   }
 
   /* Determine the default VCP.  This will be used to set the default VCP 
      in the default previous state. */
   Default_previous_state.current_vcp_number = Def_vcp; 

   /* Need to reinitialize the message. */
   ret = ORPGDA_write( ORPGDAT_GSM_DATA, (char *) &Default_previous_state, 
                       sizeof(Previous_state_t), id) ;
   if( ret != sizeof(Previous_state_t) ) {

      LE_send_msg( GL_ERROR,
                   "ORPGDA_write (PREV_RDA_STAT_ID) failed (%d)\n", ret);
      exit(1);

    }
    else {
        LE_send_msg(GL_INFO,
                    "Data ID %d: wrote %d bytes to PREV_RDA_STAT_ID (%d)",
                    ORPGDAT_GSM_DATA, ret, PREV_RDA_STAT_ID) ;
    }

   /* Free memory. */
   if( buf != NULL )
      free( buf );

   return ;

/*END of Write_previous_rda_state()*/
}

/*\///////////////////////////////////////////////////////
//
//  Description:
//     Sets the initial_vol flag in the Volume Status message.
//
//     If volume status fails to read or write, module exits.
//
//  Inputs:
//
//  Outputs:
//
//  Returns:
//     -1 on failure, 0 on success.
//
///////////////////////////////////////////////////////\*/
static int Update_vol_stat_gsm(void){

   int ret;
   LB_id_t id = VOL_STAT_GSM_ID ;
   Vol_stat_gsm_t vol_stat;

   /* 
     Read volume status. If read fails, return -1.
   */
   ret = ORPGDA_read ( ORPGDAT_GSM_DATA, (char *) &vol_stat, 
                       sizeof(Vol_stat_gsm_t), id);
   if( ret <= 0 ){

      LE_send_msg( 0, 
         "ORPGDA_read (VOL_STAT_GSM_ID) failed. Set Defaults (ret = %d)\n", 
                       ret);

      return (-1);

   }

   vol_stat.initial_vol = 1;

   /* 
     Write volume status.  If write fails, return -1;
   */
   ret = ORPGDA_write ( ORPGDAT_GSM_DATA, (char *) &vol_stat, 
                        sizeof(Vol_stat_gsm_t), id);
    if( ret != (int) sizeof(Vol_stat_gsm_t) ){
      LE_send_msg( GL_ERROR, 
                  "ORPGDA_write (VOL_STAT_GSM_ID) failed (%d). Set Defaults\n", 
                  ret);

      return (-1);
    }
    else {
        LE_send_msg(GL_INFO,
                    "Data ID %d: wrote %d bytes to VOL_STAT_GSM_ID (%d)",
                    ORPGDAT_GSM_DATA, ret, VOL_STAT_GSM_ID) ;
    }

   return (0);

/* END of Update_vol_stat_gsm() */
}

/*\////////////////////////////////////////////////////////////////////
//
//  Description:
//     Writes the Volume-Based status message of the General 
//     Status Message Linear Buffer.
//
//  Inputs:
//
//  Outputs:
//
//  Returns:
//     -1 on error, 0 otherwise.
//
//  Globals:
//
//  Notes:
//
////////////////////////////////////////////////////////////////////\*/
static int Write_vol_stat_gsm(){

   int ret, i;
   int          vcp_ind                 = 0;
   int		dual_pol_expected	= 0;
   Vcp_struct   *vcp_table              = NULL;
   char         *buf			= NULL;
   double	value			= 0.0;


   /* Initialize the Volume Status Structure. */
   memset( &Vol_stat, 0, sizeof( Vol_stat_gsm_t ) ); 

   /* Get the default weather mode for startup. The default    */
   /* weather mode is defined in the site adaptation data      */
   /* group.  It can be either "Precipitation" or "Clear Air". */
   /* Failure occurs if the adaptation data can't be accessed.  */
   if( (ret = DEAU_get_string_values( "site_info.wx_mode", &buf )) > 0 ){

      if( !strncmp( buf, "Precipitation", 13 ) ){

         Def_wxmode = (int) ORPGSITE_PRECIP_MODE;

         if( (ret = DEAU_get_values( "site_info.def_mode_A_vcp", &value, 1 )) > 0 )
            Def_vcp = (int) value;

         else{

            LE_send_msg( GL_ERROR,
                         "Error retrieving default VCP for Mode A. (%d)\n", ret );
            return(-1);

         }

      }
      else if( !strncmp( buf, "Clear Air", 9 ) ){

         Def_wxmode = ORPGSITE_CLEAR_AIR_MODE;

         if( (ret = DEAU_get_values( "site_info.def_mode_B_vcp", &value, 1 )) > 0 )
            Def_vcp = (int) value;

         else{

            LE_send_msg( GL_ERROR,
                         "Error retrieving default VCP for Mode B. (%d)\n", ret );
            return(-1);

         }

     }

     LE_send_msg( GL_INFO, "Volume Status:\n" );
     LE_send_msg( GL_INFO, "--->VCP: %d, Weather Mode: %d\n", Def_vcp, Def_wxmode );

   }

   /*
     Find index into vcp table for this vcp.
   */
   if( (vcp_ind = ORPGVCP_index( Def_vcp )) < 0 ){

      /* 
        VCP not found in vcp table.
      */
      LE_send_msg( 0, "Default VCP Number (%d) Not Found In VCP Table.\n",
                   Def_vcp );
      return(-1);

   }

   /*
     Clear the Dual Pol Expected flag. 
   */
   Vol_stat.dual_pol_expected = 0;

   /*
     Get the number of elevation angles.
   */
   Vol_stat.num_elev_cuts = ORPGVCP_get_num_elevations( Def_vcp );
   LE_send_msg( GL_INFO, "--->VCP %d Has %d Elevation Cuts.\n",
                Def_vcp, Vol_stat.num_elev_cuts );

   /*
     Get elevation angle (*10), elevation index, and set super res bit map.
   */
   for( i = 0; i < Vol_stat.num_elev_cuts; i++ ){

      float elev_angle = ORPGVCP_get_elevation_angle( Def_vcp, i );
      int super_res = ORPGVCP_get_super_res( Def_vcp, i );
      Vol_stat.elevations[i] = Round( elev_angle*10.0 );
      Vol_stat.elev_index[i] = ORPGVCP_get_rpg_elevation_num( Def_vcp, i );
      if( super_res & VCP_HALFDEG_RAD )
         Vol_stat.super_res_cuts |= (1 << (Vol_stat.elev_index[i]-1) );

      if( (dual_pol_expected = ORPGVCP_get_dual_pol( Def_vcp, i )) )
         Vol_stat.dual_pol_expected = 1;

      LE_send_msg( GL_INFO, "----->Elev %d:  Ang: %d, RPG Ind: %d, Super Res: %d, Dual Pol: %d\n",
                   i, Vol_stat.elevations[i], Vol_stat.elev_index[i], 
                   Vol_stat.super_res_cuts, dual_pol_expected );
   }

   /* 
     Initialize the weather mode. 
   */
   Vol_stat.mode_operation = Def_wxmode;

   /*
     Initialize the vcp number.
   */
   Vol_stat.vol_cov_patt = Def_vcp;

   /*
     Initialize the index of the VCP in the VCP table.
   */
   Vol_stat.rpgvcpid = vcp_ind + 1;
   LE_send_msg( GL_INFO, "--->RPG VCP ID:  %d\n", Vol_stat.rpgvcpid );

   /* 
     Initialize the VCP data. 
   */
   vcp_table = (Vcp_struct *) ORPGVCP_ptr( vcp_ind );
   memcpy( &Vol_stat.current_vcp_table, vcp_table, sizeof( Vcp_struct ) );

   /*
     Initialize expected volume duration.
   */
   Vol_stat.expected_vol_dur = ORPGVCP_get_vcp_time( Def_vcp );
   LE_send_msg( GL_INFO, "--->Expected VCP Duration:  %d\n", 
                Vol_stat.expected_vol_dur );

   /*
     Initialize the initial volume flag. 
   */
   Vol_stat.initial_vol = 1;

   /*
     Initialize the current volume scan date and time.
   */
   Vol_stat.cv_time = (unsigned long) 0;
   Vol_stat.cv_julian_date = 1;

   /*
    Write the Volume Status.
   */
   ret = ORPGDA_write ( ORPGDAT_GSM_DATA, (char *) &Vol_stat, 
                        sizeof(Vol_stat_gsm_t), VOL_STAT_GSM_ID );
   if( ret < 0 ){

      LE_send_msg( 0, "ORPGDA_write (VOL_STAT_GSM_ID) failed (%d)\n", ret);
      return(-1);

   }
   else
      LE_send_msg(GL_INFO,
                  "Data ID %d: wrote %d bytes to VOL_STAT_GSM_ID (%d)",
                    ORPGDAT_GSM_DATA, ret, VOL_STAT_GSM_ID) ;

   return(0);

/*END of write_vol_stat_gsm()*/

}

/*\////////////////////////////////////////////////////////////////////
//
//  Description:
//     Writes the Weather Status message of the General Status Message 
//     Linear Buffer.
//           
//  Inputs:  
//
//  Outputs:    
//                                                            
//  Returns:    
//     There are no return values defined for this function.
//           
//  Globals:
//           
//  Notes:
//
////////////////////////////////////////////////////////////////////\*/
static void Write_wx_status(){

    Wx_status_t wx_status;
    Siteadp_adpt_t site_data;
    double deau_ret_val;
    int ret;
    char *ds_name = NULL;
          
    if( ORPGDA_read( ORPGDAT_GSM_DATA, (char *) &wx_status,
                     sizeof(Wx_status_t), WX_STATUS_ID ) > 0)
       return;
       
    /* Set the status information. */

    /* The default weather status and current VCP will be set based on
       default weather mode and VCP. */
    wx_status.current_wxstatus = (int) WX_STATUS_UNDEFINED;
    wx_status.current_vcp = (int) WX_STATUS_UNDEFINED;
    ret = ORPGSITE_get_site_data( &site_data );
    if( ret >= 0 ){

       wx_status.current_wxstatus = site_data.wx_mode;
       if( site_data.wx_mode == PRECIPITATION_MODE )
          wx_status.current_vcp = site_data.def_mode_A_vcp;

       else
          wx_status.current_vcp = site_data.def_mode_B_vcp;

    }

    wx_status.recommended_wxstatus = (int) WX_STATUS_UNDEFINED;
    wx_status.recommended_wxstatus_start_time = (time_t) WX_STATUS_UNDEFINED;
    wx_status.recommended_wxstatus_default_vcp = (int) WX_STATUS_UNDEFINED;
    wx_status.wxstatus_deselect = (int) WX_STATUS_UNDEFINED;
    wx_status.conflict_start_time = (time_t) WX_STATUS_UNDEFINED;
    wx_status.current_wxstatus_time = (time_t) WX_STATUS_UNDEFINED;
    wx_status.precip_area = (float) WX_STATUS_UNDEFINED;

    /* Set the adaptation data. */
    ds_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA );
    if( ds_name != NULL ){

       DEAU_LB_name( ds_name );

       if( DEAU_get_values( "alg.mode_select.precip_mode_zthresh", &deau_ret_val, 1) >= 0 )
           wx_status.mode_select_adapt.precip_mode_zthresh = (int) deau_ret_val;

       else{
           
           LE_send_msg( GL_ERROR, "DEAU_get_values( alg.mode_select.precip_mode_zthresh ) Failed\n" );
           exit(1);

       }

       if( DEAU_get_values( "alg.mode_select.precip_mode_area_thresh", &deau_ret_val, 1) >= 0 )
           wx_status.mode_select_adapt.precip_mode_area_thresh = (int) deau_ret_val;

       else{
           
           LE_send_msg( GL_ERROR, "DEAU_get_values( alg.mode_select_thresh.precip_mode_area_thresh ) Failed\n" );
           exit(1);

       }

       if( DEAU_get_values( "alg.mode_select.auto_mode_A", &deau_ret_val, 1) >= 0 )
           wx_status.mode_select_adapt.auto_mode_A = (int) deau_ret_val;

       else{
           
           LE_send_msg( GL_ERROR, "DEAU_get_values( alg.mode_select_thresh.auto_mode_A ) Failed\n" );
           exit(1);

       }

       if( DEAU_get_values( "alg.mode_select.auto_mode_B", &deau_ret_val, 1) >= 0 )
           wx_status.mode_select_adapt.auto_mode_B = (int) deau_ret_val;

       else{
           
           LE_send_msg( GL_ERROR, "DEAU_get_values( alg.mode_select_thresh.auto_mode_B ) Failed\n" );
           exit(1);

       }

       if( DEAU_get_values( "alg.mode_select.mode_B_selection_time", &deau_ret_val, 1) >= 0 )
           wx_status.mode_select_adapt.mode_B_selection_time = (int) deau_ret_val;

       else{
           
           LE_send_msg( GL_ERROR, "DEAU_get_values( alg.mode_select_thresh.mode_B_selection_time ) Failed\n" );
           exit(1);

       }

       if( DEAU_get_values( "alg.mode_select.ignore_mode_conflict", &deau_ret_val, 1) >= 0 )
           wx_status.mode_select_adapt.ignore_mode_conflict = (int) deau_ret_val;

       else{
           
           LE_send_msg( GL_ERROR, "DEAU_get_values( alg.mode_select_thresh.ignore_mode_conflict ) Failed\n" );
           exit(1);

       }

       if( DEAU_get_values( "alg.mode_select.mode_conflict_duration", &deau_ret_val, 1) >= 0 )
           wx_status.mode_select_adapt.mode_conflict_duration = (int) deau_ret_val;

       else{
           
           LE_send_msg( GL_ERROR, "DEAU_get_values( alg.mode_select_thresh.mode_conflict_duration ) Failed\n" );
           exit(1);

       }
       if( DEAU_get_values( "alg.mode_select.use_hybrid_scan", &deau_ret_val, 1) >= 0 )
           wx_status.mode_select_adapt.use_hybrid_scan = (int) deau_ret_val;

       else{

           LE_send_msg( GL_ERROR, "DEAU_get_values( alg.mode_select_thresh.use_hybrid_scan ) Failed\n" );
           exit(1);

       }

       if( DEAU_get_values( "alg.mode_select.clutter_thresh", &deau_ret_val, 1) >= 0 )
           wx_status.mode_select_adapt.clutter_thresh = (int) deau_ret_val;

       else{

           LE_send_msg( GL_ERROR, "DEAU_get_values( alg.mode_select_thresh.clutter_thresh ) Failed\n" );
           exit(1);

       }

    }

    /* Initialize the supplemental status information to 0. */
    memset( &wx_status.a3052t, 0, sizeof( A3052t ) );

    /* Write_the status. */
    ret = ORPGDA_write( ORPGDAT_GSM_DATA, (char *) &wx_status, sizeof(Wx_status_t), WX_STATUS_ID);
    
    if( ret != (int) sizeof(Wx_status_t) ) 
      LE_send_msg( GL_ERROR, "ORPGDA_write (WX_STATUS_ID) failed: %d\n", ret );
     
    else 
        LE_send_msg( GL_INFO, "Data ID %d: wrote %d bytes to WX_STATUS_ID (%d)",
                    ORPGDAT_GSM_DATA, ret, WX_STATUS_ID );
     
        
    return;        
                    
/*END of Write_wx_status()*/
}

/*\////////////////////////////////////////////////////////////////////
//
//  Description:
//     Writes the SAIL Status/Request Message of the General Status 
//     Message Linear Buffer.
//           
//  Inputs:  
//
//  Outputs:    
//                                                            
//  Returns:    
//     There are no return values defined for this function.
//           
//  Globals:
//           
//  Notes:
//
////////////////////////////////////////////////////////////////////\*/
static void Write_sails_status(){

    int ret;
    SAILS_status_t sails_status = {0};
    SAILS_request_t sails_request = {0};

    if( ((ret = ORPGDA_read( ORPGDAT_GSM_DATA, (char *) &sails_status,
                            sizeof(SAILS_status_t), SAILS_STATUS_ID )) <= 0)
                                 ||
         ((ret = ORPGDA_read( ORPGDAT_GSM_DATA, (char *) &sails_request,
                            sizeof(SAILS_request_t), SAILS_REQUEST_ID )) <= 0) ){

       /* Initialize the SAILS GSM data. */
       ORPGSAILS_init();

   }

}


/*\////////////////////////////////////////////////////////////////////
//
//  Description:
//     Writes the Scan Summary data message of the General Status 
//     Message Linear Buffer.
//
//  Inputs:
//
//  Outputs:
//
//  Returns:
//     There are no return values defined for this function.
//
//  Globals:
//
//  Notes:
//     All values initialized to 0.
//
////////////////////////////////////////////////////////////////////\*/
static void Write_scan_summary(void){

    Summary_Data *summary = NULL;
    int ret, vol_scan;
    

    summary = (Summary_Data *) calloc( sizeof(1), sizeof( Summary_Data ) );
    if( summary != NULL ){

       /* If the message already exists, just return. */
       if( ORPGSUM_read_summary_data( (char *) summary ) > 0 ){

          free( summary );
          LE_send_msg( GL_INFO, "Scan Summary Data Available ... No Update\n" );
          return;

       }

       /* 
         If initial volume flag set in volume status, update the scan
         summary array for volume scan associated with Vol_stat.volume_number.
       */
       if( Vol_stat.initial_vol ){

          vol_scan = ORPGMISC_vol_scan_num( Vol_stat.volume_number );

          summary->scan_summary[vol_scan].volume_start_date = Vol_stat.cv_julian_date;
          summary->scan_summary[vol_scan].volume_start_time = Vol_stat.cv_time;
          summary->scan_summary[vol_scan].weather_mode = Vol_stat.mode_operation;
          summary->scan_summary[vol_scan].vcp_number = Vol_stat.vol_cov_patt;

          LE_send_msg( GL_INFO, "Scan Summary for Volume Scan %d", vol_scan );
          LE_send_msg( GL_INFO, "--->Vol Start Time: %d, Vol Start Date: %d", 
                       summary->scan_summary[vol_scan].volume_start_time,
                       summary->scan_summary[vol_scan].volume_start_date );
          LE_send_msg( GL_INFO, "--->VCP: %d, Weather Mode: %d", 
                       summary->scan_summary[vol_scan].vcp_number,
                       summary->scan_summary[vol_scan].weather_mode );

       }

       ret = ORPGSUM_set_summary_data( summary );
       if( ret != (int) sizeof(Summary_Data) ) 
          LE_send_msg( GL_ERROR, "ORPGSUM_set_summary_data() failed: %d\n", ret);
         
       else
          LE_send_msg( GL_INFO, "ORPGSUM_set_summary_data() Wrote %d Bytes", ret );

       free( summary );

    }
    else
       LE_send_msg( GL_MEMORY, "Calloc Failed For Scan Summary Data\n" );

    return ;

/*END of Write_scan_summary()*/
}

/*********************************************************************

      Description:
        This function returns the round-off integer of a float point
        number.

      Inputs: 
        r - a floating number.

      Outputs:

      Returns: 
        This function returns the round-off integer of a float point
        number.

**********************************************************************/
static int Round ( float r ){

    if ((double)r >= 0.)
        return ((int)(r + .5));
    else
        return (-(int)((-r) + .5));

/* END of Round() */
}

