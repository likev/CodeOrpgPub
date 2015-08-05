/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/11 21:35:15 $
 * $Id: DP_Precip_4bit_headerProd.c,v 1.5 2014/03/11 21:35:15 steves Exp $
 * $Revision: 1.5 $
 * $
 */

/***********************************************************************
   Filename: DP_Precip_4bit_headerProd.c

   Description
   ===========
      Builds product header portion of buffer for OHA and STA products.

   Inputs:
      short*          prodbuf        - pointer to product buffer.
      int             vsnum          - volume scan number
      int             prodcode       - product code
      Coldat_t*       Color_data     - color data
      int             maxval         - maximum accum
      LT_Accum_Buf_t* inbuf          - for Supplemental Data

   Output: None

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ---------------    -----------------------------
   20071007     0000      Pham,Ward,Stein    Initial Implementation
   20080709     0001      Ward               Added product version numbers
************************************************************************/

#include "DP_Precip_4bit_func_prototypes.h"

int dp4bit_product_header(short* prodbuf, int vsnum, int prodcode,
                          Coldat_t* Color_data, int maxval,
                          LT_Accum_Buf_t* inbuf)
{
   int indx, i4word;
   int result = -1; /* result from RPGC function calls */
   int julian_date, mins_since_midnight;
   unsigned char* ptemp = NULL; /* temporary pointer to facilitate writing of
                                 * product version and spot blank flag into
                                 * the final product */

   static unsigned int graphic_size = sizeof(Graphic_product);

   Graphic_product *hdr = (Graphic_product *) prodbuf;
   Symbology_block *sym = (Symbology_block *)
                          ((char *) prodbuf + graphic_size);

   /* Check for NULL pointers. */

   if(pointer_is_NULL(prodbuf, "dp4bit_product_header", "prodbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(Color_data, "dp4bit_product_header", "Color_data"))
      return(NULL_POINTER);

   if(pointer_is_NULL(inbuf, "dp4bit_product_header", "inbuf"))
      return(NULL_POINTER);

   /* Initialize product buffer */

   memset((void *) prodbuf, 0, graphic_size);

   /* Initialize some fields in the product description block */

   RPGC_prod_desc_block((void *) prodbuf, prodcode, vsnum);

   /* Product data levels (16-level, 4-bit product) */

   if(prodcode == CODE169)
      indx = COLOR_INDEX_169;
   else
      indx = COLOR_INDEX_171;

   /* Set the data level thresholds. I assume these are used
    * in the output scale display. */

   hdr->level_1  = Color_data->thresh[indx][0];
   hdr->level_2  = Color_data->thresh[indx][1];
   hdr->level_3  = Color_data->thresh[indx][2];
   hdr->level_4  = Color_data->thresh[indx][3];
   hdr->level_5  = Color_data->thresh[indx][4];
   hdr->level_6  = Color_data->thresh[indx][5];
   hdr->level_7  = Color_data->thresh[indx][6];
   hdr->level_8  = Color_data->thresh[indx][7];
   hdr->level_9  = Color_data->thresh[indx][8];
   hdr->level_10 = Color_data->thresh[indx][9];
   hdr->level_11 = Color_data->thresh[indx][10];
   hdr->level_12 = Color_data->thresh[indx][11];
   hdr->level_13 = Color_data->thresh[indx][12];
   hdr->level_14 = Color_data->thresh[indx][13];
   hdr->level_15 = Color_data->thresh[indx][14];
   hdr->level_16 = Color_data->thresh[indx][15];

   /* One Hour product (169): end date/time at HW48 and HW49
    *                         null prod flag  at HW30
    *
    * Note: The One Hour product has nothing filled in at HW27 (param_1)
    *       and HW28 (param_2)
    *
    * Storm total product (171): begin date/time at HW27 and HW28
    *                            null prod flag  at HW30
    *                            end date/time   at HW48 and HW49 (minutes) */

   if(prodcode == CODE169)
   {
      hdr->param_1 = 0; /* set so won't have spurious value */
      hdr->param_2 = 0; /* set so won't have spurious value */

      hdr->param_3 = check_null_product(inbuf->supl.null_One_Hr_biased,
                                       "inbuf->supl.null_One_Hr_biased");

      UNIX_time_to_julian_mins (inbuf->supl.hrly_endtime,
                                &julian_date, &mins_since_midnight);

      hdr->param_5 = check_date((short) julian_date);
      hdr->param_6 = check_time((short) mins_since_midnight);
   }
   else /* prodcode == CODE171 */
   {
      UNIX_time_to_julian_mins (inbuf->supl.stmtot_begtime,
                                &julian_date, &mins_since_midnight);

      hdr->param_1 = check_date((short) julian_date);
      hdr->param_2 = check_time((short) mins_since_midnight);

      hdr->param_3 = check_null_product(inbuf->supl.null_Storm_Total,
                                       "inbuf->supl.null_Storm_Total");

      UNIX_time_to_julian_mins (inbuf->supl.stmtot_endtime,
                                &julian_date, &mins_since_midnight);

      hdr->param_5 = check_date((short) julian_date);
      hdr->param_6 = check_time((short) mins_since_midnight);
   }

   /* Maximum accumulation value (in 10th of an inch): HW47.
    * maxval is in 1000s of an inch. */

   /* Ward - for testing, use to see full maxval:
    * hdr->param_4 = (short) RPGC_NINT(SCALE_MAX_ACCUM *
                             check_max_accum(maxval)); */

   hdr->param_4 = (short) RPGC_NINT(SCALE_MAX_ACCUM *
                          check_max_accum(maxval / 1000.0));

   /* Mean field bias HW50 */

   hdr->param_7 = (short) RPGC_NINT(SCALE_MEAN_FIELD_BIAS *
                          check_mean_field_bias(inbuf->qpe_adapt.bias_info.bias));

   /* OHA and STA use packet AF1F which is already compressed
    * so they can use param_8 to param_10.
    *
    * Sample size (effective No. Gage/Radar Pairs) in 100s. HW51 */

   hdr->param_8 = (short) RPGC_NINT(SCALE_SAMPLE_SIZE *
                          check_sample_size(inbuf->qpe_adapt.bias_info.grpsiz));

   /* Set the product version in the high byte of HW 54 (n_maps),
    * preserving the spot blank flag in the low byte. */

   ptemp = (unsigned char *) &(hdr->n_maps); /* ptemp is a temp pointer
                                              * to facilitate code writing */
   *ptemp = (unsigned char) inbuf->supl.vol_sb;

   ptemp += 1; /* move up 1 byte */

   if(prodcode == CODE169)
      *ptemp = OHA_VERSION;
   else /* prodcode == CODE171 */
      *ptemp = STA_VERSION;

   /* Product header offset and calculate product message length, in bytes
    * (= header + block1) (retrieve length of product symbology block from
    * its sub-header */

   if(prodcode == CODE169)
   {
      RPGC_set_prod_block_offsets(prodbuf,
                                  graphic_size/sizeof(short),
	                          0, 0);

      RPGC_get_product_int(&sym->block_len, &i4word);
      result = RPGC_prod_hdr((void*) prodbuf, prodcode, &i4word);

    }

    /* Note: If CODE171, RPGC_set_prod_block_offsets and RPGC_prod_hdr
     *       are called in Build_STA_product() */

    if(DP_PRECIP_4BIT_DEBUG)
    {
       fprintf(stderr,"BLOCK LENGTH (i4word) %d bytes\tPRODCODE: %d\n",
                       i4word, prodcode);
       fprintf(stderr, "result of call to RPGC_prod_hdr: %d\n", result);
       fprintf(stderr, "PRODUCT SIZE is now (i4word) (%d) bytes\n", i4word);
    }

    return(FUNCTION_SUCCEEDED);

} /* end dp4bit_product_header() =========================== */

/* The following is OLD_CODE, kept around for comparison purposes:
 *
 *      * Use OHA_COLOR_INDEX from a3cd70g.ftn file (TABLE NUMBER 22). *
 *
 *      hdr->level_1  = (short) 0xA002;   * code for ND - no data      *
 *      hdr->level_2  = (short) 0x2800;   * code for > 0.00            *
 *      hdr->level_3  = (short) 0x2002;   *            0.10            *
 *      hdr->level_4  = (short) 0x2005;   *            0.25            *
 *      hdr->level_5  = (short) 0x200A;   *            0.50            *
 *      hdr->level_6  = (short) 0x200F;   *            0.75            *
 *      hdr->level_7  = (short) 0x2014;   *            1.00            *
 *      hdr->level_8  = (short) 0x2019;   *            1.25            *
 *      hdr->level_9  = (short) 0x201E;   *            1.50            *
 *      hdr->level_10 = (short) 0x2023;   *            1.75            *
 *      hdr->level_11 = (short) 0x2028;   *            2.00            *
 *      hdr->level_12 = (short) 0x2032;   *            2.50            *
 *      hdr->level_13 = (short) 0x203C;   *            3.00            *
 *      hdr->level_14 = (short) 0x2050;   *            4.00            *
 *      hdr->level_15 = (short) 0x2078;   *            6.00            *
 *      hdr->level_16 = (short) 0x20A0;   *            8.00            *
 *   }
 *   else if ( prodcode == CODE171 )
 *   {
 *      * Use STA COLOR INDEX from a3cd70g.ftn file (TABLE NUMBER  23). *
 *
 *      hdr->level_1  = (short) 0x9002; * code for ND - no data         *
 *      hdr->level_2  = (short) 0x1800;
 *      hdr->level_3  = (short) 0x1003; * 0x1000 +  3                   *
 *      hdr->level_4  = (short) 0x1006; * 0x1003 +  3                   *
 *      hdr->level_5  = (short) 0x100A; * 0x1006 +  4                   *
 *      hdr->level_6  = (short) 0x100F; * 0x100A +  5                   *
 *      hdr->level_7  = (short) 0x1014; * 0x100F +  5                   *
 *      hdr->level_8  = (short) 0x1019; * 0x1014 +  5                   *
 *      hdr->level_9  = (short) 0x101E; * 0x1019 +  5                   *
 *      hdr->level_10 = (short) 0x1028; * 0x101E +  A                   *
 *      hdr->level_11 = (short) 0x1032; * 0x1028 +  A                   *
 *      hdr->level_12 = (short) 0x103C; * 0x1032 +  A                   *
 *      hdr->level_13 = (short) 0x1050; * 0x103C + 14                   *
 *      hdr->level_14 = (short) 0x1064; * 0x1050 + 14                   *
 *      hdr->level_15 = (short) 0x1078; * 0x1064 + 14                   *
 *      hdr->level_16 = (short) 0x1096; * 0x1078 + 1E                   *
 *                                      * 0x10FF = 0x1096 + 69          *
 * } */
