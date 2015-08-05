/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 22:54:27 $
 * $Id: DP_Precip_4bit_packetAF1Frle.c,v 1.5 2009/10/27 22:54:27 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/******************************************************************************
   Description
   ===========
      Run-length encode the  One Hour Accumulation (OHA) and Storm Total
   Accumulation (STA) products.

   Inputs:
      unsigned int* prod_length             - product length
      short accumGrid[][MAX_2KM_RESOLUTION] - accumulation grid
      int prod_id                           - product id
      Coldat_t* Color_data                  - color data

   Outputs:
      short* outbuf - product buffer containing run-length encoded data.

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----        -------    ----------------   ----------------------
   10/07       0000       Pham, Ward, Stein  Initial Implementation
******************************************************************************/

#include <packet_af1f.h>
#include "DP_Precip_4bit_func_prototypes.h"

int packetAF1F_rle(unsigned int* prod_length, short* outbuf,
                   short accumGrid[][MAX_2KM_RESOLUTION],
                   int prod_id, Coldat_t* Color_data)
{
   int   indx, az, ptr, delta, start;
   int   rle_index           = 0; /* byte counter */
   int   num_bytes_left      = 0; /* number of bytes left in the output buffer */
   int   buffind             = 0; /* short counter */
   int   rlebytes            = 0; /* byte counter */
   static short first_time   = TRUE;
   short encode_blank_radial = FALSE;
   unsigned int i4word       = 0;
   char msg[200]; /* stderr message */

   Graphic_product   *hdr;
   Symbology_block   *sym;
   packet_af1f_hdr_t *packet_af1f;

   static unsigned int graphic_size = sizeof(Graphic_product);
   static unsigned int sym_size     = sizeof(Symbology_block);

   static short zeroGrid[MAX_AZM][MAX_2KM_RESOLUTION];

   if(DP_PRECIP_4BIT_DEBUG)
     fprintf(stderr," packetAF1F_rle() prod_id: %d\n", prod_id);

   /* Check for NULL pointers. */

   if(pointer_is_NULL(prod_length, "packetAF1F_rle", "prod_length"))
      return(NULL_POINTER);

   if(pointer_is_NULL(outbuf, "packetAF1F_rle", "outbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(Color_data, "packetAF1F_rle", "Color_data"))
      return(NULL_POINTER);

   hdr = (Graphic_product *) outbuf;
   sym = (Symbology_block *) ((char *) outbuf + graphic_size);

   packet_af1f = (packet_af1f_hdr_t *) ((char *) sym + sym_size);

   if(first_time == TRUE)
   {
      memset(zeroGrid, NODATA_VALUE_4BIT,
             MAX_AZM * MAX_2KM_RESOLUTION * sizeof(short));

      first_time = FALSE;
   }

   if(DP_PRECIP_4BIT_DEBUG)
   {
      fprintf( stderr," OBUF_OVERHEAD ( %d )\n",
                      (graphic_size +
                       sizeof(packet_af1f_hdr_t) +
                       sizeof(packet_af1f_radial_data_t)) );
   }

   /* Set up radial data packet header block */

   packet_af1f->code              = 0xaf1f;
   packet_af1f->index_first_range = ZERO;
   packet_af1f->num_range_bins    = NUMBINS;
   packet_af1f->i_center          = ICENTER;
   packet_af1f->j_center          = JCENTER;
   packet_af1f->scale_factor      = 2000;
   packet_af1f->num_radials       = MAX_AZM;

   /* Initialize counters */

   buffind = (int) ((&packet_af1f->num_radials - outbuf) + 1);

   if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr,"\nbuffind( %d )\n",buffind);

   if(prod_id == CODE169)
      indx = COLOR_INDEX_169;
   else /* prod_id == CODE171 */
      indx = COLOR_INDEX_171;

   encode_blank_radial = FALSE;

   /* Do for each azimuth */

   for(az = 0; az < MAX_AZM; ++az)
   {
      /* Scale integer for starting angle */

      start = az * 10;

      /* Delta azimuth angle - one degree in tenths of a degree */

      delta = 10;

      /* First radial is a special case to correct for a ramtek glitch
       * need to start at 359 degrees and make delta two degrees */

      if(az == ZERO)
      {
         start = MX_DEG_IN_TENTHS;
         delta = 20;
      }

      /* Compute pointer to each radial */

      ptr = az * NUMBINS;

      if(encode_blank_radial == FALSE)
      {
         /* Check to see if we should start encoding blank radials
          *
          * Find the number of bytes left in the product output buffer.
          *
          * 150 = SYMB_OFFSET(120) + SYMB_SIZE(10) +
          *       LYR_HDR_SIZE(6)  + PKT_HDR_SIZE(14) */

         num_bytes_left = SIZE_4BIT - (rle_index + 150);

         if(prod_id == CODE171) /* STA also has a tab */
            num_bytes_left -= TAB_SIZE;

         /* If we don't have enough bytes to encode the rest of the
          * radials at EST_BYTES_PER_RAD, start encoding blank radials
          * (at about 14 bytes/radial). */

         if(num_bytes_left < ((MAX_AZM - az) * EST_BYTES_PER_RAD))
         {
            encode_blank_radial = TRUE; /* start encoding blank radials */

            sprintf(msg, "Product %d full after %d %s %d, %s\n",
                    prod_id, az, "radials, num_bytes_left", num_bytes_left,
                    "encoding blank radials");

            RPGC_log_msg(GL_INFO, msg);
            if(DP_PRECIP_4BIT_DEBUG)
               fprintf(stderr, msg);
         }
      }

      /* Color_data->coldat[] are the colors that accumGrid[]
       * will be run length encoded with. */

      if(encode_blank_radial)
      {
         RPGC_run_length_encode(start, delta, (short*) &zeroGrid[az], 0,
                                NUMBINS - 1, NUMBINS,
                                1, &(Color_data->coldat[indx][0]),
                                &rlebytes, buffind, outbuf);
      }
      else /* encode the normal radial */
      {
         RPGC_run_length_encode(start, delta, (short*) &accumGrid[az], 0,
                                NUMBINS - 1, NUMBINS,
                                1, &(Color_data->coldat[indx][0]),
                                &rlebytes, buffind, outbuf);
      }

      /* Update position counters */

      rle_index += rlebytes;
      buffind   += rlebytes / sizeof(short);

    } /* end az loop */

    if(DP_PRECIP_4BIT_DEBUG)
       fprintf(stderr, "rle_index %d; buffind %d\n", rle_index, buffind);

    /* Set up header to block1 */

    RPGC_set_product_int(&hdr->sym_off, graphic_size/sizeof(short));

    /* Fill in divider and block id */

    sym->divider  = -1;
    sym->block_id =  1;

    /* Fill in layer length and layer divider */

    i4word = rle_index + sizeof(packet_af1f_hdr_t);

    RPGC_set_product_int(&sym->data_len, i4word);

    sym->n_layers      =  1;
    sym->layer_divider = -1;

    i4word += sym_size;
    RPGC_set_product_int( &sym->block_len, i4word );

    *prod_length = (unsigned int) i4word;

    if(DP_PRECIP_4BIT_DEBUG)
       fprintf(stderr,"End packetAF1F_rle(); prod_length = %d\n", *prod_length);

    return(FUNCTION_SUCCEEDED);

} /* end packetAF1F_rle() ================================== */

/******************************************************************************
   Filename: DP_Precip_4bit_packetAF1Frle.c

   Description
   ===========
      create_threshold_table() create threshold table for 4-bit product (i.e
   Storm total product (STA) and One Hour Accumulation product (OHA). Both
   products are 16 color levels.

   NOTE: This 'thing' has traditionally been called a "color" table, but it
   really is used to set the thresholds for run-length encoding.  As of
   21 Aug 07, we are renaming these to "threshold" tables.  There is NO
   color information here. Stein/Ward

   Color table STMTOTNC in prcpprod Fortran
   STMTOTNC -> 23 from ~/include/a309.inc
   TABLE NUMBER  23 from ~/include/a3cd70g.ftn
   Use DATA(COLDAT)
   1* 0,  3* 1,  3* 2 -> 1 step at  threshold[0], 3 at  threshold[1],
   3 at  threshold[2], etc.

   *** NO LONGER USED, KEPT AROUND FOR COMPARISON PURPOSES ***

   Input:
      prod_id    - product id

   Output:
      threshold  - array threshold levels

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----        -------    ----------         -------------------------------
   20071007    0000       James Ward         Initial Implementation
   20080514    0001       James Ward         No longer used, we now do
                                             a direct read from the database
******************************************************************************/

/* void create_threshold_table ( short *oha_threshold, short *sta_threshold )
 * {
 *   -* fill the short array 'threshold' with threshold levels *-
 *
 *  int i;
 *
 *  -* OHA threshold levels *-
 *      for (i = 0; i < 1; i++)
 *        oha_threshold[i] = 0;
 *      for (i = 1; i < 3; i++)
 *        oha_threshold[i] = 1;
 *      for (i = 3; i < 6; i++)
 *        oha_threshold[i] = 2;
 *      for (i = 6; i < 11; i++)
 *        oha_threshold[i] = 3;
 *      for (i = 11; i < 16; i++)
 *        oha_threshold[i] = 4;
 *      for (i = 16; i < 21; i++)
 *        oha_threshold[i] = 5;
 *      for (i = 21; i < 26; i++)
 *        oha_threshold[i] = 6;
 *      for (i = 26; i < 31; i++)
 *        oha_threshold[i] = 7;
 *      for (i = 31; i < 36; i++)
 *        oha_threshold[i] = 8;
 *      for (i = 36; i < 41; i++)
 *        oha_threshold[i] = 9;
 *      for (i = 41; i < 51; i++)
 *        oha_threshold[i] = 10;
 *      for (i = 51; i < 61; i++)
 *        oha_threshold[i] = 11;
 *      for (i = 61; i < 81; i++)
 *        oha_threshold[i] = 12;
 *      for (i = 81; i < 121; i++)
 *        oha_threshold[i] = 13;
 *      for (i = 121; i < 161; i++)
 *        oha_threshold[i] = 14;
 *      for (i = 161; i < 256; i++)
 *        oha_threshold[i] = 15;
 *
 *  /- STA threshold levels -/
 *      for(i = 0; i < 1; i++)        /-   1   -/
 *         sta_threshold[i] = 0;
 *      for(i = 1; i < 4; i++)        /-   3   -/
 *         sta_threshold[i] = 1-;
 *      for(i = 4; i < 7; i++)        /-   3   -/
 *         sta_threshold[i] = 2;
 *      for(i = 7; i < 11; i++)       /-   4   -/
 *         sta_threshold[i] = 3;
 *      for(i = 11; i < 16; i++)      /-   5   -/
 *         sta_threshold[i] = 4;
 *      for(i = 16; i < 21; i++)      /-   5   -/
 *         sta_threshold[i] = 5;
 *      for(i = 21; i < 26; i++)      /-   5   -/
 *         sta_threshold[i] = 6;
 *      for(i = 26; i < 31; i++)      /-   5   -/
 *         sta_threshold[i] = 7;
 *      for(i = 31; i < 41; i++)      /-  10   -/
 *         sta_threshold[i] = 8;
 *      for(i = 41; i < 51; i++)      /-  10   -/
 *         sta_threshold[i] = 9;
 *      for(i = 51; i < 61; i++)      /-  10   -/
 *         sta_threshold[i] = 10;
 *      for(i = 61; i < 81; i++)      /-  20   -/
 *         sta_threshold[i] = 11;
 *      for(i = 81; i < 101; i++)     /-  20   -/
 *         sta_threshold[i] = 12;
 *      for(i = 101; i < 121; i++)    /-  20   -/
 *         sta_threshold[i] = 13;
 *      for(i = 121; i < 151; i++)    /-  30   -/
 *         sta_threshold[i] = 14;
 *      for(i = 151; i < 256; i++)    /- 105   -/
 *         sta_threshold[i] = 15;
 *
 *} /- create_threshold_table() ============================================== */
