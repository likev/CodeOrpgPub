/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/11 21:35:14 $
 * $Id: DP_Precip_4bit_OHA_prod.c,v 1.5 2014/03/11 21:35:14 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/******************************************************************************
   Filename: DP_Precip_4bit_OHA_prod.c

   Description
   ===========
      This function generates One Hour Accumulation product (4-bit product) by
   using packet AF1F for graphical product.  The resolution is 2km by 1deg
   (16 color levels).

   Note: This product will generate every volume scan.

   Input:  LT_Accum_Buf_t* inbuf      - pointer to inbuf structure
           int             vol_num    - current volume scan number
           char*           prodname   - product name
           Coldat_t*       Color_data - color data

   Output:

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ---------------    -----------------------------
   20071007     0000      Pham,Ward,Stein    Initial Implementation
******************************************************************************/

#include "DP_Precip_4bit_func_prototypes.h"

int Build_OHA_product(LT_Accum_Buf_t* inbuf, int vol_num, char* prodname,
                      Coldat_t* Color_data)
{
   int* p169_ptr = NULL;
   int  opstat   = RPGC_NORMAL;
   int  Code_OHA = 0;
   int  minval   = 0;
   int  maxval   = 0;
   int  ret      = FUNCTION_SUCCEEDED; /* return value */
   unsigned int sym_block_len = 0; /* used by STA to set tab */

   int accum_grid[MAX_AZM][MAX_2KM_RESOLUTION];
   short precip_val[MAX_AZM][MAX_2KM_RESOLUTION];

   if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr,"\nBeginning Build_OHA_product() ...\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(inbuf, "Build_OHA_product", "inbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(prodname, "Build_OHA_product", "prodname"))
      return(NULL_POINTER);

   if(pointer_is_NULL(Color_data, "Build_OHA_product", "Color_data"))
      return(NULL_POINTER);

   /* ===================================================================== */
   /* Build graphical ONE HOUR ACCUMULATION (DSA) Product 168               */
   /* ===================================================================== */
   /* Determine whether product has been requested, this volume scan        */

   p169_ptr = (int *) RPGC_get_outbuf_by_name(prodname, SIZE_4BIT, &opstat);

   /* If output buffer successfully acquired, proceed */

   if ((opstat == RPGC_NORMAL) && (p169_ptr != NULL))
   {
       /* Get product code number (169) */

       Code_OHA = RPGC_get_code_from_name(prodname);

       if(inbuf->supl.null_One_Hr_biased) /* make a null product */
       {
          make_null_symbology_block((char *) p169_ptr,
                                     inbuf->supl.null_One_Hr_biased,
                                     prodname,
                                     inbuf->qpe_adapt.accum_adapt.restart_time,
                                     inbuf->supl.last_time_prcp,
                                     inbuf->qpe_adapt.prep_adapt.rain_time_thresh);
       }
       else /* make a non-null product */
       {
          /* Convert high resolution (250m) to low resolution (2km) */

          convert_Resolution(inbuf->One_Hr_biased, accum_grid);

          /* Find maximum accumulation value and convert grid to 256 values */

          ret = convert_precip_4bit(accum_grid,
                                    &minval,
                                    &maxval,
                                    Code_OHA,
                                    precip_val);

          if(ret == FUNCTION_SUCCEEDED)
          {
              /* Run length encode the output array */

              packetAF1F_rle(&sym_block_len, (short *) p169_ptr, precip_val,
                             Code_OHA, Color_data);
          }
          else /* couldn't do the convert */
          {
              make_null_symbology_block((char *) p169_ptr,
                                        NULL_REASON_1,
                                        prodname,
                                        inbuf->qpe_adapt.accum_adapt.restart_time,
                                        inbuf->supl.last_time_prcp,
                                        inbuf->qpe_adapt.prep_adapt.rain_time_thresh);
          }
       }

       /* Build the Product Header - Message Header & Product Description
        * blocks */

       dp4bit_product_header((short *) p169_ptr, vol_num, Code_OHA,
                             Color_data, maxval, inbuf);

       /* Release output buffer of OHA Product data and forward to storage */

       RPGC_rel_outbuf((void*) p169_ptr, FORWARD);
    }
    else if (p169_ptr != NULL)
    {
       RPGC_rel_outbuf((void*) p169_ptr, DESTROY);
    }

    return(FUNCTION_SUCCEEDED);

} /* end Build_OHA_product() ========================================== */
