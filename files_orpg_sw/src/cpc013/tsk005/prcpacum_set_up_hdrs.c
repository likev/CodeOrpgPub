/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:24 $
 * $Id: prcpacum_set_up_hdrs.c,v 1.1 2005/03/09 15:43:24 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_set_up_hdrs.c 

   Description
   ===========
      This function sets up all the conditions necessary for computing period
   and hourly totals. functions performed include:
      1) Determining the cases, extrapolation or interpolation, based on the
         time scan difference between current and previous rate scans.
      2) Reading the period and hour headers from disk.
      3) Appending new headers to the period header (14-16).
      4) Defines the current hour. 
      5) Normalizes the header times to a twenty four hour plus orientation.
      6) Determines whether there is sufficient time in the hour to compute
         hourly totals.

   Change History
   ==============
   02/21/89      0000      P. Pisani            spr # 90067
   04/23/90      0001      David M. Lynch       spr # 90697
   02/22/91      0002      Bayard Johnston      spr # 91254
   02/15/91      0002      John Dephilip        spr # 91762
   12/03/91      0003      Steve Anderson       spr # 92740
   12/10/91      0004      Ed Nichlas           spr 92637 pdl removal
   04/24/92      0005      Toolset              spr 91895
   03/25/93      0006      Toolset              spr na93-06801
   01/28/94      0007      Toolset              spr na94-01101
   03/03/94      0008      Toolset              spr na94-05501
   04/11/96      0009      Toolset              ccr na95-11802
   12/23/96      0010      Toolset              ccr na95-11807
   03/16/99      0011      Toolset              ccr na98-23803
   06/30/03      0012      D. Miller            ccr na02-06508
   01/05/05      0013      Cham Pham            ccr NA05-01303
****************************************************************************/
/* Global include files */
#include <a313hbuf.h>
#include <a313h.h>

/* Local include file */
#include "prcprtac_Constants.h"

/* Declare function prototypes */
extern void read_header_flds( int* );
extern void fill_new_hdrs( void );
extern void define_hour( void );
extern void normalize_times( void );
extern void time_in_hour( void );

void set_up_hdrs( int *iostat )
{
  if ( DEBUG ) {fprintf(stderr," A3135S__SET_UP_HDRS\n");}

/* Clear i/o status flag and determine cases. if time difference
   between previous and current scans is greater than maximum for
   interpolation then set cases to 'extrapolate' otherwise 'interpolate'
 */
  *iostat = FLAG_CLEAR;

  if ( RateSupl.tim_scndif > blka.max_interp_tim ) 
  {
    blka.cases = EXTRAP;
  }
  else
  {
    blka.cases = INTERP;
  }

  if (DEBUG)
   {fprintf(stderr,"(EXTRAP  1 -- INTERP  0) blka.cases: %d\n",blka.cases);}
      
/* Set current 'cases' into current hour header.*/
  HourHdr[curr_hour].h_flag_case = blka.cases;

/* Read period and hourly header fields from disk.*/
  read_header_flds( iostat );

/* If i/o status is good proceed; otherwise return with error.*/
  if ( *iostat == FLAG_CLEAR ) 
  {
/* Add one or three new periods depending on whether the cases is
   extrapolation or interpolation.
 */
    fill_new_hdrs( );

/* Set up the end and beginning of hour either to current time,
   previous clock hour, or end gage time. 
 */
    define_hour( );

/* Adjust times used if there has been a date change. times after
   midnight are incremented by seconds per day.
 */
    normalize_times( );

/* Determine if there is enough time in the hour to calculate hourly totals.*/
    time_in_hour( );
  }

}
