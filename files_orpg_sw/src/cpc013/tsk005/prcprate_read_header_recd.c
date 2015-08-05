/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:44:06 $
 * $Id: prcprate_read_header_recd.c,v 1.1 2005/03/09 15:44:06 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprate_read_header_recd.c

   Description
   ===========
      This function reads the rate scan header record from disk, returning
   the status code from the read operation.

   Change History
   =============
   08/29/88      0000      greg umstead         spr # 80390
   02/22/91      0001      bayard johnston      spr # 91254
   02/15/91      0001      john dephilip        spr # 91762
   12/03/91      0002      steve anderson       spr # 92740
   12/10/91      0003      ed nichlas           spr 92637 pdl removal
   04/24/92      0004      toolset              spr 91895
   03/25/93      0005      toolset              spr na93-06801
   01/28/94      0006      toolset              spr na94-01101
   03/03/94      0007      toolset              spr na94-05501
   04/11/96      0008      toolset              ccr na95-11802
   12/23/96      0009      toolset              ccr na95-11807
   03/16/99      0010      toolset              ccr na98-23803
   01/13/05      0011      D. Miller; J. Liu    CCR NA04-33201 
   02/20/05      0012      Cham Pham            ccr NA05-01303
****************************************************************************/
/* Global include files */
#include <prcprtac_file_io.h>
#include <a313hbuf.h>
#include <a313h.h>

/* Local include file */
#include "prcprtac_Constants.h"

void read_header_recd( int *iostatus )
{
 int readrec = 0;

  if ( DEBUG ) {fprintf(stderr,"BEGIN MODULE A3134H__READ_HEADER_RECD...\n");}

/* Call Header_IO() to read header info...*/

  *iostatus = Header_IO( readrec, rathdr );

/* Now copy information to actual fields...*/

  if ( *iostatus == IO_OK ) 
  {

/** First the flags...*/
    a313hgen.ref_flag_zero = RateHdr.hdr_rflagz;
    a313hgen.ref_sc_good   = RateHdr.hdr_rfscgd;

/** Next, the date/time and bad scan related fields...*/
    a313hgen.ref_sc_date = RateHdr.hdr_rfscdt;
    a313hgen.ref_sc_time = RateHdr.hdr_rfsctm;
    a313hgen.ptr_badscn  = RateHdr.hdr_ptbdsc;
    a313hgen.nbr_badscns = RateHdr.hdr_nobdsc;

/** More debug writes...*/
    if ( DEBUG ) 
    {
      fprintf(stderr,"A3134H:  AFTER READING HEADER...\n");
      fprintf(stderr,"ref_sc_date=%d\tref_sc_time=%d\n",
                      a313hgen.ref_sc_date,a313hgen.ref_sc_time);
      fprintf(stderr,"ptr_badscn=%d\tnbr_badscns=%d\n",
                      a313hgen.ptr_badscn,a313hgen.nbr_badscns);
      fprintf(stderr,"END OF MODULE A3134H__READ_HEADER_RECD...\n");
    }

  }/* End if iostatus equals to IO_OK */
}
