/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/12 20:05:00 $
 * $Id: DP_Precip_4bit_STA_prod.c,v 1.6 2014/03/12 20:05:00 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/******************************************************************************
   Filename: DP_Precip_4bit_STA_prod.c

   Description
   ===========
      This function generates Storm total product (4-bit product) by using
   packet AF1F for graphical product and packet 56 for Tabular Alphanumeric
   block. The resolution is 2km by 1deg (16 color levels).

   Note: This product will generate every volume scan.

   Input:  LT_Accum_Buf_t* inbuf      - pointer to inbuf structure
           int             vol_num    - current volume scan number
           char*           prodname   - product name
           Coldat_t*       Color_data - color data
           Siteadp_adpt_t* Siteadp    - site adaptation info

   Output:

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ---------------    -----------------------------
   20071007     0000      Pham,Ward,Stein    Initial Implementation
******************************************************************************/

#include "DP_Precip_4bit_func_prototypes.h"

int Build_STA_product(LT_Accum_Buf_t* inbuf, int vol_num,
                      char* prodname, Coldat_t* Color_data,
                      Siteadp_adpt_t* Siteadp)
{
   int* p171_ptr = NULL;
   int  opstat   = RPGC_NORMAL;
   int  Code_STA = 0;
   int  minval   = 0;
   int  maxval   = 0;
   int  ret      = FUNCTION_SUCCEEDED; /* return value */
   int  offset   = 0; /* offset in bytes from start of msg */
   int  i4word   = 0;

   unsigned int TLength       = 0;
   unsigned int TOffset       = 0;
   unsigned int sym_block_len = 0;

   int   accum_grid[MAX_AZM][MAX_2KM_RESOLUTION];
   short precip_val[MAX_AZM][MAX_2KM_RESOLUTION];

   Symbology_block* sym_ptr = NULL;

   static unsigned int graphic_size = sizeof(Graphic_product);

   if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr,"\nBeginning Build_STA_product() ...\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(inbuf, "Build_STA_product", "inbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(prodname, "Build_STA_product", "prodname"))
      return(NULL_POINTER);

   if(pointer_is_NULL(Color_data, "Build_STA_product", "Color_data"))
      return(NULL_POINTER);

   if(pointer_is_NULL(Siteadp, "Build_STA_product", "Siteadp"))
      return(NULL_POINTER);

   /* ===================================================================== */
   /* Build graphical STORM TOTAL ACCUMULATION (STA) Product 171            */
   /* ===================================================================== */
   /* Determine whether product has been requested, this volume scan        */

   p171_ptr = (int *) RPGC_get_outbuf_by_name(prodname, SIZE_4BIT, &opstat);

   /* If output buffer successfully acquired, proceed */

   if ( (opstat == RPGC_NORMAL) && (p171_ptr != NULL) )
   {
      /* Get product code number (171) */

      Code_STA = RPGC_get_code_from_name(prodname);

      if(inbuf->supl.null_Storm_Total) /* make a null product */
      {
         make_null_symbology_block((char *) p171_ptr,
                                    inbuf->supl.null_Storm_Total,
                                    prodname,
                                    inbuf->qpe_adapt.accum_adapt.restart_time,
                                    inbuf->supl.last_time_prcp,
                                    inbuf->qpe_adapt.prep_adapt.rain_time_thresh);

         sym_ptr = (Symbology_block*) (((char*) p171_ptr) + graphic_size);

         RPGC_get_product_int(&(sym_ptr->block_len), &sym_block_len);
      }
      else /* make a non-null product */
      {
         /* Convert high resolution (250m) to low resolution (2km) */

         convert_Resolution(inbuf->Storm_Total, accum_grid);

         /* Find maximum accumulation value and convert grid to 256 values */

         ret = convert_precip_4bit(accum_grid,
                                   &minval,
                                   &maxval,
                                   Code_STA,
                                   precip_val);

         if(ret == FUNCTION_SUCCEEDED)
         {
            /* Run length encode the output array */

            packetAF1F_rle(&sym_block_len, (short *) p171_ptr, precip_val,
                           Code_STA, Color_data);
         }
         else /* couldn't do the convert, make a null product */
         {
            make_null_symbology_block((char *) p171_ptr,
                                       NULL_REASON_1,
                                       prodname,
                                       inbuf->qpe_adapt.accum_adapt.restart_time,
                                       inbuf->supl.last_time_prcp,
                                       inbuf->qpe_adapt.prep_adapt.rain_time_thresh);

            sym_ptr = (Symbology_block*) (((char*) p171_ptr) + graphic_size);

            RPGC_get_product_int(&(sym_ptr->block_len), &sym_block_len);
         }
      }

      /* Build the Product Header - Message Header & Product *
       * Description blocks)                                 */

      dp4bit_product_header((short *) p171_ptr, vol_num, Code_STA,
                            Color_data, maxval, inbuf);

      /* Generate Standard Tabular Alphanumeric product */

      STA_tab((char *) p171_ptr, (int) sym_block_len, (int*) &TLength,
              (int*) &TOffset, vol_num, inbuf, Siteadp, &offset);

      if(DP_PRECIP_4BIT_DEBUG)
         fprintf(stderr, "TOffset = %d shorts\n", TOffset);

      RPGC_set_prod_block_offsets( (short *) p171_ptr, 
                                   sizeof(Graphic_product)/sizeof(short),
                                   0, TOffset );

      /* Complete the Product Header. */

      i4word = TLength - sizeof(Graphic_product);
      RPGC_prod_hdr((void*) p171_ptr, CODE171, &i4word);

      /* Release output buffer of STA Product data and forward to storage */

      RPGC_rel_outbuf((void*) p171_ptr, FORWARD);
   }
   else if (p171_ptr != NULL)
   {
      RPGC_rel_outbuf((void*) p171_ptr, DESTROY);
   }

   return(FUNCTION_SUCCEEDED);

} /* end Build_STA_product() ==================== */
