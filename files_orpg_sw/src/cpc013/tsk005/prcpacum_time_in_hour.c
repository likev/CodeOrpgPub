/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 18:19:56 $
 * $Id: prcpacum_time_in_hour.c,v 1.2 2006/02/09 18:19:56 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_time_in_hour.c

   Description
   ===========
      This function determines the non missing time in the hour and the 
   fractional part that each period is within the hour.  Upon entry, the 
   new_frac array is initialized to zero and as a default the interpolation 
   case is assumed thus setting the period index to 14 (n1) and initializing 
   the p_frac values for 15 (n2) and 16 (n3) to zero. Next, if the case is 
   for extrapolation, the period for which the end of hour occurred is 
   determined. If the end of hour occurred within or at the end of the second 
   period, then the period index is set to 15 (n2), the p_frac value for 14 
   (n1) is set to one thousand, and  the p_frac value for 16 (n3) is set to 
   zero since none of the third period is within the hour. If the end of hour 
   occurred within or at the end of the third period, then the period index is
   set to 16 (n2), the p_frac value for both first n1 (14) and second (n2) is
   set to one thousand since both are within the hour. In either case the non
   missing time is incremented by the time is incremented by the delta time 
   within the first period. The second period is not included since it is 
   missing and the contributions for the third period must still be determined.
   the beginning time for the period which contains the end of hour is then 
   subtracted from the ending time for the hour and this value used to 
   increment non missing time. This value is also divided by the delta time of
   the period and the quotient multiplied by one thousand and the result stored
   in the p_frac value of the period header for the period within which the 
   hour ends (first, second, or third). The period headers for the previous 
   hour are then searched for non missing time. The delta time value for each
   header that has non missing time is used to increment the non missing time
   total. Also, the fractional part that the period is within the hour is 
   stored in corresponding slots of the new_frac array. These fractional 
   values will all be one thousand with the possible exception of the border
   period close to the beginning of the hour.  Either all of this period or
   only a fraction of it will be in the hour. Finally, the non missing time
   total is compared against a threshold for the minimum time within an hour
   to compute hourly accumulations. If the total is greater than or equal to
   this threshold value then the flag h_flag_nhrly is cleared in the hour 
   header for the current hour indicating that hourly totals can be computed.
   If the non missing time total is less than the threshold, then this same
   flag is set indicating not to compute hourly totals.

   Change History
   ==============
   02/21/89      0000      P. Pisani            spr # 90067
   02/22/91      0001      Paul Jendrowski      spr # 91254
   02/15/91      0001      John Dephilip        spr # 91762
   12/03/91      0002      Steve Anderson       spr # 92740
   12/10/91      0003      Ed Nichlas           spr 92637 pdl removal
   04/24/92      0004      Toolset              spr 91895
   03/25/93      0005      Toolset              spr na93-06801
   01/28/94      0006      Toolset              spr na94-01101
   03/03/94      0007      Toolset              spr na94-05501
   04/01/95      0008      Toolset              ccr na95-11802
   06/02/95      0009      R. Rierson           ccr na94-35301
   12/23/96      0010      Toolset              ccr na95-11807
   03/16/99      0011      Toolset              ccr na98-23803
   12/31/02      0012      D. Miller            ccr na00-28601
   01/07/05      0013      Cham Pham            ccr NA05-01303
   10/26/06      0014      Cham Pham            ccr NA05-21401
****************************************************************************/
/* Global include files */
#include <a313hbuf.h>
#include <a313h.h>

/* Local include file */
#include "prcprtac_Constants.h"

void time_in_hour( )
{
int    local_index, prd_index, 
       non_miss_time, spot_blank_state;
double frac = 0.;

/* Initialize spot blank state*/
  spot_blank_state = FALSE;

  if (DEBUG) {fprintf(stderr," A31356__TIME_IN_HOUR\n");}

/* Initalize new-fraction array and non missing time counter to zero*/
  memset( &blka.new_frac, INIT_VALUE, NUM_PREV_PRD*sizeof(int) );

/* Initalize non missing time counter to zero */
  non_miss_time = 0;

/* Set default case to interpolate [period index set to fourteen] determine
   whether case is extapolate. If it is extrapolate set fractions for second
   and third period to zero. The index is already set to fourteen. The default 
   for extrapolate is therefore set for the end of hour within the first period.
   Therefore set for the end of hour within the first period. if the case is for
   extrapolation; determine whether the hour ends within the second or third 
   new period. If the hour ends in either of these two periods, the index is
   set accordingly and "frac" for the other two periods is set accordingly. 
 */
  prd_index = n1;
  PerdHdr[n2].p_frac = MIN_PER_CENT;
  PerdHdr[n3].p_frac = MIN_PER_CENT;

/* Check to see if PerdHdr[n1].p_spot_blank was blanked, and include in
   current hour. 
 */
  if ( PerdHdr[n1].p_spot_blank == FLAG_SET ) 
  {
    spot_blank_state = TRUE;
  }                

/* Extrapolation case*/
  if ( blka.cases == EXTRAP ) 
  {
    if ( (HourHdr[curr_hour].h_end_time > PerdHdr[n2].p_beg_time) &&
         (HourHdr[curr_hour].h_end_time <= PerdHdr[n2].p_end_time) ) 
    {
    /* Use the 2nd period*/
       prd_index = n2;
       PerdHdr[n1].p_frac = MAX_PER_CENT;
       PerdHdr[n3].p_frac = MIN_PER_CENT;
       non_miss_time += PerdHdr[n1].p_delt_time;    
    }
    else if ( (HourHdr[curr_hour].h_end_time > PerdHdr[n3].p_beg_time) &&
              (HourHdr[curr_hour].h_end_time <= PerdHdr[n3].p_end_time) )
    {
    /* Use the 3rd period*/
       prd_index = n3;
       PerdHdr[n1].p_frac = MAX_PER_CENT;
       PerdHdr[n2].p_frac = MAX_PER_CENT;
       non_miss_time += PerdHdr[n1].p_delt_time;

    /* Check to see if PerdHdr(p_spot_blank,n3) was blanked, and include in
       current hour
     */
       if ( PerdHdr[n3].p_spot_blank == FLAG_SET ) 
       {
         spot_blank_state = TRUE;
       }   
    }     
  }/* End extrap  */

/* Since the period in which the end of hour occurred has been established, it
   must now be determined what fraction of the period is within the hour. The
   fraction is determined by the ratio of the time difference of period begin
   and end of hour to the time difference of period end and period begin. This
   faction is then stored into the period header scaled 1000. if the missing
   flag for the period is clear; the non missing time total is updated.
 */ 
  frac = HourHdr[curr_hour].h_end_time - PerdHdr[prd_index].p_beg_time;

  if (DEBUG) 
    {fprintf(stderr,"FRAC = %f\n",frac);}

  if ( PerdHdr[prd_index].p_flag_miss == FLAG_CLEAR )
  {
    non_miss_time += frac;
  }

  if (DEBUG) 
    {fprintf(stderr,"NON_MISS_TIME: %d\n",non_miss_time);}

  PerdHdr[prd_index].p_frac=(frac/PerdHdr[prd_index].p_delt_time)*MAX_PER_CENT;

  if (DEBUG) 
  {
    fprintf(stderr,"PerdHdr[%d].p_frac=%d\n",
            prd_index,PerdHdr[prd_index].p_frac);
  }

/* The following code sets up the new_frac array. basically all fractions for
   periods earlier than that which contains the beginning of the hour are set
   to zero and all fractions for later periods are set to the maximum. For the
   period which contains the beginning of the hour, the fraction is computed as
   the ratio of the time difference between the end period and begin hour to the
   time difference between the end period and begin period.
 */
  local_index = blka.current_index;

/* Do for all periods from most recent to oldest while end of while period time
   is after beginning of new hour.
 */
  for (; ;) 
  {
    if ( PerdHdr[local_index].p_end_time > HourHdr[curr_hour].h_beg_time )
    {
      if ( PerdHdr[local_index].p_beg_time >= HourHdr[curr_hour].h_beg_time )
      {
       /* Use entire period*/
        frac = PerdHdr[local_index].p_delt_time;
        blka.new_frac[local_index] = MAX_PER_CENT;
      }
      else 
      {
       /* Use portion of period*/
        frac = PerdHdr[local_index].p_end_time-HourHdr[curr_hour].h_beg_time;
        blka.new_frac[local_index] = (frac/PerdHdr[local_index].p_delt_time)
                                      *MAX_PER_CENT;
      }

     /* Check for missing period*/
      if ( PerdHdr[local_index].p_flag_miss == FLAG_CLEAR ) 
      {
        non_miss_time += frac;
      }

     /* Check to see if any of the periods were spot blanked*/
      if ( PerdHdr[local_index].p_spot_blank == FLAG_SET ) 
      {
        spot_blank_state = TRUE;
      } 

      local_index += DECR;
      
      if ( local_index == INIT_VALUE ) 
      {
        local_index = NUM_PREV_PRD;
      }

     /* Simulate infinite loop with continue*/
      if ( local_index != blka.current_index ) continue;
    }

/* Exit infinite loop when local index equals to current index */
    break;  

  }/* End infinite for loop */   

/* Set spot blanking state for current hourly period*/
  if ( spot_blank_state ) 
  {
    HourHdr[curr_hour].h_spot_blank = FLAG_SET;
  }
  else
  {
    HourHdr[curr_hour].h_spot_blank = FLAG_CLEAR;
  }

/* Determine whether enough time has been accumulated for hourly totals.
   If there is enough time clear the 'no hourly' in the hourly header;
   Otherwise, set this same flag.
 */
  if ( non_miss_time >= blka.min_tim_hrly ) 
  {
    HourHdr[curr_hour].h_flag_nhrly = FLAG_CLEAR;
  }
  else
  {
    HourHdr[curr_hour].h_flag_nhrly = FLAG_SET;

/* Clear spot blank field if not enought time has accumulated*/
    HourHdr[curr_hour].h_spot_blank = FLAG_CLEAR;
  }

}
