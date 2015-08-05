/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:05 $
 * $Id: prcpacum_fill_supl_missing.c,v 1.1 2005/03/09 15:43:05 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_fill_supl_missing.c 

   Description
   ===========
        This function fills the supplementary data array portion of the output
   buffer with various flags and data for the case where it is determined that
   the current volume scan will not be processed because either the bad scan
   flag was set or the time difference between scans exceeded the threshold.
   If the the bad scan flag is set or if both reference scan dates and times
   are zero, then the missing dates and times in the supplementary data array
   portion of the output buffer are set to zero; Otherwise, they are set to 
   the values of the average and reference scan dates and times from the
   supplementary data array portion of the input buffer. The beginning time 
   for the hour is set to the average scan time (from input buffer) less total
   seconds per hour. The beginning date for the hour is set to the average
   scan date (from the input buffer).
   
   Change History
   ==============
   02/21/89      0000      P. Pisani            spr # 90067
   02/22/91      0001      Bayard Johnston      spr # 91254
   02/15/91      0001      John Dephilip        spr # 91762
   12/03/91      0002      Steve Anderson       spr # 92740
   12/10/91      0003      Ed Nichlas           spr 92637 pdl removal
   04/24/92      0004      Toolset              spr 91895
   03/25/93      0005      Toolset              spr na93-06801
   01/28/94      0006      Toolset              spr na94-01101
   03/03/94      0007      Toolset              spr na94-05501
   04/01/95      0008      Toolset              ccr na95-11802
   07/19/95      0009      Robert Rierson       ccr na94-35301
   12/23/96      0010      Toolset              ccr na95-11807
   03/16/99      0011      Toolset              ccr na98-23803
   06/30/03      0012      D. Miller            ccr na02-06508
   01/04/05      0013      C. Pham              ccr NA05-01303
****************************************************************************/
/* Global include files */
#include <prcprtac_main.h>
#include <a313h.h>
#include <a313hbuf.h>

/* Local include file */
#include "prcprtac_Constants.h"

void fill_supl_missing( )
{
int beg_hour, beg_day;

  if ( DEBUG ) {fprintf(stderr,"A3135R__FILL_SUPL_MISSING\n");}

/* Set zero scan to scan. if this is case for initalization or restart set
   missing variables to null vales otherwise set missing begin date/time
   to reference scan date/time and set missing end date/time to average scan
   date/time. 
*/
  AcumSupl.flg_zerscn = FLAG_SET;

  if ( (RateSupl.flg_badscn==FLAG_SET) ||
      ((RateSupl.ref_scndat==INIT_VALUE) && (RateSupl.ref_scntim==INIT_VALUE)) )
  { 
  /* No missing period */
    AcumSupl.flg_msgprd = FLAG_CLEAR;
    AcumSupl.beg_misdat = INIT_VALUE;
    AcumSupl.beg_mistim = INIT_VALUE;
    AcumSupl.end_misdat = INIT_VALUE;
    AcumSupl.end_mistim = INIT_VALUE;
  }
  else 
  {
  /* Set missing period*/
    AcumSupl.flg_msgprd = FLAG_SET;
    AcumSupl.beg_misdat = RateSupl.ref_scndat;
    AcumSupl.beg_mistim = RateSupl.ref_scntim;
    AcumSupl.end_misdat = EpreSupl.avgdate;
    AcumSupl.end_mistim = EpreSupl.avgtime;
  }

/* Set flags 'zero hourly' and 'no hourly'*/
  AcumSupl.flg_zerhly = FLAG_SET;
  AcumSupl.flg_nohrly = FLAG_SET;

/* Set begin hour to average scan time less seconds per hour (3600). If value
   negative increment results by number of seconds per day and and set begin
   day to average scan day less one. Set these values in supplemental data array
 */
  beg_hour = EpreSupl.avgtime - SEC_P_HR;

  if ( beg_hour < INIT_VALUE ) 
  {
    beg_hour = beg_hour + SEC_P_DAY;
    beg_day  = EpreSupl.avgdate + DECR;
  }
  else
  {
    beg_day  = EpreSupl.avgdate;
  }

/* Put begin and end dates and times in supplemental array*/
  AcumSupl.beg_hrdate = beg_day;
  AcumSupl.beg_hrtime = beg_hour;
  AcumSupl.end_hrdate = EpreSupl.avgdate;
  AcumSupl.end_hrtime = EpreSupl.avgtime;

/* Set hourly scan type in supplemental data array and number of interpolated
   outliers to zero and maximum hourly accumulation to zero.
 */
  AcumSupl.hly_scntyp = INIT_VALUE;
  AcumSupl.num_intout = INIT_VALUE;
  AcumSupl.max_hlyacu = INIT_VALUE;
  AcumSupl.flg_spot_blank = FLAG_CLEAR;
}
