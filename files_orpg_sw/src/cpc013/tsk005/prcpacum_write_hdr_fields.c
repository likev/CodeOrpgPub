/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:32 $
 * $Id: prcpacum_write_hdr_fields.c,v 1.1 2005/03/09 15:43:32 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_write_hdr_fields.c
    Author: Kelley Miles
    Created: 13 DEC 2004

    Description
    ===========
    This function performs some basic cleanup activities prior to placing the
    task back into the trap wait state. The newly added period(s) are first
    integrated into the period header. Next, the period headers are written
    to disk, Finally, the hourly header is written to disk.

    Change History
    ============
    DATE          VERSION   PROGRAMMER           NOTES
    ----------    -------   ----------------     --------------------
    02/21/89      0000      P. PISANI            SPR # 90067
    04/23/90      0001      DAVID M. LYNCH       SPR # 90697
    02/22/91      0002      BAYARD JOHNSTON      SPR # 91254
    02/15/91      0002      JOHN DEPHILIP        SPR # 91762
    12/03/91      0003      STEVE ANDERSON       SPR # 92740
    12/10/91      0004      ED NICHLAS           SPR 92637 PDL Removal
    04/24/92      0005      Toolset              SPR 91895
    03/25/93      0006      Toolset              SPR NA93-06801
    01/28/94      0007      Toolset              SPR NA94-01101
    03/03/94      0008      Toolset              SPR NA94-05501
    04/11/96      0009      Toolset              CCR NA95-11802
    12/23/96      0010      Toolset              CCR NA95-11807
    03/16/99      0011      Toolset              CCR NA98-23803
    12/31/02      0012      D. Miller            CCR NA00-28601
    12/13/04      0013      K. Miles, C. Pham    CCR NA05-01303

Included variable descriptions
    CASES          Indicates method for period accumulations. 
                   CASES=0 indicates method is INTERPOLATION. 
                   CASES=1 indicates method is EXTRAPOLATION.
    CURRENT_INDEX  Current index into previous period headers maintained
                   on disk.
    HourHdr        Relevant information concerning hourly accumulations.
                   Indexed by one symbolic name listed under value and
                   current or previous hour.
    NEW_FRAC       Contains fraciton each period is in the hour, for all
                   previous period scans. Decimal real fraction multiplied
                   by 1000 and stored as an integer value.
    PerdHdr        Contains relevant information concerning each period.
    CURR_HOUR      Constant. Current hour offset into hour header.
    HLYHDR         Constant. Flag indicating I/O to be performed on hourly
                   header record.
    H_BEG_DATE     Constant. Offset into hour header for previous or current
                   hour pointing to the Julian date for the beginning of
                   the hour.
    H_CURR_PRD     Constant. Position of current period field in hourly
                   header array.
    INCR           Constant. A value used for incrementing by one.
    INTERP         Constant. Indicates that method for period accumulations
                   is INTERPOLATION.
    IO_OK          Constant. Parameter defining valid I/O return code.
    n1             Constant. Index for first new period out of a possible
                   three added.
    n3             Constant. Index for third new period out of a possible
                   three added.
    NUM_PREV_PRD   Constant. Number of pervious periods maintained on disk.
    prdhdr         Constant. Flag indicating I/O to be performed on period
                   header record.
    p_frac         Constant. Position of fraction of period within past hour
                   in period header array.
    write          Constant. Write operation parameter for SYSIO call.

Internal tables/work area
    acz_prd_flds   Number of fields for each period.
    fields         Counter to process the loop that sets newly added
                   periods into the period header.
    first          Value 0 used to initialize do loops that deal with the
                   period header.
    local_index    Value set to current index used to control searching
                   periods in the previous hour.
    prd_index      Period n1, n2, or n3 for which the end of the hour occurs.
****************************************************************************/
/** Global include file */
#include <a313h.h>

/** Local include files */
#include "prcprtac_file_io.h"
#include "prcprtac_Constants.h"

void write_hdr_fields( int *iostat )
{
   int local_index,
       prd_index;
   int first=1;
   int writerec=1;

   if ( DEBUG ) {fprintf(stderr," A3135M__WRITE_HDR_FLDS\n");}

/* Copy new fraction values to period header fraction fields */

   *iostat = IO_OK;

   for( prd_index=0; prd_index<=NUM_PREV_PRD; prd_index++ )
   {
      PerdHdr[prd_index].p_frac = blka.new_frac[prd_index];
   }

/* If case is interpolate, set up to move fields from one new header to
   the oldest slots in the previous period headers. If case is extrapolate,
   set up to move three new periods to oldest slots in previous period headers.
 */
   if ( blka.cases == INTERP )
   {
      local_index = n1;
   }
   else
   {
      local_index = n3;
   }

/* Move one or three new period header fields to oldest slot(s) in previous
   period scans 
 */
   for( prd_index=n1; prd_index<=local_index; prd_index++ )
   {
      blka.current_index += INCR;
      if ( blka.current_index > NUM_PREV_PRD )
      {
         blka.current_index = first;
      }
      memcpy( &PerdHdr[blka.current_index], &PerdHdr[prd_index], 
                                       sizeof(Period_Header_t) );
   }

/* Write previous period scan headers to disk */

   *iostat = Header_IO( writerec, prdhdr );

/* Set current index into hourly header for current hour which will become
   current index for next volume scan. 
 */
   HourHdr[curr_hour].h_curr_prd = blka.current_index;

/* If iostat is good from pervious write then write current hour header 
   to disk. 
 */
   if ( *iostat == IO_OK )
   {
      memcpy( &HourlyHdr, &HourHdr[curr_hour], sizeof(Hour_Header_t) );
      *iostat = Header_IO( writerec, hlyhdr );
   }

}
