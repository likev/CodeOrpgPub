/*
 */

/******************************************************************************
   Filename: DP_Precip_8bit_DSA_prod.c

   Description
   ===========
      This function generates Digital Storm-Total Accumulation product
   (8-bit product) by using packet 16 for graphical product. The resolution is
   250 m by 1 deg (256 color levels).

   This product is built like the DAA, with adaptable parameters/supplemental
   data/bias info added in a 2nd layer, via append_ascii_layer2().

   This product will generate every volume scan.

   Input:  LT_Accum_Buf_t* inbuf    - pointer to inbuf structure
           int             vol_num  - current volume scan number
           char*           prodname - product name

   Output:  None

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   -----       -------    ---------------    ----------------------
   10/07       0000       Pham,Ward,Stein    Initial Implementation
******************************************************************************/

#include "DP_Precip_8bit_func_prototypes.h"

void build_DSA_product(LT_Accum_Buf_t* inbuf, int vol_num, char* prodname,
                       int DSA_max)
{
   int*  p172_ptr  = NULL;
   int   opstat    = RPGC_NORMAL;
   int   Code_DSA  = 0;
   int   minval    = 0;
   int   maxval    = 0;
   float scale     = 1.0;
   float offset    = 0.0;
   int   ret       = FUNCTION_SUCCEEDED; /* return value */
   unsigned char data_scaled[MAX_AZM][MAX_BINS]; /* accum grid scaled to *
                                                    256 levels           */
   Symbology_block* sym_ptr = NULL;
   unsigned int  sym_block_len = 0;

   static unsigned int graphic_size = sizeof(Graphic_product);

   if ( DP_PRECIP_8BIT_DEBUG )
      fprintf(stderr,"\nBeginning build_DSA_product() ...\n");

   /* ===================================================================== */
   /* build graphical DIGITAL STORM-TOTAL ACCUMULATION (DSA) Product 172    */
   /* ===================================================================== */

   p172_ptr = (int *) RPGC_get_outbuf_by_name(prodname, SIZE_P172, &opstat );

   /* If output buffer successfully acquired, proceed */

   if ( (opstat == RPGC_NORMAL) && (p172_ptr != NULL) )
   {
      /* Get product code number (172) */

      Code_DSA = RPGC_get_code_from_name(prodname);

      if(inbuf->supl.null_Storm_Total) /* make a null product */
      {
         make_null_symbology_block((char *) p172_ptr,
                                    inbuf->supl.null_Storm_Total,
                                    prodname,
                                    inbuf->qpe_adapt.accum_adapt.restart_time,
                                    inbuf->supl.last_time_prcp,
                                    inbuf->qpe_adapt.prep_adapt.rain_time_thresh);

         /* We get a pointer to the symbology block because we still
          * want to append the layer 2 after the null product messages. */

         sym_ptr = (Symbology_block*) (((char*) p172_ptr) + graphic_size);

         RPGC_get_product_int(&(sym_ptr->block_len), &sym_block_len);
      }
      else /* make a non-null product */
      {
         /* Convert the data into 256 levels */

         ret = convert_DSA_int_to_256_char (inbuf->Storm_Total,
                                            &minval,
                                            &maxval,
                                            &scale,
                                            &offset,
                                            Code_DSA,
                                            data_scaled,
                                            DSA_max);
         if(ret == FUNCTION_SUCCEEDED)
         {
            /* Encode as packet 16 */

            sym_block_len = (unsigned int) packet16_dig((short *) p172_ptr,
                                                                  data_scaled);
         }
         else /* couldn't do the convert */
         {
            make_null_symbology_block((char *) p172_ptr,
                                       NULL_REASON_1,
                                       prodname,
                                       inbuf->qpe_adapt.accum_adapt.restart_time,
                                       inbuf->supl.last_time_prcp,
                                       inbuf->qpe_adapt.prep_adapt.rain_time_thresh);

            /* We get a pointer to the symbology block because we still
             * want to append the layer 2 after the null product messages. */

            sym_ptr = (Symbology_block*) (((char*) p172_ptr) + graphic_size);

            RPGC_get_product_int(&(sym_ptr->block_len), &sym_block_len);
         }
      }

      /* Add adaptable parameters/supplemental data/bias info as an
       * ASCII data in Layer 2 of the product buffer */

      append_ascii_layer2 (inbuf, (char *) p172_ptr, sym_block_len, vol_num);

      /* Add the Message Header & Product Description blocks */

      dp8bit_product_header((short *) p172_ptr, vol_num, Code_DSA,
                             minval, maxval, scale, offset, inbuf);

      /* Release and forward the DSA output buffer */

      RPGC_rel_outbuf((void*) p172_ptr, FORWARD);

   } /* end if got output buffer */

} /* end build_DSA_product() ============================= */
