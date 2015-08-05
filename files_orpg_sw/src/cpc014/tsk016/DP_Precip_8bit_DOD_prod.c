/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 22:51:24 $
 * $Id: DP_Precip_8bit_DOD_prod.c,v 1.4 2009/10/27 22:51:24 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/******************************************************************************
   Filename: DP_Precip_8bit_DOD_prod.c

   Description
   ===========
      This function generates Digital One Hour Difference Accumulation product
   (8-bit product) by using packet 16 for graphical product. The resolution is
   250 m by 1 deg (256 color levels).

   The DOD is approximately the DAA (170) - DPA (108).

   We don't use the DPA directly because the (legacy) DPA product waits
   52 minutes after the storm start to generate a product, so to get an instant
   DOD, we use the HYADJSCN buffer, which feeds the DPA.

   This product will generate every volume scan.

   Input:  LT_Accum_Buf_t* inbuf    - pointer to inbuf structure
           int             vol_num  - current volume scan number
           char*           prodname - product name

   Output:

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   -----       -------    ---------------    ----------------------
   10/07       0000       Pham,Ward,Stein    Initial Implementation
******************************************************************************/

#include "DP_Precip_8bit_func_prototypes.h"

void build_DOD_product(LT_Accum_Buf_t* inbuf, int vol_num, char* prodname)
{
   int*  p174_ptr  = NULL;
   int   opstat    = RPGC_NORMAL;
   int   Code_DOD  = 0;
   int   minval    = 0;
   int   maxval    = 0;
   float scale     = 1.0;
   float offset    = 0.0;
   int   ret       = FUNCTION_SUCCEEDED; /* return value */
   unsigned char data_scaled[MAX_AZM][MAX_BINS]; /* accum grid scaled to *
                                                    256 levels           */
   if ( DP_PRECIP_8BIT_DEBUG )
      fprintf(stderr,"\nBeginning build_DOD_product() ...\n");

   /* ======================================================================= */
   /* build graphical DIGITAL ONE-HOUR ACCUMULATION (DOD) Product 174         */
   /* ======================================================================= */

   p174_ptr = (int *) RPGC_get_outbuf_by_name(prodname, SIZE_P174, &opstat);

   /* If output buffer successfully acquired, proceed */

   if ( (opstat == RPGC_NORMAL) && (p174_ptr != NULL) )
   {
      /* Get product code number (174) */

      Code_DOD = RPGC_get_code_from_name(prodname);

      /* There is no null product check as we never make a null 174
       * product. Go ahead and convert the data into 256 levels */

      ret = compute_datalevel_diffprod (inbuf->One_Hr_diff,
                                        &minval,
                                        &maxval,
                                        &scale,
                                        &offset,
                                        Code_DOD,
                                        data_scaled);

      /* Don't bother checking the return value, we're always going
       * to make a product ...
       *
       * if(ret == FUNCTION_SUCCEEDED)
       * {
       * }
       */

      /* Encode as packet 16 */

      packet16_dig((short *) p174_ptr, data_scaled);

      /* Add the Message Header & Product Description blocks */

      dp8bit_product_header((short *) p174_ptr, vol_num, Code_DOD,
                             minval, maxval, scale, offset, inbuf);

      /* Release and forward the DOD output buffer */

      RPGC_rel_outbuf((void*) p174_ptr, FORWARD);

   } /* end if got output buffer */

} /* end build_DOD_product() ================================ */
