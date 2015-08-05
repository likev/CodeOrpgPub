/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:02 $
 * $Id: prcpacum_fill_supl_array.c,v 1.1 2005/03/09 15:43:02 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_fill_supl_array.c

   Description
   ===========
       This function fills the supplementary data array portion of the output
   buffer with various flags and data. If the case is for interpolation the
   missing date and time data is set to zero. If the case is for extrapolation
   the missing date and time data is set from the newly added missing period
   (n2) from the period header array. The begining and ending date/times of
   the supplementary data array portion of the output buffer are set from
   the hour header beginning and ending date/times.
    
   Change History
   ==============
   02 21 89      0000      P. Pisani            spr # 90067
   02 22 91      0001      Paul Jendrowski      spr # 91254
   02 13 91      0001      Bayard Johnston      spr # 91254
   02 15 91      0001      John Dephilip        spr # 91762
   12 03 91      0002      Steve Anderson       spr # 92740
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
#include <a313h.h>
#include <a313hbuf.h>

/* Local include file */
#include "prcprtac_Constants.h"

/* Declare function prototypes */
void restore_times( void );
void normalize_times( void );


void fill_supl_array( )
{

/* Debug writes ... */
  if ( DEBUG ) 
  {    
    fprintf(stderr,"  A3135Q__FILL_SUPL_ARRAY\n");
    fprintf(stderr,"3BLKA.CURRENT_INDEX: %d\n",blka.current_index);
  }

/* Fill fields in supplemental data portion of output buffer which could not be
   filled earlier, due to delayed allocation of output buffer. If case is
   interpolation set missing values in supplemental data array to null. If
   extrapolation, set missing values in supplemental data array to those 
   contained in the period 2 header.
*/
  restore_times( );
  AcumSupl.flg_zerscn = blka.scn_flag;

/* Interpolation case */
  if ( blka.cases == INTERP ) 
  {
    AcumSupl.flg_msgprd = FLAG_CLEAR;

   /* No missing period, set dates and times to zero */
    AcumSupl.beg_misdat = INIT_VALUE;
    AcumSupl.beg_mistim = INIT_VALUE;
    AcumSupl.end_misdat = INIT_VALUE;
    AcumSupl.end_mistim = INIT_VALUE;
  }
/* Extrapolation case */
  else 
  {
    AcumSupl.flg_msgprd = FLAG_SET;

   /* There is a missing period, fill in dates and times*/
    AcumSupl.beg_misdat = PerdHdr[n2].p_beg_date;
    AcumSupl.beg_mistim = PerdHdr[n2].p_beg_time;
    AcumSupl.end_misdat = PerdHdr[n2].p_end_date;
    AcumSupl.end_mistim = PerdHdr[n2].p_end_time;
  }

/* Set hourly values from hourly header into supplemental data array. */
  AcumSupl.flg_zerhly = HourHdr[curr_hour].h_flag_zero;
  AcumSupl.flg_nohrly = HourHdr[curr_hour].h_flag_nhrly;

/* Fill in hour begin and end time*/
  AcumSupl.beg_hrdate = HourHdr[curr_hour].h_beg_date;
  AcumSupl.beg_hrtime = HourHdr[curr_hour].h_beg_time;
  AcumSupl.end_hrdate = HourHdr[curr_hour].h_end_date;
  AcumSupl.end_hrtime = HourHdr[curr_hour].h_end_time;
  AcumSupl.hly_scntyp = HourHdr[curr_hour].h_scan_type;
  AcumSupl.max_hlyacu  = INIT_VALUE;
  
  if ( DEBUG ) 
  {
    fprintf(stderr,"==== A3135Q ACUM SUPL ====================\n");
    fprintf(stderr,"AcumSupl.flg_zerscn: %d\n",AcumSupl.flg_zerscn);
    fprintf(stderr,"AcumSupl.flg_msgprd: %d\n",AcumSupl.flg_msgprd);
    fprintf(stderr,"AcumSupl.beg_misdat: %d\n",AcumSupl.beg_misdat);
    fprintf(stderr,"AcumSupl.beg_mistim: %d\n",AcumSupl.beg_mistim);
    fprintf(stderr,"AcumSupl.end_misdat: %d\n",AcumSupl.end_misdat);
    fprintf(stderr,"AcumSupl.end_mistim: %d\n",AcumSupl.end_mistim);
    fprintf(stderr,"AcumSupl.flg_zerhly: %d\n",AcumSupl.flg_zerhly);
    fprintf(stderr,"AcumSupl.flg_nohrly: %d\n",AcumSupl.flg_nohrly);
    fprintf(stderr,"AcumSupl.beg_hrdate: %d\n",AcumSupl.beg_hrdate);
    fprintf(stderr,"AcumSupl.beg_hrtime: %d\n",AcumSupl.beg_hrtime);
    fprintf(stderr,"AcumSupl.hly_scntyp: %d\n",AcumSupl.hly_scntyp);
    fprintf(stderr,"AcumSupl.num_intout: %d\n",AcumSupl.num_intout);
    fprintf(stderr,"AcumSupl.max_hlyacu: %d\n",AcumSupl.max_hlyacu);
    fprintf(stderr,"AcumSupl.flg_spot_blank: %d\n",AcumSupl.flg_spot_blank);
  }  

/* Normalize the times*/
  normalize_times( );

}
