/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/05/03 14:30:55 $
 * $Id: get_adapt_params.c,v 1.10 2011/05/03 14:30:55 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/***************************************************************************

   get_adapt_params.c

   PURPOSE:

   This routine reads in adaptable parameters.

   INPUTS:

   None.

   OUTPUTS:

   None.

   RETURNS:

   None.

   CALLS:

   None.


   NOTES:

   th_qc_chk            =  Check for comparing one value against another when
                           quality controlling dealiased values (m/s*10)

   th_fst_tplt_chk      = check used in triplet dealiasing for comparing one
                          value against another for dealiasing (m/s*10)  

   seed_unf             = threshold to use for comparison between seeds and
                          possible dealiased values (percentage of a Nyquist velocity)  

   th_overlap_size      = "tight" threshold used for matching differences between
                          the triplets. All three PRFs must fall within +- this 
                          value (m/s)  

   th_overlap_relax     = same as th_overlap_size except it is a "looser" threshold (m/s)
   
   th_seed_chk          = number of gates forward and aft along a radial a seed can
                          be used for dealiasing a given gate 

   gates_back           = number of gates backward along a radial to use when finding
                          averages for seeds
 
   gates_for            = number of gates forward along a radial to use when finding
                          averages for seeds

   max_az_bin_cnt       = maximum number of azimuthal jumps allowed between radials
                          before an error is assumed 

   max_rad_bin_cnt      = maximum number of radial jumps allowed before an error is
   			  assumed 

   max_rad_jmp          = maximum value allowed gates on the same radial (m/s*10)
   
   fix_trip_min		= number of bins before the end of the first trip to flag as 
   			  missing.  Applied to all three velocity fields.
   
   fix_trip_max		= number of bins after the end of the first trip to flag as 
   			  missing.  Applied to all three velocity fields.
  
   mpda_tover		= Minimum power difference required between echoes at ranges 
   			  corresponding to the first, second, third, and fourth trips to
   			  unfold data from the second and third Doppler scans. 
 
   max_delta_az		= Maximum number of degrees of separation allowed when aligning
   			  radials between the different scans. 

   HISTORY:

   B. Conway, 6/96      - original development
   B. Conway, 8/00      - cleanup
   C. Calvert, 6/03     - change RPGC calls to RPG calls
   C. Calvert, 11/03    - modified for new dea adaptation data format

****************************************************************************/

#ifdef DEBUG
#include <stdio.h>
#endif

#include <mpda_parameters.h>
#include "mpda_constants.h"
#include "mpda_adapt_params.h"
#include <orpgerr.h>

void get_adapt_params( void *struct_address )
{
   mpda_adapt_params_t *mpda = ( mpda_adapt_params_t * )struct_address;

   /* Set internal adaptable parameter variables   */
   /* to values from adaptable parameter structure */

   mpda_tover		= mpda -> gui_mpda_tover;
   min_trip_fix		= mpda -> gui_min_trip_fix;
   max_trip_fix		= mpda -> gui_max_trip_fix;
   seed_unf		= 0.8;
   th_overlap_size	= mpda -> gui_th_overlap_size;
   th_overlap_relax	= mpda -> gui_th_overlap_relax;
   th_seed_chk		= 80;
   gates_back		= 2;
   gates_for		= 2;
   th_qc_chk		= 200;
   th_fst_tplt_chk	= 200;
   max_az_bin_cnt	= 10;
   max_rad_bin_cnt	= 10;
   max_rad_jmp		= 300;
   use_sounding		= 1;
   max_delta_az		= 300;
       
#ifdef DEBUG
   printf( "ADPT: gui_mpda_tover \t\t= %5.2f, %5.2f\n", mpda -> gui_mpda_tover, mpda_tover );
   printf( "ADPT: gui_min_trip_fix \t= %d, %d\n", mpda -> gui_min_trip_fix, min_trip_fix );
   printf( "ADPT: gui_max_trip_fix \t= %d, %d\n", mpda -> gui_max_trip_fix, max_trip_fix );
   printf( "ADPT: gui_th_overlap_size \t= %d, %d\n", mpda -> gui_th_overlap_size, th_overlap_size );
   printf( "ADPT: gui_th_overlap_relax \t= %d, %d\n", mpda -> gui_th_overlap_relax, th_overlap_relax );
#endif

}
