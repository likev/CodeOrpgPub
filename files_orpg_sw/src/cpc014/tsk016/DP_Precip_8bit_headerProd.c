/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/07/29 22:35:58 $
 * $Id: DP_Precip_8bit_headerProd.c,v 1.5 2014/07/29 22:35:58 dberkowitz Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/*****************************************************************************
   Filename: DP_Precip_8bit_headerProd.c

   Description
   ===========
      Builds product header portion of buffer for 8-bit products (DAA, DSA,
   DOD, and DSD).

   Inputs:
      short*          prodbuf  - pointer to product buffer.
      int             vsnum    - volume scan number
      int             prodcode - product code
      int             minval   - minimum data level
      int             maxval   - maximum data level
      float           scale    - scale used in encoding
      float           offset   - offset used in encoding
      LT_Accum_Buf_t* inbuf    - input buffer with supplemental data

   Output: None

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ---------------    -----------------------------
   20071007     0000      Pham,Ward,Stein    Initial Implementation
   20080709     0001      Ward               Added product version numbers
   20140331     0002      Murnan             Added TOH and HW 27&28 definitions
                                             (CCRs NA12-00223 and NA12-00264)
******************************************************************************/

#include "DP_Precip_8bit_func_prototypes.h"

void dp8bit_product_header(short* prodbuf, int vsnum, int prodcode,
                           int minval, int maxval, float scale, float offset,
                           LT_Accum_Buf_t* inbuf)
{
   /* i4word is Fortran speak for "a word represented as a 4 byte integer".
    * A halfword is a 2 byte short. */

   int i4word              = 0;
   int julian_date         = 0;
   int mins_since_midnight = 0;

   Graphic_product* hdr; /* start of (Graphic) Product header */
   Symbology_block* sym; /* start of Symbology block          */

   unsigned char* ptemp; /* temporary pointer to facilitate writing of
                          * product version + spot blank flag into
                          * the final product */

   static unsigned int graphic_size = sizeof(Graphic_product);

  if ( DP_PRECIP_8BIT_DEBUG )
      fprintf(stderr,"\ndp8bit_product_header(max %d, min %d)\n",
                       maxval, minval );

   hdr = (Graphic_product *) prodbuf;
   sym = (Symbology_block *) ((char *) prodbuf + graphic_size);

   /* Initialize Graphic_product header to NULL. */

   memset((void *) prodbuf, 0, graphic_size);

   /* Initialize some fields in the product description block */

   RPGC_prod_desc_block((void *) prodbuf, prodcode, vsnum);

   /* This is a digital, 8-bit, 256-level product. scale/offset are
    * floats, and can pretty much be anything, so no check function is
    * used. To check that scale/offset are encoded correctly by the
    * RPG, Brian Klein suggest this URL:
    *
    *    http://babbage.cs.qc.edu/IEEE-754/Decimal.html
    *
    * Enter your Decimal Floating-Point: -60
    * Click on Not Rounded, and you should see:
    * Hexadecimal: C2700000,
    *
    * so under cvt:
    *
    * hdr->level_1 (halfword 31) should be C270
    * hdr->level_2 (halfword 32) should be 0000.
    *
    * To go back the other way, from hex to IEEE float, use:
    *
    * http://babbage.cs.qc.edu/IEEE-754/32bit.html.
    *
    * The difference products, 175/175, have a fixed offset = 128,
    * which should encode as hex 4300 0000 */

   RPGC_set_product_float((void*) &(hdr->level_1), scale);
   RPGC_set_product_float((void*) &(hdr->level_3), offset);

   /* According to Brian Klein, level_5 = hw 35 is reserved
    * by the FAA (Digital Vil) to store a logarithmic scale.
    * See p. 3-33 of the ICD. */

   hdr->level_6 = UCHAR_MAX; /* hw 36, = 255 = max data level  */
   hdr->level_7 =         1; /* hw 37, 1 leading flag, No Data */
   hdr->level_8 =         0; /* hw 38, 0 trailing flags        */

   switch (prodcode)
   {
      case CODE170: /* DAA */

         /* NULL product flag HW30 */
         /* confirm/set null product flag for TOH_unbiased */

         hdr->param_3 = check_null_product(inbuf->supl.null_TOH_unbiased,
                                          "inbuf->supl.null_TOH_unbiased");

         /* create product or a null product for TOH_unbiased */

         if(inbuf->supl.null_TOH_unbiased == FALSE ||
            (inbuf->supl.null_TOH_unbiased >= NULL_REASON_6 &&
            inbuf->supl.null_TOH_unbiased <= NULL_REASON_7))
         {
            /* minimum time for a product to be generated (mins) HW 27 */
            hdr->param_1 = (short) inbuf->supl.TOH_min_time_period;

            /* actual time accumulated (rounded mins) HW 28 */
            hdr->param_2 = (short) RPGC_NINT(inbuf->supl.total_TOH_accum_time /
                                   SECS_PER_MIN);

            /* End date/time (in minutes) at HW48 and HW49 */

            UNIX_time_to_julian_mins (inbuf->supl.TOH_endtime,
                                   &julian_date, &mins_since_midnight);
         }
         else /* confirm/set null product for One_Hr_unbiased */          
         {
            /* NULL product flag HW30 */

            hdr->param_3 = check_null_product(inbuf->supl.null_One_Hr_unbiased,
                                          "inbuf->supl.null_One_Hr_unbiased");

            /* HW 27 and 28 not set if not TOH */


            /* End date/time (in minutes) at HW48 and HW49 */

            UNIX_time_to_julian_mins (inbuf->supl.hrly_endtime,
                                   &julian_date, &mins_since_midnight); 
         }

         hdr->param_5 = check_date((short) julian_date);
         hdr->param_6 = check_time((short) mins_since_midnight);

         break; /* DAA */

      case CODE172: /* DSA */

         /* Begin date/time (in minutes) at HW27 and HW28 */

         UNIX_time_to_julian_mins (inbuf->supl.stmtot_begtime,
                                   &julian_date, &mins_since_midnight);

         hdr->param_1 = check_date((short) julian_date);
         hdr->param_2 = check_time((short) mins_since_midnight);

         /* Null product flag */

         hdr->param_3 = check_null_product(inbuf->supl.null_Storm_Total,
                                          "inbuf->supl.null_Storm_Total");

         /* End date/time (in minutes) at HW48 and HW49 */

         UNIX_time_to_julian_mins (inbuf->supl.stmtot_endtime,
                                   &julian_date, &mins_since_midnight);

         hdr->param_5 = check_date((short) julian_date);
         hdr->param_6 = check_time((short) mins_since_midnight);

         break; /* DSA */

      case CODE174: /* DOD */

         /* The DOD product is never null, so a Null product flag is not set.
          *
          * End date/time (in minutes) at HW48 and HW49. */

         UNIX_time_to_julian_mins (inbuf->supl.hrlydiff_endtime,
                                   &julian_date, &mins_since_midnight);

         hdr->param_5 = check_date((short) julian_date);
         hdr->param_6 = check_time((short) mins_since_midnight);

         break; /* DOD */

      default: /* CODE175 - DSD */

         /* Begin date/time (in minutes) at HW27 and HW28 */

         UNIX_time_to_julian_mins (inbuf->supl.stmdiff_begtime,
                                   &julian_date, &mins_since_midnight);

         hdr->param_1 = check_date((short) julian_date);
         hdr->param_2 = check_time((short) mins_since_midnight);

         /* Null product flag */

         hdr->param_3 = check_null_product(inbuf->supl.null_Storm_Total_diff,
                                          "inbuf->supl.null_Storm_Total_diff");

         /* End date/time (in minutes) at HW48 and HW49 */

         UNIX_time_to_julian_mins (inbuf->supl.stmdiff_endtime,
                                   &julian_date, &mins_since_midnight);

         hdr->param_5 = check_date((short) julian_date);
         hdr->param_6 = check_time((short) mins_since_midnight);

         break; /* DSD */
   }

   /* Maximum accumulation value/difference (in 10th of an inch): HW47.
    * maxval is in 1000ths of an inch. */

   switch (prodcode)
   {
      case CODE170: /* DAA */
      case CODE172: /* DSA */

         hdr->param_4 = (short) RPGC_NINT(SCALE_MAX_ACCUM /* 10.0 */ *
                                check_max_accum(maxval / 1000.0));

         /* Mean field bias for DAA/DSA is at HW50 */

         hdr->param_7 = (short) RPGC_NINT(SCALE_MEAN_FIELD_BIAS /* 100.0 */ *
                                check_mean_field_bias(inbuf->qpe_adapt.bias_info.bias));
         break;

      default: /* CODE174 - DOD and CODE175 - DSD */

         hdr->param_4 = (short) RPGC_NINT(SCALE_MAX_ACCUM_DIFF /* 10.0 */ *
                                check_max_accum_diff(maxval / 1000.0));

        /* Min accum difference (10th of an inch) for DOD/DSD is at HW50
          * Convert from 1000s of an inch to 10ths of an inch. */

         hdr->param_7 = (short) RPGC_NINT(SCALE_MIN_ACCUM_DIFF /* 10.0 */ *
                                check_min_accum_diff(minval / 1000.0));
         break;
   }

   /* NOTE: param_8, param_9, and param_10 are reserved for BZIP2 product
    * compression. All packet 16 (8 bit) products use compression.
    *
    * Set the product version in the high byte of HW 54 (n_maps),
    * preserving the spot blank flag in the low byte. */

   ptemp = (unsigned char *) &(hdr->n_maps); /* ptemp is a temp pointer
                                              * to facilitate output     */
   *ptemp = (unsigned char) inbuf->supl.vol_sb;

   ptemp += 1; /* move up 1 byte */

   switch (prodcode)
   {
      case CODE170: *ptemp = DAA_VERSION;
                    break;
      case CODE172: *ptemp = DSA_VERSION;
                    break;
      case CODE174: *ptemp = DOD_VERSION;
                    break;
           default: *ptemp = DSD_VERSION; /* CODE175 */
                    break;
   }

   /* Set the offset to the symbology block, GAB (0 -> product has none),
    * and TAB (0 -> product has none) */

   RPGC_set_prod_block_offsets((void*) prodbuf,
                                graphic_size/sizeof(short),
                                0, 0);

   /* Get the symbology block length */

   RPGC_get_product_int(&(sym->block_len), &i4word);

   /* Store the product code and sym block length in the product header */

   RPGC_prod_hdr((void*) prodbuf, prodcode, &i4word);

} /* end dp8bit_product_header() ========================== */
