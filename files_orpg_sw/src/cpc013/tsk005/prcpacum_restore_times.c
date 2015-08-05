/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:21 $
 * $Id: prcpacum_restore_times.c,v 1.1 2005/03/09 15:43:21 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_restore_times.c 

   Description
   ===========
       This function restores the period and hourly header times to a twenty
   four hour orientation. each time (begin and end) in the header has subtracted
   from it the product of total seconds per day and the difference between
   minimum day and date in the particular header field. If there had been no
   day turn over then the difference between minimum day and day in the header
   field will be zero thereby leaving the time as is. If this difference is not
   zero then the time is adjusted properly.
    
   Change History
   =============
   02/21/89      0000      P. Pisani            spr # 90067
   02/22/91      0001      Bayard Johnston      spr # 91254
   02/15/91      0001      John Dephilip        spr # 91762
   12/03/91      0002      Steve Anderson       spr # 92740
   12/10/91      0003      Ed Nichlas           spr 92637 pdl removal
   04/24/92      0004      Toolset              spr 91895
   03/25/93      0005      Toolset              spr na93-06801
   01/28/94      0006      Toolset              spr na94-01101
   03/03/94      0007      Toolset              spr na94-05501
   04/11/96      0008      Toolset              ccr na95-11802
   12/23/96      0009      Toolset              ccr na95-11807
   03/16/99      0010      Toolset              ccr na98-23803
   06/30/03      0011      D. Miller            ccr na02-06508
   01/07/05      0012      Cham Pham            ccr NA05-01303
****************************************************************************/
/* Global include files */
#include <prcprtac_main.h>
#include <a313h.h>
#include <a313hbuf.h>

/* Local include file */
#include "prcprtac_Constants.h"

void restore_times( )
{
int curr_per, hour, first, prd_search;

  first = 0;

  if ( DEBUG ) {fprintf(stderr," A3135L__RESTORE_TIMES\n");}

/* If the times have been normalized then cycle through period headers to
   subtract out seconds per day from all times which previously had seconds per
   day added to them.
 */
  if ( blka.cases == EXTRAP ) 
  {
    prd_search = n3;
  }
  else
  {
    prd_search = n1;
  }     

/* Search through period header*/
  for ( curr_per=first; curr_per<=prd_search; curr_per++ ) 
  {
     if ( PerdHdr[curr_per].p_beg_date > INIT_VALUE ) 
     {
       PerdHdr[curr_per].p_beg_time = PerdHdr[curr_per].p_beg_time-
	                             (PerdHdr[curr_per].p_beg_date-
				      blka.min_day_no)*SEC_P_DAY;
       PerdHdr[curr_per].p_end_time = PerdHdr[curr_per].p_end_time-
	                             (PerdHdr[curr_per].p_end_date-
			              blka.min_day_no)*SEC_P_DAY;
     }
   }/* End loop prd_search */
 
/* If times were normalized then cycle through hour header to subtract out
   times which previously had seconds per day added to them.
 */
  for ( hour=first; hour<ACZ_TOT_HOURS; hour++ ) 
  {
     if ( HourHdr[hour].h_beg_date > INIT_VALUE ) 
     {
       HourHdr[hour].h_beg_time = HourHdr[hour].h_beg_time-
	                         (HourHdr[hour].h_beg_date-
				  blka.min_day_no)*SEC_P_DAY;
       HourHdr[hour].h_end_time = HourHdr[hour].h_end_time-
	                         (HourHdr[hour].h_end_date-
				  blka.min_day_no)*SEC_P_DAY;
     }
   }/* End loop ACZ_TOT_HOURS */

/* If times were normalized then adjust the time last precipitation
   detected by subtracting out number of seconds per day.
 */
   if ( blka.time_last_prcp > INIT_VALUE ) 
   {
     blka.time_last_prcp = blka.time_last_prcp-
                (EpreSupl.last_time_rain-blka.min_day_no)*SEC_P_DAY;
   }

}          
