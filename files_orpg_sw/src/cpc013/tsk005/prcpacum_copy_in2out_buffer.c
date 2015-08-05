/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 18:19:46 $
 * $Id: prcpacum_copy_in2out_buffer.c,v 1.2 2006/02/09 18:19:46 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_copy_in2out_buffer.c 
  
   Description
   ===========
      This function copies the contents of the input from Rate algorithm to the
   output buffer.  Data copied includes: precipitation status message;
   adaptation data; supplemental data; and lfm 4 x 4 data.

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
   10/26/05      0012      Cham Pham            ccr NA05-21401

   INPUT           TYPE        		DESCRIPTION
   -----           ----        		----------- 
   prcprtacbuf     PRCPRTAC_smlbuf_t    Contains informations which copied
                                        to output buffer structure.

   OUTPUT          TYPE                 DESCRIPTION
   ------          ----                 -----------
   prcprtac_buf    PRCPRTAC_smlbuf_t    Contains updated information which is 
                                        written to the output buffer.  
****************************************************************************/
/* Global include files */
#include <prcprtac_main.h>         
#include <a313hbuf.h>
#include <a313h.h>

/* Local include file */
#include "prcprtac_Constants.h"

void copy_in2out_buffer( PRCPRTAC_smlbuf_t *prcprtac_buf )
{
int lfm4sz; 

 if ( DEBUG ) 
   {fprintf(stderr,"A31353_COPY_INPUT_BUFFER\n");}

/* Move precipitation status message from input to output buffer.*/
 memcpy(prcprtac_buf->HydrMesg,&prcprtacbuf.HydrMesg,C_HYZMESG*sizeof(int));

/* Move adaptation data from input to output buffer.*/
 memcpy(prcprtac_buf->HydrAdapt,&prcprtacbuf.HydrAdapt,C_HYZADPT*sizeof(double));

/* Move supplementary data from input to output buffer.*/
 memcpy(prcprtac_buf->HydrSupl,&prcprtacbuf.HydrSupl,C_HYZSUPL*sizeof(int));

/* Move lfm4 data from input to output buffer.*/
 lfm4sz = HYZ_LFM4 * HYZ_LFM4;
 memcpy(prcprtac_buf->LFM4grd,lfm4Grd,sizeof(short)*lfm4sz);

}
