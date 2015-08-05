/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/07/29 22:35:58 $
 * $Id: DP_Precip_8bit_DAA_prod.c,v 1.5 2014/07/29 22:35:58 dberkowitz Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/******************************************************************************
   Filename: DP_Precip_8bit_DAA_prod.c

   Description
   ===========
      This function generates Digital One Hour Accumulation product (8-bit
   product) by using packet 16 for graphical product. The resolution is
   250 m by 1 deg (256 color levels).

   This product will generate every volume scan. It will represent the
   latest One_Hour_Unbiased product unless the most recent added data
   crosses the top of an hour.  When this happens the One_Hour_unbiased product
   is replaced with a Top_of_Hour unbiased (TOH) product.
   

   Input:  LT_Accum_Buf_t* inbuf    - pointer to inbuf structure
           int             vol_num  - current volume scan number
           char*           prodname - product name

   Output:

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   -----       -------    ---------------    ----------------------
   10/07       0000       Pham,Ward,Stein    Initial Implementation
   03/06/2014  0001       Murnan             Added Top Of Hour product 
                                             (CCR NA12-00223)
******************************************************************************/

#include "DP_Precip_8bit_func_prototypes.h"

void build_DAA_product(LT_Accum_Buf_t* inbuf, int vol_num, char* prodname)
{
   int*  p170_ptr  = NULL;
   int   opstat    = RPGC_NORMAL;
   int   Code_DAA  = 0;
   int   minval    = 0;
   int   maxval    = 0;
   float scale     = 1.0;
   float offset    = 0.0;
   int   ret       = FUNCTION_SUCCEEDED; /* return value */
   unsigned char data_scaled[MAX_AZM][MAX_BINS]; /* accum grid scaled to *
						    256 levels           */
   char  msg[200];       /* stderr message            */

   if ( DP_PRECIP_8BIT_DEBUG )
      fprintf(stderr,"Beginning build_DAA_product() ...\n");

   /* ======================================================================= */
   /* build graphical DIGITAL ONE-HOUR ACCUMULATION (DAA) Product 170         */
   /* ======================================================================= */

   p170_ptr = (int *) RPGC_get_outbuf_by_name(prodname, SIZE_P170, &opstat);

   /* If output buffer successfully acquired, proceed */

   if ( (opstat == RPGC_NORMAL) && (p170_ptr != NULL) )
   {
      /* Get product code number (170) */

      Code_DAA = RPGC_get_code_from_name(prodname);

      if(inbuf->supl.null_TOH_unbiased <= 0)  /* make TOH_unbiased product */
      {
         /* Convert the data into 256 levels */

         if(DP_PRECIP_8BIT_DEBUG)
         {
            sprintf(msg, "%s %s\n",
                "build_DAA_product:", "TOH_unbiased product generated.");
            fprintf(stderr,msg);
         }

         ret = convert_int_to_256_char (inbuf->TOH_unbiased,
                                        &minval,
                                        &maxval,
                                        &scale,
                                        &offset,
                                        Code_DAA,
                                        data_scaled);
         if(ret == FUNCTION_SUCCEEDED)
         {
            /* Encode as packet 16 */

            packet16_dig((short *) p170_ptr, data_scaled);
         }
         else /* couldn't do the convert */
         {

            if(DP_PRECIP_8BIT_DEBUG)
            {
               sprintf(msg, "%s %s %s\n",
                   "build_DAA_product:", "convert to 256 issue -",
                   "null TOH_unbiased product generated.");
               fprintf(stderr,msg);
            }


            make_null_symbology_block((char *) p170_ptr,
                                       inbuf->supl.null_TOH_unbiased,
                                       prodname,
                                       inbuf->qpe_adapt.accum_adapt.restart_time,
                                       inbuf->supl.last_time_prcp,
                                       inbuf->qpe_adapt.prep_adapt.rain_time_thresh);
         }
      }   /* end TOH_unbiased product */
      else if(inbuf->supl.null_One_Hr_unbiased <= 0) /* make One_Hr_unbiased product */
      {
         /* Convert the data into 256 levels */

         if(DP_PRECIP_8BIT_DEBUG)
         {
            sprintf(msg, "%s %s\n",
                "build_DAA_product:", "One_Hour_unbiased product generated.");
            fprintf(stderr,msg);
         }

         ret = convert_int_to_256_char (inbuf->One_Hr_unbiased,
                                        &minval,
                                        &maxval,
                                        &scale,
                                        &offset,
                                        Code_DAA,
                                        data_scaled);
         if(ret == FUNCTION_SUCCEEDED)
         {
            /* Encode as packet 16 */

            packet16_dig((short *) p170_ptr, data_scaled);
         }
         else /* couldn't do the convert */
         {

            if(DP_PRECIP_8BIT_DEBUG)
            {
               sprintf(msg, "%s %s %s\n",
                   "build_DAA_product:", "convert to 256 issue -",
                   "null One_Hour_unbiased product generated.");
               fprintf(stderr,msg);
            }

            make_null_symbology_block((char *) p170_ptr,
                                       inbuf->supl.null_One_Hr_unbiased,
                                       prodname,
                                       inbuf->qpe_adapt.accum_adapt.restart_time,
                                       inbuf->supl.last_time_prcp,
                                       inbuf->qpe_adapt.prep_adapt.rain_time_thresh);
         }
      }   /* end One_Hr_unbiased product */
      else if(inbuf->supl.null_TOH_unbiased >= 6 &&
              inbuf->supl.null_TOH_unbiased <= 7) /* make a null TOH_unbiased product */
      {
         if(DP_PRECIP_8BIT_DEBUG)
         {
            sprintf(msg, "%s %s %s\n",
                "build_DAA_product:", "TOH null product code 6 or 7 - ",
                "null TOH_unbiased product generated.");
            fprintf(stderr,msg);
         }

         make_null_symbology_block((char *) p170_ptr,
                                    inbuf->supl.null_TOH_unbiased,
                                    prodname,
                                    inbuf->qpe_adapt.accum_adapt.restart_time,
                                    inbuf->supl.last_time_prcp,
                                    inbuf->qpe_adapt.prep_adapt.rain_time_thresh);
      }  /* end null TOH product */
      else /* make a null One_Hr_unbiased product */
      {

         if(DP_PRECIP_8BIT_DEBUG)
         {
            sprintf(msg, "%s %s %s %s\n",
                "build_DAA_product:", "TOH null product codes 1-5 ",
                "and One_Hour null product codes 1-5 - ", 
                "null One_Hour_unbiased product generated.");
            fprintf(stderr,msg);
         }

         make_null_symbology_block((char *) p170_ptr,
                                    inbuf->supl.null_One_Hr_unbiased,
                                    prodname,
                                    inbuf->qpe_adapt.accum_adapt.restart_time,
                                    inbuf->supl.last_time_prcp,
                                    inbuf->qpe_adapt.prep_adapt.rain_time_thresh);
      }  /* end if make product */

      /* Add the Message Header & Product Description blocks */

      dp8bit_product_header((short *) p170_ptr, vol_num, Code_DAA,
                             minval, maxval, scale, offset, inbuf);

      /* Release and forward the DAA output buffer */

      RPGC_rel_outbuf((void*) p170_ptr, FORWARD);

   } /* end if got output buffer */

} /* end build_DAA_product() ================================= */
