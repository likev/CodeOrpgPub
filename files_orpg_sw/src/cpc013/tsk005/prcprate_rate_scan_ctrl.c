/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:43:48 $
 * $Id: prcprate_rate_scan_ctrl.c,v 1.2 2008/01/04 20:43:48 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprate_rate_scan_ctrl.c

   Description
   ===========
     The purpose of this function is to provide high level control for the 
     execution of the rate algorithm.  This function performs the following 
     functions: 
       1) calls function to initialize adaptation data.
       2) initializes 'flag zero' from input buffer.
       3) initializes current scan time to average scan time of hybrid scan.
       4) calls function to read reference header from disk.
       5) stores supplemental data fields in output buffer.
       6) initializes 'flag bad' for quality control test.
       7) stores bad scan information into output buffer.
   and 8) map rate scan bins onto 1/4 lfm grid.

   Change History
   =============
   08/29/88      0000      Greg Umstead         SPR # 80390
   04/10/90      0001      Dave Hozlock         SPR # 90697
   06/21/90      0001      Paul Pisani          SPR # 90775
   02/22/91      0002      Bayard Johnston      SPR # 91254
   02/15/91      0002      John Dephilip        SPR # 91762
   02/20/91      0002      Paul Jendrowski      SPR # 91685
   02/21/91      0002      John Dephilip        SPR # 91782
   03/12/91      0002      Paul Jendrowski      SPR # 91857
   12/03/91      0003      Steve Anderson       SPR # 92740
   12/10/91      0004      Ed Nichlas           SPR 92637 PDL Removal
   04/24/92      0005      Toolset              SPR 91895
   03/25/93      0006      Toolset              SPR NA93-06801
   01/28/94      0007      Toolset              SPR NA94-01101
   03/03/94      0008      Toolset              SPR NA94-05501
   04/01/95      0009      Toolset              CCR NA95-11802
   06/02/95      0010      R. Rierson           CCR NA94-35301
   11/12/95      0011      Dennis Miller        CCR NA94-08459
   12/23/96      0012      Toolset              CCR NA95-11807
   03/16/99      0013      Toolset              CCR NA98-23803
   01/13/05      0014      D. Miller; J. Liu    CCR NA04-33201
   02/20/05      0015      Cham Pham            CCR NA05-01303
*****************************************************************************/
/* Global include files */
#include <prcprtac_main.h>
#include <a313hbuf.h>
#include <a313h.h>
/* Local include files */
#include "prcprtac_file_io.h"
#include "prcprate_rate_scan_ctrl.h"
#include "prcprtac_Constants.h"

int rate_scan_ctrl( )
{
 int iostatus, offset;

/* Debug writes...*/
  if ( DEBUG ) 
    {fprintf(stderr," ***** BEGIN MODULE A3134R__RATE_SCAN_CTRL\n");}

/* Initialize reflectivity-index-to-precip-rate conversion table...*/

  init_precip_table( );

  if (DEBUG) 
    {fprintf(stderr," 4R: CALLED A3134I\n");}

/* Initialize adaptation data...*/

  init_rate_adapt( );

  if (DEBUG) 
    {fprintf(stderr,"4R: CALLED A3134D\n");}

/* Initialize flag_zero to flag (zero rate) from input buffer*/

  a313hgen.flag_zero = EpreSupl.zerohybrd;
  RateSupl.flg_zerate = a313hgen.flag_zero;

/* Initialize current scan time to average scan time of hybrid scan*/

  a313hgen.scan_time = EpreSupl.avgtime;
  a313hgen.scan_date = EpreSupl.avgdate;

  if (DEBUG) 
  {
   fprintf(stderr,"++ SCAN_TIME=%d  SCAN_DATE=%d\n",
                  a313hgen.scan_time,a313hgen.scan_date);
  }

/* Read reference header data from disk...*/

  read_header_recd( &iostatus );

  if (DEBUG) 
    {fprintf(stderr,"4R: CALLED A3134H\n");}

/* Check i/o status from read...*/
  if ( iostatus == IO_OK ) 
  {
/* Compute time difference from current and reference scan times,
   accounting for possible date difference...
 */
    a313hgen.time_dif = (a313hgen.scan_date-a313hgen.ref_sc_date)*SEC_P_DAY- 
                         a313hgen.ref_sc_time + a313hgen.scan_time;

    if (DEBUG) 
      {fprintf(stderr,"  TIME_DIF = %d\n",a313hgen.time_dif);}

/* Check for negative time difference due to archive playback*/
    if ( a313hgen.time_dif < 0 ) 
    {

/* Reinitialize the disk file database*/
      if (DEBUG) 
      {
        fprintf(stderr,"Illogical time difference = %d\n",a313hgen.time_dif);
        fprintf(stderr,"Remove lb_name: %s (fd %d)\n",LB_name,fdlb);
      }

      LB_close( fdlb );
      iostatus = LB_remove( LB_name );

      if ( iostatus == IO_OK ) 
      {
        if (DEBUG) 
         {fprintf(stderr,"a3134r: remove dkfile (status %d) \n",iostatus);}

        initialize_file_io( &iostatus );
      }

/* Set time difference to a flag value*/
      a313hgen.time_dif = FLAG_TIME_DIF;
    }

/* Store supplemental data fields in output buffer...*/
    RateSupl.tim_scndif = a313hgen.time_dif;
    RateSupl.flg_zerref = a313hgen.ref_flag_zero;
    RateSupl.ref_scndat = a313hgen.ref_sc_date;
    RateSupl.ref_scntim = a313hgen.ref_sc_time;
    RateSupl.scan_sb_stat = EpreSupl.vol_sb;

    if (DEBUG) 
    {
      fprintf(stderr,
            "134R: hydrsupl(vol_stat): %d suplout(scan_sb_stat):%d\n",
             EpreSupl.vol_sb, RateSupl.scan_sb_stat);
    }

/* Debug writes...*/
    if (DEBUG)
      {fprintf(stderr,"A3134R:  SCAN TIME DIF - %d\n",a313hgen.time_dif);}

/* Otherwise, construct rate scan array and rate and area arrays...*/
    if ( a313hgen.flag_zero == FLAG_CLEAR ) 
    {
      avg_hybscn_pairs( );
    }

/* Initialize flag_bad for quality control tests.*/
    a313hgen.flag_bad = FLAG_CLEAR;

/* Debug writes...*/
    if (DEBUG) 
    {
      fprintf(stderr,"A3134R:  REF SCAN GOOD FLAG - %d IOSTATUS = %d\n",
                   a313hgen.ref_sc_good,iostatus);
    }
/* If reference scan is known to be good (i.e. current scan is third
   scan since startup)...
 */
    if ( (a313hgen.ref_sc_good == FLAG_SET) && (iostatus == IO_OK) ) 
    {

/* Check for zero time difference due to archive playback */
      if ( a313hgen.time_dif <= 0 ) 
      {
        if (DEBUG) 
          {fprintf(stderr,"a3134r: time dif<=0, setting flag bad\n");}

        a313hgen.flag_bad = FLAG_SET;

/* Perform bad scan array management, since flag_bad and nbr^?_badscns
   are now both known...
 */
        update_bad_scans( &iostatus );
      }

    }
    else 
    {

      if ( DEBUG ) 
        {fprintf(stderr,"+++++ REF_SC_DATE= %d\n",a313hgen.ref_sc_date);}

/*** If reference date is not dummy value, set good ref. scan flag...*/

      if ( a313hgen.ref_sc_date != INIT_VALUE ) 
      {
        a313hgen.ref_sc_good = FLAG_SET;
      }

/*** Debug writes...*/
      if ( DEBUG ) 
      {
        fprintf(stderr,"+++++ REF_SC_GOOD= %d\n",a313hgen.ref_sc_good);
        fprintf(stderr,
              "A3134R:  REF SCAN BAD, REF_SC_DATE==%d  REF_SC_GOOD=%d\n",
               a313hgen.ref_sc_date, a313hgen.ref_sc_good);
      }
    }

/** Check i/o status again, in case update_bad_scans was called...*/
    if ( iostatus == IO_OK ) 
    {

/** Store bad scan info. in output buffer supplemental data...*/
      RateSupl.flg_badscn = a313hgen.flag_bad;
      RateSupl.cnt_badscn = a313hgen.nbr_badscns;
      RateSupl.bad_scnptr = a313hgen.ptr_badscn;
      RateSupl.ref_scngod = a313hgen.ref_sc_good;

      if ( DEBUG ) 
      {
        fprintf(stderr,"*******************\n");
        fprintf(stderr,"RateSupl.flg_badscn= %d RateSupl.cnt_badscn= %d\n",
                        RateSupl.flg_badscn,RateSupl.cnt_badscn);
        fprintf(stderr,"RateSupl.bad_scnptr= %d RateSupl.ref_scngod= %d\n",
                        RateSupl.bad_scnptr,RateSupl.ref_scngod);
      }

/** If current scan not flagged as bad...*/
      if ( a313hgen.flag_bad == FLAG_CLEAR ) 
      {

/** If precipitation exists, perform range-effect corrections on
    rate scan */
        if ( a313hgen.flag_zero == FLAG_CLEAR ) 
        {
          range_effect_correc( );
        }
      }
/** Now map rate scan bins onto 1/4 lfm grid.*/
      lfm4_map( );

    }/*end if(iostatus.eq.IO_OK)...update_bad_scan() call */

/* Copy Rate supplemental data to the output buffer */
   offset = SSIZ_PRE;
   memcpy(&prcprtacbuf.HydrSupl[offset],&RateSupl,sizeof(PRCPRTAC_rate_supl_t));

   if ( DEBUG ) 
   {
     fprintf(stderr,"RateSupl.flg_zerate=%d\n",RateSupl.flg_zerate);
     fprintf(stderr,"RateSupl.flg_badscn=%d\n",RateSupl.flg_badscn);
     fprintf(stderr,"RateSupl.cnt_badscn=%d\n",RateSupl.cnt_badscn);
     fprintf(stderr,"RateSupl.flg_zerref=%d\n",RateSupl.flg_zerref);
     fprintf(stderr,"RateSupl.ref_scndat=%d\n",RateSupl.ref_scndat);
     fprintf(stderr,"RateSupl.ref_scntim=%d\n",RateSupl.ref_scntim);
     fprintf(stderr,"RateSupl.tim_scndif=%d\n",RateSupl.tim_scndif);
     fprintf(stderr,"RateSupl.bad_scnptr=%d\n",RateSupl.bad_scnptr);
     fprintf(stderr,"RateSupl.ref_scngod=%d\n",RateSupl.ref_scngod);
     fprintf(stderr,"RateSupl.scan_sb_stat=%d\n",RateSupl.scan_sb_stat);
   }
  }/*end if(iostatus.eq.IO_OK)...read_header_recd() call */

/* Debug writes...*/
  if ( DEBUG ) 
    {fprintf(stderr,"***** END OF MODULE A3134R__RATE_SCAN_CTRL\n");}

  return iostatus;
}
