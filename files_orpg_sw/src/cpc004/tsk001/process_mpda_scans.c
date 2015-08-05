/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/08 22:32:52 $
 * $Id: process_mpda_scans.c,v 1.3 2006/09/08 22:32:52 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/**************************************************************************

   process_mpda_scans.c

   PURPOSE:

   This routine is the driver for dealiasing the multi-PRF scans. At this
   point the scans have been range dealiased.

   CALLED FROM:

   apply_mpda

   INPUTS:

   None.

   CALLS:

   first_triplet_attempt
   second_triplet_attempt
   pairs_and_trips_attempts
   final_unf_attempts
   replace_orig_vals
   write_debug_output
   fix_radial_errors
   fix_azimuthal_errors
   despeckle_results

   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:
  
   1) It has been found that sometimes, simply because of the way
      radar gates fall, that processing backwards can be just as
      important as processing forward. At this time the routine
      goes for/back at every step.

   2) The debugging has been left in for further possible needs.
      This is the only routine that has any remaining debug calls.

   3) The final_unf_attempts has the option to use the ewt values
      or not. Again, there are cases where using the ewt will cause
      problems, and cases where it will solve problems. It is not
      clear cut when to use the ewt.  

   HISTORY:

   B. Conway, 5/96      - original development
   B. Conway, 8/00      - cleanup 

****************************************************************************/

#include "mpda_constants.h"

#ifdef DEBUG
#include <stdio.h>
#include <time.h>
#endif

#include "mpda_structs.h"

void 
first_triplet_attempt(int direction);

void 
second_triplet_attempt(int direction);

void
pairs_and_trips_attempts(int direction);

void 
final_unf_attempts(int direction, int use_ewt);

void 
replace_orig_vals();

void 
fix_radial_errors();

#ifdef DEBUG
int
count_data_bins();

void 
write_debug_output();
#endif

void dump_data();

void 
fix_azimuthal_errors();

void 
despeckle_results();
 
void
process_mpda_scans()
{

/*
   The following if statement processes data that contain more
   than 2 prfs. Each processing attempt is followed by the the
   error checking routines. Debugging statements have been left
   in this code.
*/

#ifdef DEBUG
int old_cnt=0, new_cnt=0;
#endif

if(num_prfs > PAIRS)
     {
#ifdef DEBUG
  new_cnt = count_data_bins();
  printf("first triplet attp - forward\n");
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
#endif
     first_triplet_attempt(GO_FORWARD);    
#ifdef DEBUG
  old_cnt = new_cnt;
  new_cnt = count_data_bins();
  printf("first attempt found %d gates\n", new_cnt - old_cnt);
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
  printf("first triplet attp - backward\n");
#endif
     first_triplet_attempt(GO_BACKWARD);
#ifdef DEBUG
  old_cnt = new_cnt;
  new_cnt = count_data_bins();
  printf("first attempt found %d gates\n", new_cnt - old_cnt);
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
#endif
     despeckle_results();
     fix_azimuthal_errors();
     fix_radial_errors();
#ifdef DEBUG
  new_cnt = count_data_bins();
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
  printf("second triplet attp - forward\n");
#endif
     second_triplet_attempt(GO_FORWARD);
#ifdef DEBUG
  old_cnt = new_cnt;
  new_cnt = count_data_bins();
  printf("second trip found %d gates\n", new_cnt - old_cnt);
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
  printf("second triplet attp - backward\n");
#endif
    second_triplet_attempt(GO_BACKWARD);
#ifdef DEBUG
  old_cnt = new_cnt;
  new_cnt = count_data_bins();
  printf("second trip found %d gates\n", new_cnt - old_cnt);
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
#endif
    despeckle_results();
    fix_azimuthal_errors();
    fix_radial_errors();
#ifdef DEBUG
  new_cnt = count_data_bins();
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
#endif
    }

/*
   The following processing is performed on all data - pairs
   and triplets.
*/
#ifdef DEBUG
  printf("first_pair_attempt - forward\n");
#endif
   pairs_and_trips_attempts(GO_FORWARD);
#ifdef DEBUG
  old_cnt = new_cnt;
  new_cnt = count_data_bins();
  printf("pairs found %d gates\n", new_cnt - old_cnt);
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
  printf("first_pair_attempt - backward\n");
#endif
   pairs_and_trips_attempts(GO_BACKWARD);
#ifdef DEBUG
  old_cnt = new_cnt;
  new_cnt = count_data_bins();
  printf("pairs found %d gates\n", new_cnt - old_cnt);
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
#endif
   despeckle_results();
   fix_azimuthal_errors();
   fix_radial_errors();
#ifdef DEBUG
  new_cnt = count_data_bins();
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);

/*
   The following processing is applied to all 
   data. These are the final attempts at a final 
   solution using combinations of forward/back along
   the radial both with/without EWT values.
*/

  printf("final attempts - forward\n");
#endif
   final_unf_attempts(GO_FORWARD, FALSE);
#ifdef DEBUG
  old_cnt = new_cnt;
  new_cnt = count_data_bins();
  printf("found %d gates\n", new_cnt - old_cnt);
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
  printf("final attempts - backward\n");
#endif
   final_unf_attempts(GO_BACKWARD, FALSE);
#ifdef DEBUG
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
  old_cnt = new_cnt;
  new_cnt = count_data_bins();
  printf("found %d gates\n", new_cnt - old_cnt);
#endif
   despeckle_results();
   fix_azimuthal_errors();
   fix_radial_errors();
#ifdef DEBUG
  new_cnt = count_data_bins();
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
  printf("final attempts with sounding - forward\n");
#endif
   final_unf_attempts(GO_FORWARD, TRUE);
#ifdef DEBUG
  old_cnt = new_cnt;
  new_cnt = count_data_bins();
  printf("found %d gates\n", new_cnt - old_cnt);
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
  printf("final attempts with sounding - backward\n");
#endif
   final_unf_attempts(GO_BACKWARD, TRUE);
#ifdef DEBUG
  old_cnt = new_cnt;
  new_cnt = count_data_bins();
  printf("found %d gates\n", new_cnt - old_cnt);
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
#endif
   despeckle_results();
   fix_azimuthal_errors();
   fix_radial_errors();
#ifdef DEBUG
  new_cnt = count_data_bins();
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);
/*
   This routine sets unsolved gates to the orignal value closest
   to the seed average.
*/ 

  printf("replacing original values\n");
#endif
   replace_orig_vals();
#ifdef DEBUG
  old_cnt = new_cnt;
  new_cnt = count_data_bins();
  printf("replaced %d vels\n", new_cnt - old_cnt);
  printf("Time: %6.2f\n",(float) clock()/(float) CLOCKS_PER_SEC);

  write_debug_output();
#endif
}
#ifdef DEBUG
int count_data_bins()
{
  int i,j, cnt=0;
  short *data_ptr;
    
  data_ptr = &save.final_vel[0][0];
  for(i=0;i<MAX_RADS;i++)
    for(j=0;j<MAX_GATES;j++)
      if(*data_ptr++ < DATA_THR)
        cnt++;

  return cnt;
}

#define FLOAT_100 100.
void
write_debug_output(int vol, int tilt, int pass)
{
   int i, j; 
   int rf_cnt1, rf_cnt2, rf_cnt3, rf_cnt4, total_bin_count; 
   int vel_bin_cnt1;  
   int vel_bin_cnt2;
   int vel_bin_cnt3;
   int vel_bin_cnt4;
   float rf_area1, rf_area2, rf_area3, rf_area4;
   float vel_area1, vel_area2, vel_area3, vel_area4;
   float rads, tstrng;

/*  Initialize some variables to zero  */

   rf_cnt1 = 0;
   rf_cnt2 = 0;
   rf_cnt3 = 0;
   rf_cnt4 = 0;  
   total_bin_count = 0;
   vel_bin_cnt1 = 0;
   vel_bin_cnt2 = 0;
   vel_bin_cnt3 = 0;
   vel_bin_cnt4 = 0;
   rf_area1 = rf_area2 = rf_area3 = rf_area4 = 0;
   vel_area1 = vel_area2 = vel_area3 = vel_area4 = 0;

   for(i=0;i<save.tot_prf1_rads;++i)
      {
      rads = save.tot_prf1_rads;
      for(j=0;j<MAX_GATES;++j)
        {
        tstrng = (float)j/4.-.5;

        ++total_bin_count;  
        if (save.final_vel[i][j] < DATA_THR)
          {
          ++vel_bin_cnt1;    
          vel_area1 = vel_area1 + 0.25*3.14159*(2*tstrng+0.25)/rads;
          }
        if (save.final_vel[i][j] == RNG_FLD)
          {
          ++rf_cnt1;
          rf_area1 = rf_area1 + 0.25*3.14159*(2*tstrng+0.25)/rads;
          }  
        if (save.vel1[i][j] == RNG_FLD)
          {
          ++rf_cnt2;
          rf_area2 = rf_area2 + 0.25*3.14159*(2*tstrng+0.25)/rads;
          }
        if (save.vel1[i][j] < DATA_THR)
          {
          ++vel_bin_cnt2;
          vel_area2 = vel_area2 + 0.25*3.14159*(2*tstrng+0.25)/rads;
          } 
        if (save.vel2[i][j] == RNG_FLD)
          {
          ++rf_cnt3;
          rf_area3 = rf_area3 + 0.25*3.14159*(2*tstrng+0.25)/rads;
          }
        if (save.vel2[i][j] < DATA_THR)
          {
          ++vel_bin_cnt3;
          vel_area3 = vel_area3 + 0.25*3.14159*(2*tstrng+0.25)/rads;
          }
        if (save.vel3[i][j] == RNG_FLD)
          {
          ++rf_cnt4;
          rf_area4 = rf_area4 + 0.25*3.14159*(2*tstrng+0.25)/rads;
          }      
        if (save.vel3[i][j] < DATA_THR)
          {
          ++vel_bin_cnt4;
          vel_area4 = vel_area4 + 0.25*3.14159*(2*tstrng+0.25)/rads;
          }
      }
   }
   printf(" # of rf bins in Triple is %d\n", rf_cnt1);  
   printf(" # of rf bins in vel1 is   %d\n", rf_cnt2);   
   printf(" # of rf bins in vel2 is   %d\n", rf_cnt3);    
   printf(" # of rf bins in vel3 is   %d\n\n", rf_cnt4);    
   printf(" total bin count = %d\n\n", total_bin_count);  
   printf(" # of vel bins in Triple is %d\n", vel_bin_cnt1);  
   printf(" # of vel bins in Vel1 is   %d\n", vel_bin_cnt2);    
   printf(" # of vel bins in Vel2 is   %d\n", vel_bin_cnt3);    
   printf(" # of vel bins in Vel3 is   %d\n\n", vel_bin_cnt4);    
   printf(" Area of rf in Triple is  %10.2f\n", rf_area1);
   printf(" Area of rf in vel1 is    %10.2f\n", rf_area2);
   printf(" Area of rf in vel2 is    %10.2f\n", rf_area3);
   printf(" Area of rf in vel3 is    %10.2f\n", rf_area4);
   printf(" Area of vel in Triple is %10.2f\n", vel_area1);
   printf(" Area of vel in Vel1 is   %10.2f\n", vel_area2);
   printf(" Area of vel in Vel2 is   %10.2f\n", vel_area3);
   printf(" Area of vel in Vel3 is   %10.2f\n\n", vel_area4);
   return;
}

#endif
