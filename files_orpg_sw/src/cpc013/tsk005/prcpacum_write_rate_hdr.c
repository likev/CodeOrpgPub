/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:34 $
 * $Id: prcpacum_write_rate_hdr.c,v 1.1 2005/03/09 15:43:34 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_write_rate_hdr.c

   Description
   ===========
       Writes rate header record from information contained in the supplemental
   data array, depending on the contents of the flag indicating a bad scan.
   (also in the supplemental data array)

   Change History
   ==============
   09 26 88      0000      Greg Umstead         spr # 90067
   02 22 91      0001      Paul Jendrowski      spr # 91254
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
   01/13/05      0011      D. Miller; J. Liu    CCR NA04-33201
   02/02/05      0012      Cham Pham            ccr NA05-01303
****************************************************************************/
/* Global include files */
#include <prcprtac_main.h>
#include <a313hbuf.h>
#include <a313h.h>

/* Local include files */
#include "prcprtac_file_io.h"
#include "prcprtac_Constants.h"

void write_rate_hdr( int *iostatus )
{
 int writerec = 1;

  if ( DEBUG ) {fprintf(stderr,"'BEGIN MODULE A3135W__WRITE_RATE_HEADER\n");}

  if ( RateSupl.flg_badscn == FLAG_SET ) 
  {
/* If current scan is bad, write reference scan date/time and
   reference flag(zero)... 
 */
    RateHdr.hdr_rflagz = RateSupl.flg_zerref;
    RateHdr.hdr_rfscdt = RateSupl.ref_scndat;
    RateHdr.hdr_rfsctm = RateSupl.ref_scntim;
  }
  else 
  {
/* Otherwise (if current scan is good), use current scan date/time
   and current flag(zero)... 
 */
    RateHdr.hdr_rflagz = RateSupl.flg_zerate;
    RateHdr.hdr_rfscdt = EpreSupl.avgdate;
    RateHdr.hdr_rfsctm = EpreSupl.avgtime;
  }
  
/* Set current flag(ref. scan good), and bad scan ralated fields to values 
   from supplemental data array...
 */
  RateHdr.hdr_rfscgd = RateSupl.ref_scngod;
  RateHdr.hdr_ptbdsc = RateSupl.bad_scnptr;
  RateHdr.hdr_nobdsc = RateSupl.cnt_badscn;

/* Called shared i/o module to write rate header record...*/
  *iostatus = Header_IO ( writerec, rathdr ); 

}
