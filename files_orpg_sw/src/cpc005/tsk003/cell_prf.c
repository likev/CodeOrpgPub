/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/19 19:05:54 $
 * $Id: cell_prf.c,v 1.6 2012/09/19 19:05:54 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
#define CELL_PRF_C
#include <prfselect.h>
#include <math.h>

#define DIST_MISSING	999999.0

/* Global Variables. */

/* Static Global Variables. */

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Initialize function for this module.  This function should be called
      at the beginning of volume scan.

   Inputs:
      None

   Outputs:
      None

   Returns:
      Always returns 0.

   Notes:

////////////////////////////////////////////////////////////////////////////\*/
int CT_init( ){

   Del_time = 0;
   Set_count = 0;
   Min_PRF = DOP_PRF_BEG;

   return 0;

/* End of CT_init(). */
}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Identifies the storm cell closest to the operator selected azimuth
      and range.  If the projected location is outside the processing
      range for the PRF Selection algorithm, then cell-based PRF selection
      is disabled.

   Inputs:
      selected_id - operator selected Storm Cell ID.

   Outputs:

   Returns:
      Returns 0 on success, -1 on failure.

   Notes:

////////////////////////////////////////////////////////////////////////////\*/
int CT_identify_storm( char *selected_id ){

   int i, index = -1;
   char name[4];

   /* Initialize the name. */
   memset( &name[0], 0, MAX_CHARS );

   /* Do some validation. */
   if( Prf_status.num_storms == 0 ){

      Prf_status.state = PRF_COMMAND_STORM_BASED;
      Prf_status.error_code = PRF_STATUS_NO_STORMS_FOUND;
      Prf_status.num_storms_tracked = 0;
      memset( &Prf_status.storm_data[0], 0, MAX_STORMS*sizeof(Storm_data_t) );

      /* No cell to track. */
      return 0;

   }

   RPGC_log_msg( GL_INFO, "Cell-based PRF Selection .... Finding  Info for Cell ID: %s\n",
                 selected_id );

   /* Do For All storms, trying to find a match on Storm ID. */
   for( i = 0; i < Prf_status.num_storms; i++ ){

      memset( name, 0, 4 );
      memcpy( name, &Prf_status.storm_data[i].storm_id[0], sizeof(unsigned short) );

      if( strncmp( name, selected_id, 2 ) == 0 ){

         index = i;
         break;

      }

   }

   /* Initialize the number of cells to 0. */
   Num_cells = 0;

   /* If we have a storm, find its redeeming qualities. */
   if( index >= 0 ){

      memset( Storm_info[0].storm_id, 0, 4 );
      memcpy( Storm_info[0].storm_id, name, 2 );

      Storm_info[0].cent_rng = Prf_status.storm_data[index].storm_rng;
      Storm_info[0].cent_azm = Prf_status.storm_data[index].storm_azm;

      RPGC_log_msg( GL_INFO, "Cell ID: %s, Cent Rng: %f, Cent Azm: %f\n",
                    name, Storm_info[0].cent_rng, Storm_info[0].cent_azm );

      Storm_info[0].cent_rng = Prf_status.storm_data[index].storm_rng_proj;
      Storm_info[0].cent_azm = Prf_status.storm_data[index].storm_azm_proj;

      RPGC_log_msg( GL_INFO, "-->Projected Location-->Cell ID: %s, Rng: %f, Azm: %f\n",
                    name, Storm_info[0].cent_rng, Storm_info[0].cent_azm );

      RPGC_log_msg( GL_INFO, "-->Max VIL: %f, Max Refl: %f, Ht Max Refl: %f, Ht Top: %f\n",
              Prf_status.storm_data[index].storm_vil, Prf_status.storm_data[index].storm_mx_refl,
              Prf_status.storm_data[index].storm_ht_mx_refl, Prf_status.storm_data[index].storm_ht_top );

      /* Increment the number of storms. */
      Num_cells = 1;

      /* Update the Prf Status. */
      Prf_status.state = PRF_COMMAND_CELL_BASED;
      Prf_status.num_storms_tracked = 1;
      Prf_status.ids_storms_tracked[0] = index;

      RPGC_log_msg( GL_INFO, "-->Num Storms Tracked: %d, Index: %d\n",
              Prf_status.num_storms_tracked, Prf_status.ids_storms_tracked[0] );

   }

   /* Disable storm cell tracking if no cell matching operator input found. */
   if( Num_cells == 0 ){

      RPGC_log_msg( GL_INFO, "No Storms Found for Cell-based PRF Selection .......\n" );
      Cell_based_PRF_selection = 0;
      Storm_based_PRF_selection = 1;

      /* Default selection is Storm-based PRF selection. */
      Prf_status.state = PRF_COMMAND_STORM_BASED;
      Prf_status.error_code = PRF_STATUS_CELL_OUT_OF_RANGE; 
      Prf_status.num_storms_tracked = 0;
      
   }

   return 0;

/* End of CT_identify_storm(). */
}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Update the attributes (range, azm, etc) for the cell we are currently 
      tracking.

////////////////////////////////////////////////////////////////////////////\*/
int CT_update_storm(){

   int i, cell_found = 0;

   /* Initialize the error code. */
   int error_code = PRF_STATUS_CELL_NOT_FOUND; 

   RPGC_log_msg( GL_INFO, "Cell-based PRF Selection .... Updating Info for Cell ID: %s\n",
                 &Storm_info[0].storm_id[0] );

   /* Do For All storms, trying to find a match on Storm ID. */
   for( i = 0; i < Prf_status.num_storms; i++ ){

      char name[4];

      memset( name, 0, 4 );
      memcpy( name, &Prf_status.storm_data[i].storm_id[0], sizeof(unsigned short) );

      if( strncmp( name, &Storm_info[0].storm_id[0], 2 ) == 0 ){

         RPGC_log_msg( GL_INFO, "Cell ID: %s Found and Being Tracked\n", name );

         Storm_info[0].cent_rng = Prf_status.storm_data[i].storm_rng_proj;
         Storm_info[0].cent_azm = Prf_status.storm_data[i].storm_azm_proj;

         RPGC_log_msg( GL_INFO, "-->Projection-->Cell ID: %s, Rng: %f, Azm: %f\n",
                       name, Storm_info[0].cent_rng, Storm_info[0].cent_azm );

         RPGC_log_msg( GL_INFO, "-->Max VIL: %f, Max Refl: %f, Ht Max Refl: %f, Ht Top: %f\n",
              Prf_status.storm_data[i].storm_vil, Prf_status.storm_data[i].storm_mx_refl,
              Prf_status.storm_data[i].storm_ht_mx_refl, Prf_status.storm_data[i].storm_ht_top );

         /* The index may have changed ... so update it. */
         Prf_status.ids_storms_tracked[0] = i;

         RPGC_log_msg( GL_INFO, "-->Num Storms Tracked: %d, Index: %d\n",
                       Prf_status.num_storms_tracked, Prf_status.ids_storms_tracked[0] );

         cell_found = 1;

         /* Is cell outside of maximum processing range? */
         if( Storm_info[0].cent_rng > MAX_PROC_RNG ){

            RPGC_log_msg( GL_INFO, "Cell Projected to be Outside %d km\n",
                          MAX_PROC_RNG );
            RPGC_log_msg( GL_INFO, "No Storms Found for Cell-based PRF Selection .......\n" );

            error_code = PRF_STATUS_CELL_OUT_OF_RANGE; 
            cell_found = 0;

         }

         break;

      }

   }

   /* Disable storm cell tracking if no cell having with ID we are tracking is found. */
   if( !cell_found ){

      RPGC_log_msg( GL_INFO, "No Storms Found for Cell-based PRF Selection .......\n" );
      Cell_based_PRF_selection = 0;

      /* Default selection is Storm-based PRF selection. */
      Prf_status.state = PRF_COMMAND_STORM_BASED;
      Prf_status.error_code = error_code;
      Prf_status.num_storms_tracked = 0;

      /* Set the flag to indicate we are defaulting to Storm-based PRF Selection. */
      Storm_based_PRF_selection = 1;
      
   }

   return 0;

/* End of CT_update_storm(). */
}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Determine if the 20 km circle of influence intersects this radial.  
      If it does, set the start and end bin for subsequent processing.  Also
      set the corresponding bit in Bitmap.  

   Inputs:
      rad_hdr - pointer to radial header.
      first - first good bin on radial.
      last - last good bin on radial.

   Outputs:
      first - first bin to check for obscuration.
      last - last bin to check for obscuration.

   Returns:
      Alwaus returns 0.

   Notes:
      At the present time, Bitmap is used for testing and debug purposes.

////////////////////////////////////////////////////////////////////////////\*/
int CT_start_end_bin( Base_data_header *rad_hdr, int *first, int *last ){

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

   /* The value Num_cells should always be 1 .... Nonetheless, Do For All Num_cells. */
   for( i = 0; i < Num_cells; i++ ){

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

/* End of CT_start_end_bin(). */
}
