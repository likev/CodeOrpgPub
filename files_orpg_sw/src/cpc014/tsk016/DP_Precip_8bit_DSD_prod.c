/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 22:51:25 $
 * $Id: DP_Precip_8bit_DSD_prod.c,v 1.4 2009/10/27 22:51:25 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/******************************************************************************
   Filename: DP_Precip_8bit_DSD_prod.c

   Description
   ===========
      This function generates the Digital Storm Total Difference product
   (8-bit product) by using packet 16 for graphical product. The resolution is
   250 m by 1 deg (256 color levels).

   This product is built like the DOD.

   The DSD is approximately the DSA (172) - DSP (138).

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

void build_DSD_product(LT_Accum_Buf_t* inbuf, int vol_num, char* prodname)
{
   int*  p175_ptr  = NULL;
   int   opstat    = RPGC_NORMAL;
   int   Code_DSD  = 0;
   int   minval    = 0;
   int   maxval    = 0;
   float scale     = 1.0;
   float offset    = 0.0;
   int   ret       = FUNCTION_SUCCEEDED; /* return value */
   unsigned char data_scaled[MAX_AZM][MAX_BINS]; /* accum grid scaled to *
                                                    256 levels           */
   if ( DP_PRECIP_8BIT_DEBUG )
      fprintf(stderr,"\nBeginning build_DSD_product() ...\n");

   /* ======================================================================= */
   /* build graphical DIGITAL STORM ACCUMULATION (DSD) Product 175            */
   /* ======================================================================= */

   p175_ptr = (int *) RPGC_get_outbuf_by_name(prodname, SIZE_P175, &opstat);

   /* If output buffer successfully acquired, proceed */

   if ( (opstat == RPGC_NORMAL) && (p175_ptr != NULL) )
   {
      /* Get the product code number (175) */

      Code_DSD = RPGC_get_code_from_name(prodname);

      if(inbuf->supl.null_Storm_Total_diff) /* make a null product */
      {
         make_null_symbology_block((char *) p175_ptr,
                                    inbuf->supl.null_Storm_Total_diff,
                                    prodname,
                                    inbuf->qpe_adapt.accum_adapt.restart_time,
                                    inbuf->supl.last_time_prcp,
                                    inbuf->qpe_adapt.prep_adapt.rain_time_thresh);
      }
      else /* make a non-null product */
      {
         /* Convert the data into 256 levels */

         ret = compute_datalevel_diffprod (inbuf->Storm_Total_diff,
                                           &minval,
                                           &maxval,
                                           &scale,
                                           &offset,
                                           Code_DSD,
                                           data_scaled);

         if(ret == FUNCTION_SUCCEEDED)
         {
            /* Encode as packet 16 */

            packet16_dig((short *) p175_ptr, data_scaled);
         }
         else /* couldn't do the convert */
         {
            make_null_symbology_block((char *) p175_ptr,
                                       NULL_REASON_1,
                                       prodname,
                                       inbuf->qpe_adapt.accum_adapt.restart_time,
                                       inbuf->supl.last_time_prcp,
                                       inbuf->qpe_adapt.prep_adapt.rain_time_thresh);
         }
      }

      /* Add the Message Header & Product Description blocks */

      dp8bit_product_header((short *) p175_ptr, vol_num, Code_DSD,
                             minval, maxval, scale, offset, inbuf);

      /* Release and forward the DSD output buffer */

      RPGC_rel_outbuf((void*) p175_ptr, FORWARD);

   } /* end if got output buffer */

} /* end build_DSD_product() ================================ */
