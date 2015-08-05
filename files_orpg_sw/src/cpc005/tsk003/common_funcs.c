/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/06/19 20:56:04 $
 * $Id: common_funcs.c,v 1.10 2013/06/19 20:56:04 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */
#define COMMON_FUNCS_C
#include <prfselect.h>
#include <storm_ids.h>
#include <math.h>

#define MILLS_PER_DAY		86400000
#define MAX_AGE                 380		/* 1 volume scans with sails, in sec. */ 
#define MAX_AGE2                680		/* 2 volume scans with sails, in sec. */ 
#define MAX_WAIT		5		/* 5 sec. */
#define RETRACE_TIME		10		/* Average retrace time, in sec. */

/* Global Variables. */

/* Static Global Variables. */

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Initialization for Storm/Cell-based PRF selection. 

////////////////////////////////////////////////////////////////////////////\*/
int CF_init(){

   int ret, i;

   Num_cells = 0;
   Num_storms = 0;
   Cell_based_PRF_selection = 0;
   Storm_based_PRF_selection = 0;
   Local_storm_based_PRF_selection = 0;

   /* Read the PRF Command data. */
   ret = RPGC_data_access_read( ORPGDAT_PRF_COMMAND_INFO, &Prf_command,
                                sizeof(Prf_command), ORPGDAT_PRF_COMMAND_MSGID );

   /* Initialize on read error. */
   if( ret < 0 ){

      RPGC_log_msg( GL_INFO, "Init: RPGC_data_access_read(ORPGDAT_PRF_COMMAND_INFO) Failed\n" );
      RPGC_log_msg( GL_INFO, "--->ORPGDAT_PRF_COMMAND_MSGID Error:  %d\n", ret );

      /* PRF Command data. */
      Prf_command.command = PRF_COMMAND_STORM_BASED;
      memset( &Prf_command.storm_id[0], 0, MAX_CHARS );

   }

   /* Read the Prf Status data. */
   ret = RPGC_data_access_read( ORPGDAT_PRF_COMMAND_INFO, &Prf_status,
                                sizeof(Prf_status), ORPGDAT_PRF_STATUS_MSGID );

   /* Initialize on read error. */
   if( ret < 0 ){
   
      RPGC_log_msg( GL_INFO, "Init: RPGC_data_access_read(ORPGDAT_PRF_COMMAND_INFO) Failed\n" );
      RPGC_log_msg( GL_INFO, "--->ORPGDAT_PRF_STATUS_MSGID Error:  %d\n", ret );

      /* PRF Status data. */
      Prf_status.state = Prf_command.command;
      Prf_status.error_code = PRF_STATUS_NO_ERRORS;
      Prf_status.num_storms = 0;
      Prf_status.radius = (int) RADIUS;
      memset( &Prf_status.storm_data[0], 0, MAX_STORMS*sizeof(Storm_data_t) );

      Prf_status.num_storms_tracked = 0;
      for( i = 0; i < MAX_TRACKED; i++ )
         Prf_status.ids_storms_tracked[i] = -1;

   }

   /* Verify the command and state are the same.  If not, make them the same. */
   if( Prf_command.command != Prf_status.state )
      Prf_status.state = Prf_command.command;

   /* Set the flag for Storm-based PRF selection. */
   if( Prf_command.command == PRF_COMMAND_STORM_BASED ){

      RPGC_log_msg( GL_INFO, "Init: Storm-based PRF Selection Selected ....\n" );

      Storm_based_PRF_selection = 1;
      Local_storm_based_PRF_selection = Storm_based_PRF_selection;

   }
   else if( Prf_command.command == PRF_COMMAND_CELL_BASED ){

      RPGC_log_msg( GL_INFO, "Init: Cell-based PRF Selection Selected ....\n" );

      /* Validate the storm ID. */
      if( strlen( &Prf_command.storm_id[0] ) <= 0 ){

         RPGC_log_msg( GL_STATUS,
                       "Init: Missing Storm ID For Cell-Based PRF Selection\n" );

         Prf_command.command = PRF_COMMAND_STORM_BASED;
         memset( &Prf_command.storm_id[0], 0, MAX_CHARS );

         /* Set error code for status. */
         Prf_status.state = Prf_command.command;
         Prf_status.error_code = PRF_STATUS_COMMAND_INVALID;

         /* Set the flag for Storm-based PRF Selection. */
         Storm_based_PRF_selection = 1;

      }

   }

   /* Write informational messages to task log. */
   CF_write_informational_messages( "PRF Selection Initialization" );

   /* Return normal. */
   return 0;

/* End of CF_init(). */
}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      This function reads PRF Selection Algorithm adaptation data.

   Inputs:
      None.

   Outputs:
      None.

   Returns:
      Always returns 0;

   Notes:

////////////////////////////////////////////////////////////////////////////\*/
int CF_read_adapt(){

   int ret;
   double dtemp = 0;

   /* Read adaptation data to determine the minimum PRF.  Changing the Minimum
      PRF is only allowed if storm/cell-based PRF selection is enabled.  Note:  The
      minimum PRF to process needs to be determined before A30531_prf_init() 
      is called. */
   if( (ret = DEAU_get_values( "alg.prfselect.min_prf", &dtemp, 1 )) < 0 ){

      RPGC_log_msg( GL_INFO,
                    "DEAU_get_values( alg.prfselect.min_prf ) Failed (%d)\n",
                    ret );

      /* Default to DOP_PRF_BEG. */
      Min_PRF = DOP_PRF_BEG;

   }
   else
      Min_PRF = (int) dtemp;

   RPGC_log_msg( GL_INFO, "Minimum Allowable PRF: %d\n", Min_PRF );

   /* Read adaptation data to determine the number of storms to examine.  This
      only applies to Storm-based PRF selection. */
   if( (ret = DEAU_get_values( "alg.prfselect.max_num_storms", &dtemp, 1 )) < 0 ){

      RPGC_log_msg( GL_INFO,
                    "DEAU_get_values( alg.prfselect.max_num_storms ) Failed (%d)\n",
                    ret );

      /* Default to 3. */
      Adapt_max_num_storms = 3;

   }
   else
      Adapt_max_num_storms = (int) dtemp;

   RPGC_log_msg( GL_INFO, "Adaptation: Number of Storms to Examine (Storm-based Only): %d\n", 
                 Adapt_max_num_storms );

   /* Read adaptation data to determine the minimum VIL for storm/cell-based PRF 
      selection. */
   if( (ret = DEAU_get_values( "alg.prfselect.min_vil", &dtemp, 1 )) < 0 ){

      RPGC_log_msg( GL_INFO,
                    "DEAU_get_values( alg.prfselect.min_vil ) Failed (%d)\n",
                    ret );

      /* Default to 20.0. */
      Adapt_min_vil = 20.0;

   }
   else
      Adapt_min_vil = (float) dtemp;

   RPGC_log_msg( GL_INFO, "Adaptation: Minimum VIL to Examine (Storm-based Only): %f\n", 
                 Adapt_min_vil );

   return 0;

/* End of CF_read_adapt(). */
}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Reads A3CD09 data.   Will wait up to 4 seconds for this data to be 
      available.  If A3CD09 data is not available or older than 12 minutes,
      then PRF selection defaults back to legacy PRF selection. 

   Inputs:

   Outputs:

   Returns:
      Currently always returns 0.

   Notes:
      Two (2) minutes was initially selected.  It may be after sufficient 
      testing, the wait period needs to be extended.

////////////////////////////////////////////////////////////////////////////\*/
int CF_update_storm_info(){

   unsigned long storm_time;
   time_t wait= 0, init_wait = 0;

   int status, ind, num_storms, i;
   char name[4];
   float xc, yc, xs, ys;
   float cent_rng_proj, cent_azm_proj, cent_rng, cent_azm;

   time_t max_wait = MAX_WAIT;

   /* If non-operational, add retrace time. */
   if( !Operational )
      max_wait += RETRACE_TIME;

   /* Read ITC containing A3CD09 data. */
   if( Read_itc ){

      /* Clear the Read_itc flag. */
      Read_itc = 0;

      if( (status = RPGC_data_access_read( A3CD09_DATAID, &A3cd09, sizeof(a3cd09),
                                           A3CD09_MSGID )) < 0 ){

         RPGC_log_msg( GL_INFO, "RPGC_data_access_read( A3CD09 ) Failed: %d\n", status );
         A3cd09.numstrm = 0;

         return 0;

      }

   }

   /* Verify we have the most up-to-date information. */

   /* Check if the data time is no more than MAX_AGE seconds old.  If
      it is, then wait for something newer.  Don't wait more than
      5 seconds.  Note: If non-operational, add retrace time of 10
      seconds. */
   storm_time = (unsigned long) A3cd09.timetag;
   if( Vol_stat.cv_time < storm_time ){

      RPGC_log_msg( GL_INFO, "Vol_stat.cv_time: %ld < storm_time: %ld\n",
                    Vol_stat.cv_time, storm_time );
      Del_time = (Vol_stat.cv_time + (MILLS_PER_DAY - storm_time)) / 1000;

   }
   else 
      Del_time = (Vol_stat.cv_time - storm_time) / 1000;

   /* Check the age of the data.   If greater than MAX_AGE, wait a few
      seconds to see if the data gets updated. */
   if( Del_time > MAX_AGE ){

      /* Wait up to 4 seconds for A3CD09 data to be available. 
         Usually not a problem except with playback because there
         is no delay between volume scans. */
      wait = MISC_systime( NULL );
      init_wait = wait;

      while( !Read_itc && (wait - init_wait) <= max_wait ){

         /* Note: With adding EN_control( EN_SET_SIGNAL, EN_NTF_NO_SIGNAL ),
                  sleep does have the same behavior as before.  In order to
                  be interrupted by Event you must call EN_control( EN_WAIT,..)
                  where the 2nd argument is the wait time in milliseconds. */
/*         EN_control( EN_WAIT, 1000 ); */
         sleep(1);
         wait = MISC_systime( NULL );

      }

      /* Read ITC containing A3CD09 data. */
      if( Read_itc ){

         /* Clear the Read_itc flag. */
         Read_itc = 0;

         if( (status = RPGC_data_access_read( A3CD09_DATAID, &A3cd09, sizeof(a3cd09),
                                              A3CD09_MSGID )) < 0 ){

            RPGC_log_msg( GL_INFO, "RPGC_data_access_read( A3CD09 ) Failed: %d\n", status );
            A3cd09.numstrm = 0;

         }

         /* Check the data time. */
         storm_time = (unsigned long) A3cd09.timetag;
         if( Vol_stat.cv_time < storm_time ){

            RPGC_log_msg( GL_INFO, "Vol_stat.cv_time: %ld < storm_time: %ld\n",
                          Vol_stat.cv_time, storm_time );
            Del_time = (Vol_stat.cv_time + (MILLS_PER_DAY - storm_time)) / 1000;

         }
         else 
            Del_time = (Vol_stat.cv_time - storm_time) / 1000;

      }

   }

   RPGC_log_msg( GL_INFO, "wait: %ld, init_wait: %d\n", wait, init_wait );

   /* Maximum wait time exceeded .... Check if the data is too old. */
   if( Del_time > MAX_AGE2 ){

      RPGC_log_msg( GL_INFO, "A3CD09 Data Too Old.\n" );
      RPGC_log_msg( GL_INFO, "-->A3cd09.timetag: %lu, Vol_stat.cv_time: %lu\n", 
                    storm_time, Vol_stat.cv_time );
      RPGC_log_msg( GL_INFO, "-->Del_time: %lu\n", Del_time );
      A3cd09.numstrm = 0;

   }
   else{

      RPGC_log_msg( GL_INFO, "A3CD09 is %lu Seconds Old\n", Del_time );
      RPGC_log_msg( GL_INFO, "-->Number Storms: %d\n", A3cd09.numstrm );

   }

   /* Initialize the number of storms identified.  For storm-based PRF selection,
      we will always check the top # of storms.   For cell-based PRF selection,
      we check a particular storm. */
   Num_storms = 0;

   /* Do not consider storms that are either outside the processing range 
      or projected to be outside the processing range.   All valid storms 
      get copied to Prf_status buffer up to a maximum of MAX_STORMS. */
   num_storms = 0;

   if( A3cd09.numstrm > 0 )
      RPGC_log_msg( GL_INFO, "A3cd09 Storm Data\n" ); 

   /* Do For All storms in A3cd09 .... */
   for( i = 0; i < A3cd09.numstrm; i++ ){

      /* Initialize the storm ID. */
      memset( name, 0, 4 );

      /* Get the 2 char storm ID. */
      ind = A3cd09.strmid[i];
      memcpy( name, Charidtable[ind-1], sizeof(unsigned short) );

      /* Get the coordinates of the storm centroid. */
      xc = A3cd09.strmove[i][STR_XP0];
      yc = A3cd09.strmove[i][STR_YP0];

      /* Determine the centroid range and azimuth. */
      cent_rng_proj = cent_rng = sqrt( (xc*xc) + (yc*yc) );
      cent_azm = atan2( xc, yc )/DTR;
      if( cent_azm < 0.0f )
         cent_azm += 360.0;

      cent_azm_proj = cent_azm;

      /* Project the coordinates based on storm centroid motion. */
      xs = xc + (Del_time * A3cd09.strmove[i][STR_XSP] * .001f);
      ys = yc + (Del_time * A3cd09.strmove[i][STR_YSP] * .001f);

      /* Determine the projected storm centroid range and azimuth. */
      cent_rng_proj = sqrt( (xs*xs) + (ys*ys) );
      cent_azm_proj = atan2( xs, ys )/DTR;
      if( cent_azm_proj < 0.0f )
         cent_azm_proj += 360.0;

      /* Is the centroid range within the processing range of this algorithm??? */
      if( (cent_rng < MAX_PROC_RNG) 
                    && 
          (cent_rng_proj < MAX_PROC_RNG) ){

         /* Copy the storm information to PRF Status. */
         Prf_status.storm_data[num_storms].storm_rng = A3cd09.strmove[i][STR_RAN];
         Prf_status.storm_data[num_storms].storm_azm = A3cd09.strmove[i][STR_AZM];
         Prf_status.storm_data[num_storms].storm_rng_proj = cent_rng_proj;
         Prf_status.storm_data[num_storms].storm_azm_proj = cent_azm_proj;
         Prf_status.storm_data[num_storms].storm_vil = A3cd09.strmove[i][STR_VIL];
         Prf_status.storm_data[num_storms].storm_mx_refl = A3cd09.strmove[i][STR_MRF];
         Prf_status.storm_data[num_storms].storm_ht_mx_refl = A3cd09.strmove[i][STR_RFH];
         Prf_status.storm_data[num_storms].storm_ht_top = A3cd09.strmove[i][STR_TOP];

         /* Get the 2 char storm ID. */
         memcpy( Prf_status.storm_data[num_storms].storm_id, name, 4 );

         /* Write out information about the storms. */
         RPGC_log_msg( GL_INFO, "--->Storm ID: %s\n", &Prf_status.storm_data[num_storms].storm_id[0] );
         RPGC_log_msg( GL_INFO, "------>Storm Rng:  %f km\n", Prf_status.storm_data[num_storms].storm_rng );
         RPGC_log_msg( GL_INFO, "------>Storm Azm:  %f deg\n", Prf_status.storm_data[num_storms].storm_azm );
         RPGC_log_msg( GL_INFO, "------>Storm VIL:  %f kg/m^2\n", Prf_status.storm_data[num_storms].storm_vil );

         /* Increment the number of storms.   If greater than MAX_STORMS,
            jump out of loop. */
         num_storms++;
         if( num_storms >= MAX_STORMS )
            break;

      }
      else{

         /* Announce that this Storm is outside the processing range for Auto PRF. */
         RPGC_log_msg( GL_INFO, "--->Storm ID: %s @ Azm: %f deg, Rng: %f km is Outside Processing Range\n",
                       name, A3cd09.strmove[i][STR_AZM], A3cd09.strmove[i][STR_RAN] );
                       
      }

   }

   /* Set the number of storms. */
   Prf_status.num_storms = num_storms;
   RPGC_log_msg( GL_INFO, "Number of Storms in Prf Status: %d\n", Prf_status.num_storms );

   return 0;

/* End of CF_update_storm_info(). */
}


/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Determines if a radial intersects a circle at (r0, az0) with radius
      RADIUS.   

      Knowing x^2 + y^2 = R^2 for a circle of radius R and letting x = rsin(phi)
      and y = rcos(phi), then:

         r^2 - 2r0(sin(phi1)sin(phi0) + cos(phi1)cos(phi0)r + (r0^2 - radius^2) = 0

      This can be solved using quadratic formula:

         (-b +/- sqrt( b^2 - 4ac))/2a 

   Inputs:
      r0 - slant range to cell circle of influence, in km.
      az0 - azimuth angle to cell circle of influence, in deg.
      az1 - azimuth angle to test.

   Returns:
      Points_t data structure containing beginning and ending slant range.

/////////////////////////////////////////////////////////////////////////\*/
Points_t CF_find_points( float r0, float az0, float az1 ){

   Points_t pts;
   float b = -1.0f * (2.0f*r0) * (sin(az0*DTR)*sin(az1*DTR) + cos(az0*DTR)*cos(az1*DTR));
   float c = r0*r0 - RADIUS2;
   float r1, r2, q;

   /* Compute quantity b^2 - 4ac.  If quantity is negative, assign r1 and r2
      to missing. */
   q = (b*b) - (4.0*c);
   if( q < 0 ){

      pts.r1 = -999.0f;
      pts.r2 = -999.0f;

   }
   else {

      /* Determine the 2 ranges which intersect circle. */
      r1 = (-b - sqrt(q))/2.0f;
      r2 = (-b + sqrt(q))/2.0f;

      /* By definition, range has to be positive.  Because the above solution
         can produce negative r1 and r2 values 180.0 deg for az0, set these
         to missing. */
      if( (r1 < 0.0f) && (r2 < 0.0f) ){

         pts.r1 = RNG_MISSING;
         pts.r2 = RNG_MISSING;

      }
      else{

         pts.r1 = r1;
         pts.r2 = r2;

         if( pts.r1 < 0.0f )
            pts.r1 = 0.0f;

         if( pts.r2 < 0.0f )
            pts.r2 = 0.0f;

      }

   }

   return pts;

} /* End of CF_find_points() */


/*\////////////////////////////////////////////////////////////////////////

   Description:
      Writes informational messages to task log file.

////////////////////////////////////////////////////////////////////////\*/
void CF_write_informational_messages( char *header ){

   unsigned char flag = 0;

   /* Write out header. */
   if( header != NULL )
      RPGC_log_msg( GL_INFO, "%s", header );

   /* Write out information to log file. */
   RPGC_log_msg( GL_INFO, "--->PRF Command Buffer:\n" );
   RPGC_log_msg( GL_INFO, "------>Command:   %d\n", Prf_command.command );
   RPGC_log_msg( GL_INFO, "------>Storm ID:  %s\n", &Prf_command.storm_id[0] );

   RPGC_log_msg( GL_INFO, "--->Storm-Based/Cell-Based PRF Selection Flags: %d/%d\n",
                 Storm_based_PRF_selection, Cell_based_PRF_selection );

   RPGC_log_msg( GL_INFO, "--->PRF Status Buffer:\n" );
   RPGC_log_msg( GL_INFO, "------>State:   %d\n", Prf_status.state );
   RPGC_log_msg( GL_INFO, "------>Error:   %f\n", Prf_status.error_code );

   /* Check if Auto PRF Selection. */
   flag = (unsigned char) ORPGINFO_is_prf_select();
   if( flag )
      RPGC_log_msg( GL_INFO, "--->PRF Selection is AUTO\n" );

   else
      RPGC_log_msg( GL_INFO, "--->PRF Selection is MANUAL\n" );

   /* Return. */
   return;

/* End of CF_write_informational_message() */
}

