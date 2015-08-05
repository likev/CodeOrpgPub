/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/02/06 22:47:21 $
 * $Id: pbd_set_scan_summary.c,v 1.27 2014/02/06 22:47:21 steves Exp $
 * $Revision: 1.27 $
 * $State: Exp $
 */

#include <pbd.h>

#define VOL_SPOT_BLANK 0X80000000; 

/* Static global variables. */
static Summary_Data Summary;

/* local functions */

/********************************************************************

   Description:
      This function sets fields of the scan summary data.  In
      particular, the vcp number, the weather mode, the volume
      scan start date and time, and the spot blanking status.

   Inputs:
      volume_scan_number - Volume scan sequence number.
      rpgwmode - Weather mode.
      rpg_num_elev_cuts - Number of rpg elevation cuts in VCP
      rpg_num_elev_cuts - Number of rda elevation cuts in VCP
      last_elevation_angle - angle of the last cut in the VCP
      rpg_hd - RPG radial header.
      gdb - Generic basedata radial.

   Outputs:

   Returns: 
      There is no return value defined for this function.

********************************************************************/
void SSS_set_scan_summary( int volume_scan_number, int rpgwmode,
                           int rpg_num_elev_cuts, int rda_num_elev_cuts,
                           int last_elevation_angle, 
                           Base_data_header *rpg_hd, Generic_basedata_t *gbd ){

   Scan_Summary sdata;
   int yr, mon, day, hr, min, sec, ret;
   time_t ctime;

   if( (rpg_hd->status == GOODBVOL) && (rpg_hd->elev_num == 1) ){

      /* Set the vcp number */
      Summary.scan_summary[volume_scan_number].vcp_number = rpg_hd->vcp_num;   

      /* Set weather mode */
      Summary.scan_summary[volume_scan_number].weather_mode = rpgwmode;   
           
      /* Set volume scan start time, in seconds past midnight */
      Summary.scan_summary[volume_scan_number].volume_start_time =
           rpg_hd->time/1000;

      /* Set volume scan start date, modified Julian */
      Summary.scan_summary[volume_scan_number].volume_start_date =
           rpg_hd->date;

      /* Set the number of rpg elevation cuts in current VCP. */
      Summary.scan_summary[volume_scan_number].rpg_elev_cuts = rpg_num_elev_cuts;
   
      /* Set the number of rda elevation cuts in current VCP. */
      Summary.scan_summary[volume_scan_number].rda_elev_cuts = rda_num_elev_cuts;

      /* Set Spot Blank status for this volume scan */
      Summary.scan_summary[volume_scan_number].spot_blank_status = 0;
      if( rpg_hd->spot_blank_flag & SPOT_BLANK_VOLUME )
         Summary.scan_summary[volume_scan_number].spot_blank_status =
              VOL_SPOT_BLANK;

      /* Set the AVSET status .... */
      Summary.scan_summary[volume_scan_number].avset_status = PBD_avset_status;

      /* Set the AVSET termination angle. */ 
      Summary.scan_summary[volume_scan_number].avset_term_ang = 0;
      if( PBD_avset_status == AVSET_ENABLED )
         Summary.scan_summary[volume_scan_number].avset_term_ang = last_elevation_angle;

      /* Set the number of SAILS cuts. */
      Summary.scan_summary[volume_scan_number].n_sails_cuts = PBD_N_sails_cuts_this_vol;

      /* Indicate whether or not this radial has super resolution data. */
      memset( Summary.scan_summary[volume_scan_number].super_res, 0, ECUT_UNIQ_MAX );

      /* Set the last_rda_cut and last_rpg_cut field values to undefined. */
      Summary.scan_summary[volume_scan_number].last_rda_cut = ORPGSUM_UNDEFINED_CUT;
      Summary.scan_summary[volume_scan_number].last_rpg_cut = ORPGSUM_UNDEFINED_CUT;

      LE_send_msg( GL_INFO, "Scan Summary[ %d ] Data ......\n", volume_scan_number );
      LE_send_msg( GL_INFO," --->VCP: %d, Wx Mode: %d\n", 
                   Summary.scan_summary[volume_scan_number].vcp_number,
                   Summary.scan_summary[volume_scan_number].weather_mode );
      LE_send_msg( GL_INFO," --->Start Date: %d, Start Time: %d\n", 
                   Summary.scan_summary[volume_scan_number].volume_start_date,
                   Summary.scan_summary[volume_scan_number].volume_start_time );
      LE_send_msg( GL_INFO," ---># RDA Elev Cuts: %d, # RPG Elev Cuts: %d\n", 
                   Summary.scan_summary[volume_scan_number].rda_elev_cuts,
                   Summary.scan_summary[volume_scan_number].rpg_elev_cuts );
      LE_send_msg( GL_INFO," --->AVSET Termination Angle: %d\n", 
                   Summary.scan_summary[volume_scan_number].avset_term_ang );
   } 

   /* Set the Spot Blank bitmap for this elevation */
   Summary.scan_summary[volume_scan_number].spot_blank_status |= PBD_spot_blank_bitmap; 

   /* Set the Super Resolution bitmap for this elevation. */
   if( rpg_hd->azm_reso == HALF_DEGREE_AZM ){

      Summary.scan_summary[volume_scan_number].super_res[rpg_hd->rpg_elev_ind-1] 
                                                        |= VCP_HALFDEG_RAD;

      PBD_super_res_this_elev = 1;

   }
   else
      PBD_super_res_this_elev = 0;

   /* Set the cut number if this is the last elevation for this volume .... Will be set
      different from VCP definition if AVSET is active.  Note:  We need to check
      gbd header since the RPG header has already been changed in PH_process_header. */
   if( gbd->base.status == GOODBELLC ){

      ctime = ((Summary.scan_summary[volume_scan_number].volume_start_date-1)*86400) +
                Summary.scan_summary[volume_scan_number].volume_start_time;

      Summary.scan_summary[volume_scan_number].last_rpg_cut = rpg_hd->rpg_elev_ind;
      Summary.scan_summary[volume_scan_number].last_rda_cut = rpg_hd->elev_num;

      memcpy( &sdata, &Summary.scan_summary[volume_scan_number], sizeof(Scan_Summary) );

      LE_send_msg( GL_INFO, "Posting ORPGEVT_LAST_ELEV_CUT Event (%d) -- AVSET TERMINATED VCP .....\n",
                   ORPGEVT_LAST_ELEV_CUT );
      unix_time( &ctime, &yr, &mon, &day, &hr, &min, &sec ); 
      LE_send_msg( GL_INFO, "--->Volume Scan Date/Time: %02d/%02d/%02d %02d:%02d:%02d\n", mon, day, yr,
                   hr, min, sec );
      LE_send_msg( GL_INFO, "--->Wx Mode: %2d, VCP: %4d\n", 
                   sdata.weather_mode, sdata.vcp_number );
      LE_send_msg( GL_INFO, "--->RPG Elev Cuts: %2d, RDA Elev Cuts: %2d\n", 
                   sdata.rpg_elev_cuts, sdata.rda_elev_cuts );
      LE_send_msg( GL_INFO, "--->Last RPG Cut: %2d, Last RDA Cut: %2d\n", 
                   sdata.last_rpg_cut, sdata.last_rda_cut );
      LE_send_msg( GL_INFO, "--->AVSET Termination Angle: %3d\n", 
                   sdata.avset_term_ang );

      ret = EN_post( ORPGEVT_LAST_ELEV_CUT, (void *) &sdata, sizeof(Scan_Summary), (int) 0 );
      if( ret < 0 )
         LE_send_msg( GL_ERROR, "Event ORPGEVT_LAST_ELEV_CUT Failed.  Ret = %d\n", ret );

   }
   else if( (gbd->base.status == GOODBEL) 
                       && 
            (rpg_hd->elev_num == rda_num_elev_cuts) ){

      Summary.scan_summary[volume_scan_number].last_rpg_cut = rpg_num_elev_cuts;
      Summary.scan_summary[volume_scan_number].last_rda_cut = rda_num_elev_cuts;

      /* Post the ORPGEVT_LAST_ELEV_CUT Event if AVSET is ENABLED even though
         AVSET did not terminate the VCP. */
      if( PBD_avset_status == AVSET_ENABLED ){

         ctime = ((Summary.scan_summary[volume_scan_number].volume_start_date-1)*86400) +
                   Summary.scan_summary[volume_scan_number].volume_start_time;

         memcpy( &sdata, &Summary.scan_summary[volume_scan_number], sizeof(Scan_Summary) );

         LE_send_msg( GL_INFO, "Posting ORPGEVT_LAST_ELEV_CUT Event (%d) -- AVSET ENABLED .....\n",
                      ORPGEVT_LAST_ELEV_CUT );
         unix_time( &ctime, &yr, &mon, &day, &hr, &min, &sec ); 
         LE_send_msg( GL_INFO, "--->Volume Scan Date/Time: %02d/%02d/%02d %02d:%02d:%02d\n", mon, day, yr,
                      hr, min, sec );
         LE_send_msg( GL_INFO, "--->Wx Mode: %2d, VCP: %4d\n", 
                      sdata.weather_mode, sdata.vcp_number );
         LE_send_msg( GL_INFO, "--->RPG Elev Cuts: %2d, RDA Elev Cuts: %2d\n", 
                      sdata.rpg_elev_cuts, sdata.rda_elev_cuts );
         LE_send_msg( GL_INFO, "--->Last RPG Cut: %2d, Last RDA Cut: %2d\n", 
                      sdata.last_rpg_cut, sdata.last_rda_cut );
         LE_send_msg( GL_INFO, "--->AVSET Termination Angle: %3d\n", 
                      sdata.avset_term_ang );

         ret = EN_post( ORPGEVT_LAST_ELEV_CUT, (void *) &sdata, sizeof(Scan_Summary), (int) 0 );
         if( ret < 0 )
            LE_send_msg( GL_ERROR, "Event ORPGEVT_LAST_ELEV_CUT Failed.  Ret = %d\n", ret );

      }

   }
      
   /* Write the Scan Summary data.   Abort on write error. */
   if( (ret = ORPGSUM_set_summary_data( &Summary )) < 0 )
      PBD_abort ("ORPGDA_write (SCAN_SUMMARY_ID) Failed (%d)\n", ret);

   return;

/* End of SSS_set_scan_summary() */
}

/********************************************************************

   Description:
      Performs an initial read of the scan summary data.

   Returns: 
      There is no return value defined for this function.

********************************************************************/
void SSS_init_read_scan_summary( ){

   int ret;

   if( (ret = ORPGSUM_read_summary_data( (void *) &Summary )) >= 0 )
      LE_send_msg( GL_INFO, "Scan Summary Initial Read Successful.\n" );
   
   else
      LE_send_msg( GL_INFO, "Scan Summary Initial Read Failed (%d)\n", ret );
      
   return;

/* End of SSS_init_read_scan_summary() */
}
