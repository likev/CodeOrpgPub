/* RCS info */
/* $Author:  */
/* $Locker:   */
/* $Date: */
/* $Id:  */
/* $Revision:  */
/* $State: */

/* ===Project: DualPol ===== Author: Brian Klein =====Jun 2013 =============
 *
 * Module Name: end_of_elev_proc.c
 *
 * Module Version: 1.0
 *
 * Module Language: c
 *
 * Change History:
 *
 * Date    Version    Programmer  Notes
 * -------------------------------------------------------------
 * 06/06/13  1.0    Brian Klein  Creation
 *
 *
 * Description:  If the input isdp is a valid estimate at the end of volume,
 *		 function writes the estimate to dea data along with the
 *		 volume scan time of the estimate.  If the end of volume
 *		 is reached, write the estimated ISDP message to the system
 *		 status log at top of hour.  If an elevation has too many
 *               radials with weather too close reject the whole elevation
 *               because contamination by nearby echoes may adversely
 *               impact the estimate.
 *
 * ======================================================================== */
#include <rpgc.h>
#include <rpgcs.h>
#include <orpgsite.h>
#include <orpgred.h>
#include "dpprep.h"
#include <dpprep_isdp.h>

/* Global Variables */
extern short  Init_index; /* 0 -> init the queue index */
extern float  Current_system_phidp;
extern time_t Current_time;
extern int    Current_hour;
extern int    Previous_hour;
extern float  RDA_system_phidp;
extern short  Too_close_count;
extern Dpp_isdp_est_t Isdp_est;

int DPPC_end_of_elev_proc(const int  vol_num,  const int radial_status, const float isdp)
{
	int est_sys_phi, est;
	int rda_value, delta;
        int ret = 0;
	int yy = 1970, mm = 1, dd = 1, hr = 0, min = 0, sec = 0, mills = 0;
        int redundant_type = NO_REDUNDANCY;
        int channel_num = 1;
        char* appd;
	Scan_Summary *scan_summary;
        Redundant_info_t redundant_info;

        /* Make sure there aren't too many radials with weather too close */
        /* to the radar this elevation.  Nearby weather may adversely     */
        /* impact the estimate.                                           */
        if (Too_close_count > MAX_TOO_CLOSE ) {
           Init_index = 0;
        }

	/* At the end of volume, update the estimated ISDP only if it's valid */
        if (radial_status == GENDVOL ) {

		if (isdp != -99 && Too_close_count <= MAX_TOO_CLOSE ) {
		   scan_summary = RPGC_get_scan_summary(vol_num);
		   RPGCS_julian_to_date (scan_summary->volume_start_date,&yy,&mm,&dd);
		   RPGCS_convert_radial_time (scan_summary->volume_start_time*1000,
				                   &hr, &min, &sec, &mills);

		   /* Copy the current estimate and set the time at which */
                   /* the ISDP estimate was updated.                      */
                  
		   /* Subtract the century out of the year. */
		   if( yy >= 2000.0 )  yy -= 2000;

		   est = RPGC_NINT(isdp);
                 
		   /* Save the current IDSP estimate */
		   LE_send_msg( GL_INFO, "Updating ISDP Estimate. ISDP= %d\n", est);
		   Isdp_est.isdp_est = est;
		   Isdp_est.isdp_yy = yy;
                   Isdp_est.isdp_mm = mm;
                   Isdp_est.isdp_dd = dd;
                   Isdp_est.isdp_hr = hr;
                   Isdp_est.isdp_min = min;

                   if( (ret = ORPGDA_write( DP_ISDP_EST, (char *) &Isdp_est.isdp_est, 
                                            sizeof(Dpp_isdp_est_t), DP_ISDP_EST_MSGID)) < 0){

                   	 LE_send_msg( GL_INFO, "ISDP Estimate Update Date/Time Failed (%d)\n", ret );
                         return (ret);
		   }


                } /* end if isdp estimate was valid */

		/* Reset the queue each volume */
		Init_index = 0;
       	        Current_system_phidp = -99;

	} /* end if end of volume */

	/* Reset the counter each elevation */
       	Too_close_count = 0;

	/* Determine if it is time to produce the estimated System PhiDP message */
        /* Decision made to output the message once per hour and any volume in   */
        /* which it is out of tolerance.                                         */
	Current_time = time( NULL );
	Current_hour = Current_time / SECS_IN_HOUR;

        if (radial_status == GENDVOL ) {
           est_sys_phi = Isdp_est.isdp_est;
	   rda_value   = RPGC_NINT(RDA_system_phidp);

           /* Compute the delta between the RPG estimate and the RDA target value */
           if (est_sys_phi != -99)
              delta = est_sys_phi - rda_value;
           else
              delta = 0;

	   if( (Current_hour != Previous_hour) 
                	||
               (abs(delta) >= ISDP_WARNING_THRESH)){

		Previous_hour = Current_hour;

           	/* Get Redundant Type. */
		if( ORPGSITE_get_redundant_data( &redundant_info ) >= 0 )
			redundant_type = redundant_info.redundant_type;

           	/* Get channel number. */
		if( (channel_num = ORPGRDA_channel_num()) < 0 )
			channel_num = 1;

		if (est_sys_phi == -99) {

			if( (redundant_type == FAA_REDUNDANT)
                                           ||
                            (redundant_type == NWS_REDUNDANT) )
 
				RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG, 
				"ISDP Check [RDA:%d] (RPG est./Delta/RDA value/Applied/Quality): Insuf. Data/ unk/ %d/ unk/ unk", 
				channel_num, rda_value);

			else
				RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG, 
				"ISDP Check (RPG est./Delta/RDA value/Applied/Quality): Insuf. Data/ unk/ %d/ unk/ unk", 
				rda_value);
		}
		else {

                	if (((ret = ORPGDA_read( DP_ISDP_EST, (char *) &Isdp_est.isdp_est, 
                                                sizeof(Dpp_isdp_est_t), DP_ISDP_EST_MSGID )) < 0)
                                                                   ||
			    ((ret = DEAU_get_string_values ("alg.dpprep.isdp_apply",&appd)) <= 0)) {
				LE_send_msg (GL_INFO, 
				"Adaptation data/Estimated ISDP not available (%d)", ret);
				return (ret);
			}

			est_sys_phi = Isdp_est.isdp_est;
			mm = Isdp_est.isdp_mm;
			dd = Isdp_est.isdp_dd;
			yy = Isdp_est.isdp_yy;
			hr = Isdp_est.isdp_hr;
			min = Isdp_est.isdp_min;
			sec = Isdp_est.isdp_sec;

                        if (abs(delta) < ISDP_WARNING_THRESH) {

  				if( (redundant_type == FAA_REDUNDANT)
                             	    	            ||
                            	    (redundant_type == NWS_REDUNDANT) )
					RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG, 
					"ISDP Check [RDA:%d] (RPG est./Delta/RDA value/Applied/Quality): %d/ %d/ %d/ %s/ okay at %02d/%02d/%02d %02d:%02d", 
					channel_num, est_sys_phi, delta, rda_value, appd, mm, dd, yy, hr, min);
				else
					RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG, 
					"ISDP Check (RPG est./Delta/RDA value/Applied/Quality): %d/ %d/ %d/ %s/ okay at %02d/%02d/%02d %02d:%02d", 
					est_sys_phi, delta, rda_value, appd, mm, dd, yy, hr, min);
			}
                       	else {

  				if( (redundant_type == FAA_REDUNDANT)
                             	    	            ||
                            	    (redundant_type == NWS_REDUNDANT) )
					RPGC_log_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
					"ISDP Check [RDA:%d] (RPG est./Delta/RDA value/Applied/Quality): %d/ %d/ %d/ %s/ BAD at %02d/%02d/%02d %02d:%02d", 
					channel_num, est_sys_phi, delta, rda_value, appd, mm, dd, yy, hr, min);
				else
					RPGC_log_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
					"ISDP Check (RPG est./Delta/RDA value/Applied/Quality): %d/ %d/ %d/ %s/ BAD at %02d/%02d/%02d %02d:%02d", 
					est_sys_phi, delta, rda_value, appd, mm, dd, yy, hr, min);

			}
		}
           }
	} /* end if time to output status message */

	return (0);

} /* end of DPPC_end_of_elev_proc */

