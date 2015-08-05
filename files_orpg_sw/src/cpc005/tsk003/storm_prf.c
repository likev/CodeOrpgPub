/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/10/02 18:06:08 $
 * $Id: storm_prf.c,v 1.2 2012/10/02 18:06:08 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
#define STORM_PRF_C
#include <prfselect.h>
#include <math.h>

/* Global Variables. */

/* Static Global Variables. */

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Initialization function for the Storm-based PRF Selection module.

   Inputs:
      None.

   Outputs:
      None.

   Returns:
      Returns 0 on success, -1 on failure.

   Notes:

////////////////////////////////////////////////////////////////////////////\*/
int ST_init( ){

   Del_time = 0;
   Set_count = 0;
   Max_num_storms = 3;
   Min_PRF = DOP_PRF_BEG;

   Local_storm_based_PRF_selection = Storm_based_PRF_selection;

   return 0;

/* End of ST_init(). */
}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      This function identifies up to Max_num_storms to process for Storm-based
      PRF selection.   The selections starts with the most significant (as 
      published in A3cd09).  Any storm that is within PRF Selection processing 
      range and is not expected to track out of the processing range can be
      used.

   Inputs:
      None.

   Outputs:
      None.

   Returns:
      Returns 0 on success, -1 on error. 

   Notes:

////////////////////////////////////////////////////////////////////////////\*/
int ST_identify_storms(){

   int i;

   for( i = 0; i < Prf_status.num_storms; i++ ){

      /* Check the storm VIL.  If less than minimum threshold, ignore. */
      if( Prf_status.storm_data[i].storm_vil < Adapt_min_vil ){

         RPGC_log_msg( GL_INFO, "Ignoring Storm ID: %s: VIL: %f < Threshold: %f\n",
                       Prf_status.storm_data[i].storm_id, 
                       Prf_status.storm_data[i].storm_vil,
                       Adapt_min_vil );

         continue;

      }

      /* Copy the storm ID. */
      memcpy( Storm_info[Num_storms].storm_id, Prf_status.storm_data[i].storm_id, 4 );

      RPGC_log_msg( GL_INFO, "Cell ID: %s, Cent Rng: %f, Cent Azm: %f\n",
                    Storm_info[Num_storms].storm_id, Prf_status.storm_data[i].storm_rng,
                    Prf_status.storm_data[i].storm_azm );

      Storm_info[Num_storms].cent_rng = Prf_status.storm_data[i].storm_rng_proj;
      Storm_info[Num_storms].cent_azm = Prf_status.storm_data[i].storm_azm_proj;

      RPGC_log_msg( GL_INFO, "-->Projected Location-->Rng: %f, Azm: %f\n",
                    Storm_info[Num_storms].cent_rng, Storm_info[Num_storms].cent_azm );

      RPGC_log_msg( GL_INFO, "-->Max VIL: %f, Max Refl: %f, Ht Max Refl: %f, Ht Top: %f\n",
                    Prf_status.storm_data[i].storm_vil, Prf_status.storm_data[i].storm_mx_refl,
                    Prf_status.storm_data[i].storm_ht_mx_refl, Prf_status.storm_data[i].storm_ht_top );

      /* Save the index values for the storms being tracked. */
      Prf_status.ids_storms_tracked[Num_storms] = Num_storms;

      /* Increment the number of storms. */
      Num_storms++;

      /* Break out of loop if the maximum number of storms limit reached. */
      if( (Num_storms >= Max_num_storms) || (Num_storms >= MAX_STORMS ) )
         break;

   }

   /* Disable storm cell tracking if no cell matching operator input found. */
   if( Num_storms == 0 ){

      RPGC_log_msg( GL_INFO, "No Storms Found for Storm-based PRF Selection .......\n" );
      Local_storm_based_PRF_selection = 0;
      Prf_status.error_code = PRF_STATUS_NO_STORMS_FOUND;
      Prf_status.num_storms_tracked = 0;

   }
   else{

      Prf_status.error_code = PRF_STATUS_NO_ERRORS;
      Prf_status.num_storms_tracked = Num_storms;

   }

   return 0;

/* End of ST_identify_storms(). */
}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Determine the start and end bin to process this radial.   This will be
      based on the storm locations and the 20 km radius around the storm. 

   Inputs:
      rad_hdr - radial header.
      first - first bin of radial.
      last - last bin of radial.

   Outputs:
      first - if storms intersect this radial, first bin to process.
      last - if storms intersect this radial, last bin to process.

   Returns:
      0 on success, -1 on failure.

   Notes:

////////////////////////////////////////////////////////////////////////////\*/
int ST_start_end_bin( Base_data_header *rad_hdr, int *first, int *last ){

   int min_bin = MAX_PROC_RNG + 1;
   int max_bin = 0;
   int i, j, fg = *first, lg = *last;
   int r1, r2;
   float bin_size;
   Points_t points;

   /* Initialize the bin bitmap. */
   memset( Bitmap, 0, BITMAP_CHARS );

   /* Determine constants. */
   /* Conversion of range to bin number. */
   bin_size = (float) rad_hdr->surv_bin_size / 1000.0f;

   /* Do For All Num_storms. */
   for( i = 0; i < Num_storms; i++ ){

      /* Ranges intersecting 20 km circle around cell centroid. */
      points = Find_points( Storm_info[i].cent_rng, Storm_info[i].cent_azm,
                            rad_hdr->azimuth );

      /* Convert range to bin number.  Need to add 1 to the bin number because we used unit 
         indexed arrays.  If r1 < 1, because of truncation the quotient is 0 but this should
         be index 1. */
      r1 = r2 = 0;
      if( points.r1 > 0.0 )
         r1 = (int) (points.r1 / bin_size) + 1;

      if( points.r2 > 0.0 )
         r2 = (int) (points.r2 / bin_size) + 1;

      /* Check that the bin numbers are within bounds of the data. */
      if( (r1 > 0) && (r1 < fg) ) r1 = fg;
      if( r2 > lg ) r2 = lg;

      /* In the case where this radial does not intersect the 20 km circle,
         set the fg > lg to prevent the radial from being processed. */
      if( (r1 <= 0) && (r2 <= 0) ){

         r1 = -2;
         r2 = -1;

      }

      /* Set the bitmap elements. */
      for( j = r1; j <= r2; j++ ){

         if( RPGCS_bit_test( (unsigned char *) Bitmap, j ) )
            Set_count++;

         RPGCS_bit_set( (unsigned char *) Bitmap, j );

      }

      /* Get the minimum and maximum bin. */
      if( (r1 > 0) && (r1 < min_bin) )
         min_bin = r1;

      if( r2 > max_bin )
         max_bin = r2;

   }

   /* Set the first and last bins to process. */
   fg = min_bin;
   lg = max_bin;

   return 0;

/* End of ST_start_end_bin(). */
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
Points_t Find_points( float r0, float az0, float az1 ){

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

} /* End of Find_points() */

