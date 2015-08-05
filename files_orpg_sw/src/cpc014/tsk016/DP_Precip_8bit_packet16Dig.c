/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 22:51:26 $
 * $Id: DP_Precip_8bit_packet16Dig.c,v 1.4 2009/10/27 22:51:26 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/******************************************************************************
   Filename: DP_Precip_8bit_packet16Dig.c

   Description
   ===========
      packet16_dig() packs two (2) 256-level radial data per (I*2) halfword of
   the product buffer. It will builds the graphic for 8-bit products: DAA,
   DSA, DOD, and DSD.

   Inputs:
      short*        prodbuf    - pointer to start of product buffer
      unsigned char dataScaled - array of scan to scan data

   Outputs:
      short*        prodbuf    - product buffer with digital radial data

   Returns:
      The Layer length.

   20090325 Ward Attempted to speed up dp_precip_8bit by eliminating the
   Packet16_data_t middleman with a direct call to dataScaled[rad][bin]:

         nbytes = RPGP_set_packet_16_radial((unsigned char*) (ptr + offset),
                                            (short) (rad * SCALE_FACTOR),
                                            (short) DELTA_ANGLE,
                                            &(dataScaled[rad][bin]),
                                            (int) MAX_BINS);

   This ran under -g, but core dumped under -0. I suspect that
   RPGP_set_packet_16_radial() is adding an extra alignment byte onto
   dataScaled[rad][bin], am restoring the working version pending further
   investigation.

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----        -------    ----------         -----
   10/07       0000       Pham,Ward,Stein    Initial Implementation
******************************************************************************/

/* Note: The inclusion order avoids warnings. The function prototype for
 * RPGP_set_packet_16_radial() is in ~/include/rpgp.h, which includes:
 *
 * rpc/xdr.h which includes:
 *
 * rpc/types.h which defines:
 *
 *  FALSE as (0)
 *  TRUE  as (1)
 *
 * note the parenthesis. If you put rpgp.h first in the include chain,
 * you will get a warning about TRUE being redefined. */

#include "DP_Precip_8bit_func_prototypes.h"
#include "dp_precip_8bit_types.h"             /* Packet16_hdr_t            */
#include <rpgp.h>                             /* RPGP_set_packet_16_radial */

#define SYMB_OFFSET 120 /* symbology block offset */

unsigned int packet16_dig(short* prodbuf, unsigned char dataScaled[][MAX_BINS])
{
   Packet16_hdr_t  packet16_hdr;
   Packet16_data_t packet16_data;
   Sym_block_t sym;
   char msg[200];  /* stderr message */

   char *ptr = (char *) prodbuf;

   int rad, bin;
   int offset = 0;
   int nbytes;
   unsigned int block_len;

   if ( DP_PRECIP_8BIT_DEBUG )
      fprintf(stderr,"\nBeginning packet16_dig() ...\n");

   /* Fill in Product Symbology Block Header: */

   sym.divider  = (short) -1;
   sym.block_id = (short)  1;

   /* Block length = symbology block size (10) +
    *                layer header size    ( 6) +
    *                packet header size   (14) +
    *                ((MAX_BINS+6) * MAX_AZM)     */

   block_len = (int) (30 + ((MAX_BINS + 6) * MAX_AZM));
   RPGC_set_product_int( (void *) &sym.block_length, block_len );
   sym.num_layers = (short) 1;
   sym.divider2   = -1;

   /* layer length = Block length -
    *                (symbology block size (10) +
    *                 layer header size    ( 6))  */

   RPGC_set_product_int( (void *) &sym.layer_length,
                         (unsigned int) (block_len - 16));

   /* Copy the symbology block into the output buffer */

   memcpy (ptr + SYMB_OFFSET, &sym, 16);

   /* Fill in packet 16 layer header */

   packet16_hdr.pcode           = PACKET_16; /* packet 16                  */
   packet16_hdr.first_range_bin = 0;         /* dist to the 1st range bin  */
   packet16_hdr.num_bins        = MAX_BINS;  /* num of bins in each radial */
   packet16_hdr.icenter         = ICENTER;   /* i center of display        */
   packet16_hdr.jcenter         = JCENTER;   /* j center of display        */
   packet16_hdr.scale_factor    = RANGE_SCALE_FACT; /* scale factor        */
   packet16_hdr.num_radials     = MAX_AZM;   /* num radials included       */

   /* Load the packet layer header into the output buffer. *
    *                                                      *
    * offset = 120 + 16 = 136                              *
    * layer header size = 14                               */

   memcpy (ptr + SYMB_OFFSET + 16, &packet16_hdr, 14);

   /* Set offset to beginning of data layer, 150 bytes = 120 + 16 + 14. */

   offset = 150;

   /* Load the accumulation layer data and process for each radial.
    * the start_angle is scaled by a factor of 10. */

   for ( rad = 0; rad < MAX_AZM; rad++ )
   {
      packet16_data.num_bytes   = (short) MAX_BINS;             /* 920 */
      packet16_data.start_angle = (short) (rad * SCALE_FACTOR); /*  10 */
      packet16_data.delta_angle = (short) DELTA_ANGLE;          /*  10 */

      /* Do for all sample bins: */

      for ( bin = 0; bin < MAX_BINS; bin++ )
      {
         packet16_data.data[bin] = dataScaled[rad][bin];
      } /* end bin loop */

      /* RPGP_set_packet_16_radial() function assumes data is passed as  *
       * unsigned BYTE values and data starts at index 0.                */

      nbytes = RPGP_set_packet_16_radial((unsigned char*) (ptr + offset),
                                         (short) packet16_data.start_angle,
                                         (short) packet16_data.delta_angle,
                                         packet16_data.data,
                                         (int) MAX_BINS);
      if(nbytes != MAX_BINS)
      {
         sprintf(msg,
                 "packet16_dig: nbytes %d != MAX_BINS %d\n",
                 nbytes, MAX_BINS);

         RPGC_log_msg(GL_INFO, msg);
         if(DP_PRECIP_8BIT_DEBUG)
            fprintf(stderr, msg);
      }

      /* Increment the offset by 926 = 6 byte radial header + 920 bins */

      offset += (6 + MAX_BINS);

   } /* End loop over all radials */

   /* Return the total length of the product minus the Graphic_product *
    * structure length which is added back by the RPG_prod_hdr()       */

   return ((unsigned int) offset - 120);

} /* end packet16_dig() ====================================== */
