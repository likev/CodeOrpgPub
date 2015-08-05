/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:43 $
 * $Id: prcprate_copy_input_buffer.c,v 1.1 2005/03/09 15:43:43 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprate_copy_input_buffer.c

   Description
   ===========
      This function copies status message, adaptation data, and supplemental 
   data arrays from input (epre task) to local structure. 

   Change History
   =============
   08/29/88      0000      Greg Umstead         spr# 80390
   04/05/90      0001      Dave Hozlock         spr# 90697
   11/03/90      0002      Paul Jendrowski      spr# 91254
   12/03/91      0003      Steve Anderson       spr # 92740
   12/10/91      0004      Ed Nichlas           spr 92637 pdl removal
   04/24/92      0005      toolset              spr 91895
   03/25/93      0006      toolset              spr na93-06801
   01/28/94      0007      toolset              spr na94-01101
   03/03/94      0008      toolset              spr na94-05501
   04/11/96      0009      toolset              ccr na95-11802
   12/23/96      0010      toolset              ccr na95-11807
   03/16/99      0011      toolset              ccr na98-23803
   01/01/05      0012      Cham Pham		ccr NA05-01303
****************************************************************************/
/* Global include files */
#include <prcprtac_main.h>
#include <a313hbuf.h>

/* Local include file */
#include "prcprtac_Constants.h"

void copy_input_buffer ( EPRE_buf_t *in_buf )  
{
int offset;

   if (DEBUG) {fprintf(stderr,"COPY_INPUT_BUFFER...\n");}

/* Do for the entire input buffer to local arrays ...*/
   memcpy( &(prcprtacbuf.HydrMesg), in_buf->HydroMesg,
                            C_HYZMESG*sizeof(int) );
   memcpy( &(prcprtacbuf.HydrAdapt), in_buf->HydroAdapt,
                            C_HYZADPT*sizeof(float) );
   memcpy( &(prcprtacbuf.HydrSupl), in_buf->HydroSupl,
                            C_HYZSUPL*sizeof(int) );
   memcpy( HybrScan, in_buf->HyScanZ, MAX_AZM*MAX_RNG*sizeof(short) );

/* Copy the supplemental buffer to EpreSupl struct */
   memcpy( &(EpreSupl), &(prcprtacbuf.HydrSupl), sizeof(EPRE_supl_t) );
         
/* Copy the input buffer to the local structure */

/* Enhanced Preprocessing parameters .............*/
   memcpy( &epre_adpt, &prcprtacbuf.HydrAdapt[0], sizeof(EpreAdapt_t) );

/* Rate parameters ...............................*/
   memcpy( &rate_adpt, &prcprtacbuf.HydrAdapt[ASIZ_PRE], sizeof(RateAdapt_t) );

/* Accumulation parameters .......................*/
   offset = ASIZ_PRE + ASIZ_RATE;
   memcpy( &acum_adpt, &prcprtacbuf.HydrAdapt[offset], sizeof(AcumAdapt_t) );

}
