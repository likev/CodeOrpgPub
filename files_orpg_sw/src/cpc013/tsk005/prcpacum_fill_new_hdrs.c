/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:01 $
 * $Id: prcpacum_fill_new_hdrs.c,v 1.1 2005/03/09 15:43:01 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_fill_new_hdrs.c

   Description
   ===========
       This function fills one or three new headers, depending on the case 
   (interpolation or extrapolation) with starting and nding dates/times;
   flag zero and flag missing; And time differnce between end and begin times
   within the period. The one or three new headers are appended to the previous
   period scan headers read from disk. If the case is for interpolation, then
   only one period is appended. If the case is for extrapolation then three new
   periods are added. For the interpolation case the period begin and end
   date/times are taken from the reference and average date/times of the input
   buffer; The delta time is set to the time scan difference from the input
   buffer; Flag missing is cleared; and flag zero is set to the value of the
   current zero flag of the input buffer. For the extrapolation case, three new 
   periods are developed as follows:  The first periods begin date/time is 
   derived from the reference scan date/time of the input buffer. The period's 
   end time is set as its begin time plus one half the maximum time for 
   interpolation; and the period's delta time is set as one half the maximum 
   time for interpolation.  The third period's end time is set as the average 
   scan date/time from the input buffer;  The begin time for the period is set 
   as its ending time less one half he maximum value for interpolation; and
   the delta time is set as one half the maximum time for interpolation.  The
   second period's begin date/time is set as the end date/time of the first
   period; The end date/time is set as the begin date/time of the third period;
   The delta time is set as the difference between end and begin time; and 
   both flags missing and zero are set. 
 
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
   01/05/05      0013      Cham Pham            ccr NA05-01303
*****************************************************************************/
/* Global include files */
#include <prcprtac_main.h>
#include <a313h.h>
#include <a313hbuf.h>

/* Local include file */
#include "prcprtac_Constants.h"

void fill_new_hdrs( )
{
  if ( DEBUG ) 
  {
    fprintf(stderr,"A31357__FILL_NEW_HDRS\n");
    fprintf(stderr,"==> CASE: %d\n",blka.cases);
  }

/* Determine which case this is*/
/* Interpolation case*/
  if ( blka.cases == INTERP ) 
  {
    PerdHdr[n1].p_beg_date = RateSupl.ref_scndat;
    PerdHdr[n1].p_beg_time = RateSupl.ref_scntim;
    PerdHdr[n1].p_end_date = EpreSupl.avgdate;
    PerdHdr[n1].p_end_time = EpreSupl.avgtime;

  /* Get time difference between this and last good time*/
    PerdHdr[n1].p_delt_time = RateSupl.tim_scndif;
    PerdHdr[n1].p_flag_zero = RateSupl.flg_zerate;
    PerdHdr[n1].p_flag_miss = FLAG_CLEAR;

  /* Set spot blank status*/
    PerdHdr[n1].p_spot_blank = RateSupl.scan_sb_stat;

    if (DEBUG) 
    {
    fprintf(stderr," .......... INTERP ...... CURRENT_INDEX: %d\n",
                                       blka.current_index);
    fprintf(stderr,"PerdHdr[%d].p_beg_date = %d\n",n1,PerdHdr[n1].p_beg_date);
    fprintf(stderr,"PerdHdr[%d].p_beg_time = %d\n",n1,PerdHdr[n1].p_beg_time);
    fprintf(stderr,"PerdHdr[%d].p_end_date = %d\n",n1,PerdHdr[n1].p_end_date);
    fprintf(stderr,"PerdHdr[%d].p_end_time = %d\n",n1,PerdHdr[n1].p_end_time);
    fprintf(stderr,"PerdHdr[%d].p_delt_time= %d\n",n1,PerdHdr[n1].p_delt_time);
    fprintf(stderr,"PerdHdr[%d].p_flag_zero= %d\n",n1,PerdHdr[n1].p_flag_zero);
    fprintf(stderr,"PerdHdr[%d].p_flag_miss= %d\n",n1,PerdHdr[n1].p_flag_miss);
   fprintf(stderr,"PerdHdr[%d].p_spot_blank= %d\n",n1,PerdHdr[n1].p_spot_blank);
    }
  }
/* Extrapolation case*/
  else 
  {
    PerdHdr[n1].p_beg_date = RateSupl.ref_scndat;
    PerdHdr[n1].p_beg_time = RateSupl.ref_scntim;
    PerdHdr[n1].p_end_date = RateSupl.ref_scndat;
    PerdHdr[n1].p_end_time = PerdHdr[n1].p_beg_time+
                             blka.max_interp_tim/AVG_FACTOR;

   /* If day change set end time to zero and INCRement day*/
    if ( PerdHdr[n1].p_end_time >= SEC_P_DAY ) 
    {
      PerdHdr[n1].p_end_time = PerdHdr[n1].p_end_time - SEC_P_DAY; 
      PerdHdr[n1].p_end_date = PerdHdr[n1].p_end_date + INCR; 
    }
    PerdHdr[n1].p_delt_time = blka.max_interp_tim/AVG_FACTOR;

   /* Check for zero flags*/
    if ( (RateSupl.flg_zerref==FLAG_SET) || (RateSupl.flg_zerate==FLAG_SET) )
    {
     /* Set flag for zero period accumulation*/
      PerdHdr[n1].p_flag_zero = FLAG_SET;
    }
    else 
    {
     /* Clear zero flag*/
      PerdHdr[n1].p_flag_zero = FLAG_CLEAR;
    } 

    PerdHdr[n1].p_flag_miss = FLAG_CLEAR;

   /* Set spot blank status*/
    PerdHdr[n1].p_spot_blank = PerdHdr[blka.current_index].p_spot_blank;

    if (DEBUG) 
    {
    fprintf(stderr," .......... EXTRAP ...... CURRENT_INDEX: %d\n",
                                       blka.current_index);
    fprintf(stderr,"PerdHdr[%d].p_beg_date = %d\n",n1,PerdHdr[n1].p_beg_date);
    fprintf(stderr,"PerdHdr[%d].p_beg_time = %d\n",n1,PerdHdr[n1].p_beg_time);
    fprintf(stderr,"PerdHdr[%d].p_end_date = %d\n",n1,PerdHdr[n1].p_end_date);
    fprintf(stderr,"PerdHdr[%d].p_end_time = %d\n",n1,PerdHdr[n1].p_end_time);
    fprintf(stderr,"PerdHdr[%d].p_delt_time= %d\n",n1,PerdHdr[n1].p_delt_time);
    fprintf(stderr,"PerdHdr[%d].p_flag_zero= %d\n",n1,PerdHdr[n1].p_flag_zero);
    fprintf(stderr,"PerdHdr[%d].p_flag_miss= %d\n",n1,PerdHdr[n1].p_flag_miss);
   fprintf(stderr,"PerdHdr[%d].p_spot_blank= %d\n",n1,PerdHdr[n1].p_spot_blank);
    }

   /* Fill fields for third period n3*/
    PerdHdr[n3].p_end_date = EpreSupl.avgdate;
    PerdHdr[n3].p_end_time = EpreSupl.avgtime;
    PerdHdr[n3].p_beg_date = EpreSupl.avgdate;
    PerdHdr[n3].p_beg_time = PerdHdr[n3].p_end_time -
                             blka.max_interp_tim / AVG_FACTOR;

   /* If day turned back then set begin time back to previous hour*/
    if ( PerdHdr[n3].p_beg_time < INIT_VALUE ) 
    {
      PerdHdr[n3].p_beg_time = PerdHdr[n3].p_beg_time + SEC_P_DAY;

     /* Set day back by one*/
      PerdHdr[n3].p_beg_date = PerdHdr[n3].p_beg_date + DECR;
    }                 
    PerdHdr[n3].p_delt_time = blka.max_interp_tim / AVG_FACTOR;
    PerdHdr[n3].p_flag_zero = RateSupl.flg_zerate;
    PerdHdr[n3].p_flag_miss = FLAG_CLEAR;

   /* Set spot blank status*/
    PerdHdr[n3].p_spot_blank = RateSupl.scan_sb_stat;

    if (DEBUG) 
    {
    fprintf(stderr,"PerdHdr[%d].p_beg_date = %d\n",n3,PerdHdr[n3].p_beg_date);
    fprintf(stderr,"PerdHdr[%d].p_beg_time = %d\n",n3,PerdHdr[n3].p_beg_time);
    fprintf(stderr,"PerdHdr[%d].p_end_date = %d\n",n3,PerdHdr[n3].p_end_date);
    fprintf(stderr,"PerdHdr[%d].p_end_time = %d\n",n3,PerdHdr[n3].p_end_time);
    fprintf(stderr,"PerdHdr[%d].p_delt_time= %d\n",n3,PerdHdr[n3].p_delt_time);
    fprintf(stderr,"PerdHdr[%d].p_flag_zero= %d\n",n3,PerdHdr[n3].p_flag_zero);
    fprintf(stderr,"PerdHdr[%d].p_flag_miss= %d\n",n3,PerdHdr[n3].p_flag_miss);
   fprintf(stderr,"PerdHdr[%d].p_spot_blank= %d\n",n3,PerdHdr[n3].p_spot_blank);
    }
   /* Fill header for n2 [missing period].*/
    PerdHdr[n2].p_beg_date = PerdHdr[n1].p_end_date;
    PerdHdr[n2].p_end_date = PerdHdr[n3].p_beg_date;
    PerdHdr[n2].p_beg_time = PerdHdr[n1].p_end_time;
    PerdHdr[n2].p_end_time = PerdHdr[n3].p_beg_time;
    PerdHdr[n2].p_delt_time = PerdHdr[n3].p_beg_time-PerdHdr[n1].p_end_time;

   /* If day turned over set time to positive*/
    if ( PerdHdr[n2].p_delt_time < INIT_VALUE ) 
    {
      PerdHdr[n2].p_delt_time = PerdHdr[n2].p_delt_time + SEC_P_DAY;
    }
     
    PerdHdr[n2].p_flag_zero = FLAG_SET;
    PerdHdr[n2].p_flag_miss = FLAG_SET;
    
   /* Set spot blank status*/
    PerdHdr[n2].p_spot_blank = FLAG_CLEAR;

    if (DEBUG) 
    {
    fprintf(stderr,"PerdHdr[%d].p_beg_date = %d\n",n2,PerdHdr[n2].p_beg_date);
    fprintf(stderr,"PerdHdr[%d].p_beg_time = %d\n",n2,PerdHdr[n2].p_beg_time);
    fprintf(stderr,"PerdHdr[%d].p_end_date = %d\n",n2,PerdHdr[n2].p_end_date);
    fprintf(stderr,"PerdHdr[%d].p_end_time = %d\n",n2,PerdHdr[n2].p_end_time);
    fprintf(stderr,"PerdHdr[%d].p_delt_time= %d\n",n2,PerdHdr[n2].p_delt_time);
    fprintf(stderr,"PerdHdr[%d].p_flag_zero= %d\n",n2,PerdHdr[n2].p_flag_zero);
    fprintf(stderr,"PerdHdr[%d].p_flag_miss= %d\n",n2,PerdHdr[n2].p_flag_miss);
   fprintf(stderr,"PerdHdr[%d].p_spot_blank= %d\n",n2,PerdHdr[n2].p_spot_blank);
    }
  }
}
