/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:29:36 $
 * $Id: prcpacum_buffer_needs.c,v 1.1 2005/03/09 15:29:36 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_buffer_needs.c 

   Description
   ===========
       This function determines the size of the output buffer needed. The output
   buffer size is set as small, medium, or large.  The small size contains room
   for only the basic fields which include: precipitation status message, 
   adaptation  data array, and lfm 4 x 4 data.  The medium size buffer, in
   addition to the small size, contains space for a 115 x 360 i*2 scan to scan
   array for period accumulations.  The large size buffer, in addition to the
   medium size, contains space for a 115 x 360 i*2 hourly scan buffer.  The
   implications are that the small size is needed for those cases where neither
   period nor hourly accumulations are to be computed; The medium size is 
   needed only for period accumulations; And the large size is needed for the
   case where where both period and hourly accumulations are to be computed.

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
   04/11/96      0008      Toolset              ccr na95-11802
   12/23/96      0009      Toolset              ccr na95-11807
   03/16/99      0010      Toolset              ccr na98-23803
   01/07/05      0011      Cham Pham            ccr NA05-01303
****************************************************************************/
/* Global include files */
#include <prcprtac_main.h>
#include <a313h.h>
#include <a313hbuf.h>

/* Local include files */
#include "prcprtac_Constants.h"

void buffer_needs( int *buff_size ) 
{     

  if ( DEBUG ) {fprintf(stderr,"A31355__BUFFER_NEEDS\n");}

/* Initalize buffer empty flag to set. It will be cleared if  any data
   is coppied into it. */
  blka.hbuf_empty = FLAG_SET;

/* Output buffer: scan to scan buffer array needs to be allocated
 * -------------  if current flag (zero rate) clear (even if previous
 * flag (zero rate) set, because we would still have accumulation in
 * the scan to scan period).  The hourly array likewise needs
 * to be allocated if current flag (zero rate) clear and will not
 * be allocated if current flag set unless the current hourly period
 * is a special case (i.e. set back), it was raining previously, and
 * the time (last precip detected) occurred after the start of the
 * new hour period. finally, though, if the hourly period has too
 * much missing time within it, the hourly portion of the output
 * buffer need not be allocated. Preset hourly and scan to scan flags and
 * and buffer size to small size if current flag (zero rate) is clear, then
 * set flag (zero hourly), clear scan to scan flag, and set buffer size to 
 * large size.
 */
  HourHdr[curr_hour].h_flag_zero = FLAG_SET;
  blka.scn_flag = FLAG_SET;
  *buff_size = SMLSIZ_ACUM;

  if ( RateSupl.flg_zerate == FLAG_CLEAR ) 
  {
    HourHdr[curr_hour].h_flag_zero = FLAG_CLEAR;
    blka.scn_flag = FLAG_CLEAR;
    *buff_size = LRGSIZ_ACUM;
  }
/* Check other obscure case: current flag (zero rate) not set but hour period
 * set back from current time and some rain occurred within the hour.
 */
  else if ( blka.time_last_prcp > HourHdr[curr_hour].h_beg_time ) 
  {
    HourHdr[curr_hour].h_flag_zero = FLAG_CLEAR;
    *buff_size = LRGSIZ_ACUM;
  }
  
  if ( (HourHdr[curr_hour].h_flag_zero == FLAG_CLEAR) &&
       (HourHdr[curr_hour].h_flag_nhrly == FLAG_SET) ) 
  {
    if ( blka.scn_flag == FLAG_SET ) 
    {
      *buff_size = SMLSIZ_ACUM;
    }
    else
    {
      *buff_size = MEDSIZ_ACUM;           
    }
  }
}
