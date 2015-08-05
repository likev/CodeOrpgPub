/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/06/26 18:22:32 $
 * $Id: dp_packet.c,v 1.2 2009/06/26 18:22:32 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#include "dp_lib_func_prototypes.h"
#include "dp_lt_accum_Consts.h"     /* SECS_PER_HOUR */

/******************************************************************************
    Filename: dp_packet.c

    Description:
    ============
    make_null_symbology_block() makes a null product as layer 1 of a symbology
    block.

    Inputs: char*           outbuf         - the output buffer
            int             null_indicator - the type of null product,
                                             used to generate the message.
            char*           prodname       - for screen message

    Outputs: A null symbology block in the output buffer

    Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

    Called by: DUA write_to_output_product(), Build_DAA_product(),
               Build_DSA_product(), Build_DSD_product(), Build_OHA_product(),
               Build_STA_product().

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    -----------   -------    ---------------    ----------------------
    29 Apr 2008    0000      Ward               Initial implementation
******************************************************************************/

int make_null_symbology_block(char* outbuf, int null_indicator, char* prodname,
                              int restart_time, time_t last_time_prcp, 
                              int rain_time_thresh)
{
   char  text[MAX_PACKET1 + 1];
   short i_start, j_start;
   short row_increment   = 7; /* pixels to move down per row */
   int   data_layer_len  = 0;
   int   num_bytes_added = 0;
   int   year            = 0;
   int   month           = 0;
   int   day             = 0;
   int   hour            = 0;
   int   minute          = 0;
   int   second          = 0;
   char  msg[200];            /* stderr message */
   char*            layer1_ptr = NULL;
   Symbology_block* sym_ptr    = NULL;

   static unsigned int graphic_size = sizeof(Graphic_product);

   /* Check for NULL pointers */

   if(pointer_is_NULL(outbuf, "make_null_symbology_block", "outbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(prodname, "make_null_symbology_block", "prodname"))
      return(NULL_POINTER);

   sym_ptr    = (Symbology_block*) (outbuf + graphic_size);
   layer1_ptr = (char*) sym_ptr;

   /* The Symbology_block includes both the layer divider and the data_len
    * for the first layer. */

   sym_ptr->divider       = (short) -1; /* 2 bytes */
   sym_ptr->block_id      = (short)  1; /* 2 bytes */

   /* We will fill in the (4 byte) sym_ptr->block_len later */

   sym_ptr->n_layers      = (short)  1; /* 2 bytes */
   sym_ptr->layer_divider = (short) -1; /* 2 bytes */

   /* We will fill in the (4 byte) sym_ptr->data_len length later */

   layer1_ptr += 16; /* 16 = 2 + 2 + 4 + 2 + 2 + 4 bytes */

   /* Add the packet 1 text. The DSA layer 2 j_starts at row 20. */

   i_start = 7;
   j_start = 9;

   /* The text depends on the null_indicator. CVG appears to wrap lines
    * after 80 characters, so we try to keep ours under 80 in case AWIPS
    * does also. */

   if(null_indicator == NULL_REASON_1)
   {
      sprintf(text,
             "No accumulation available.");

      num_bytes_added = add_packet_1(layer1_ptr, i_start, j_start, text);
      if(num_bytes_added == NULL_POINTER)
         return(NULL_POINTER);

      data_layer_len += num_bytes_added;
      layer1_ptr     += num_bytes_added;

      j_start += row_increment;

      /* Default restart_time: 60 mins */

      sprintf(text,
         "Threshold: 'Elapsed Time to Restart' [TIMRS] (%d minutes) exceeded",
          restart_time);

      num_bytes_added = add_packet_1(layer1_ptr, i_start, j_start, text);
      if(num_bytes_added == NULL_POINTER)
         return(NULL_POINTER);

      data_layer_len += num_bytes_added;
   }
   else if(null_indicator == NULL_REASON_2)
   {
      sprintf(text,
             "No precipitation detected during the specified time span");

      num_bytes_added = add_packet_1(layer1_ptr, i_start, j_start, text);
      if(num_bytes_added == NULL_POINTER)
         return(NULL_POINTER);

      data_layer_len += num_bytes_added;
   }
   else if(null_indicator == NULL_REASON_3)
   {
      sprintf(text,
             "No accumulation data available for the specified time span");

      num_bytes_added = add_packet_1(layer1_ptr, i_start, j_start, text);
      if(num_bytes_added == NULL_POINTER)
         return(NULL_POINTER);

      data_layer_len += num_bytes_added;
   }
   else if(null_indicator == NULL_REASON_4)
   {
      if(last_time_prcp > 0)
      {
         RPGCS_unix_time_to_ymdhms(last_time_prcp,
                                   &year, &month, &day,
                                   &hour, &minute, &second);
         sprintf(text,
                "No precipitation detected since %d/%d/%d %2.2d:%2.2d Z.",
                 month, day, year, hour, minute);
      }
      else
      {
         sprintf(text, "No precipitation detected since RPG startup.");
      }

      num_bytes_added = add_packet_1(layer1_ptr, i_start, j_start, text);
      if(num_bytes_added == NULL_POINTER)
         return(NULL_POINTER);

      data_layer_len += num_bytes_added;
      layer1_ptr     += num_bytes_added;

      j_start += row_increment;

      sprintf(text,
         "Threshold: 'Time Without Precipitation for Resetting Storm Totals'");

      num_bytes_added = add_packet_1(layer1_ptr, i_start, j_start, text);
      if(num_bytes_added == NULL_POINTER)
         return(NULL_POINTER);

      data_layer_len += num_bytes_added;
      layer1_ptr     += num_bytes_added;

      j_start += row_increment;

      /* Default rain_time_thresh: 60 mins */

      sprintf(text,
             "   [RAINT] is %d minutes",
              rain_time_thresh);

      num_bytes_added = add_packet_1(layer1_ptr, i_start, j_start, text);
      if(num_bytes_added == NULL_POINTER)
         return(NULL_POINTER);

      data_layer_len += num_bytes_added;
   }
   else if(null_indicator == NULL_REASON_5)
   {
      if(last_time_prcp > 0)
      {
         RPGCS_unix_time_to_ymdhms(last_time_prcp,
                                   &year, &month, &day,
                                   &hour, &minute, &second);
         sprintf(text,
                "No precipitation detected since %d/%d/%d %2.2d:%2.2d Z",
                 month, day, year, hour, minute);
      }
      else
      {
         sprintf(text, "No precipitation detected since RPG startup");
      }

      num_bytes_added = add_packet_1(layer1_ptr, i_start, j_start, text);
      if(num_bytes_added == NULL_POINTER)
         return(NULL_POINTER);

      data_layer_len += num_bytes_added;
   }
   else /* bad null indicator - still make the product */
   {
      sprintf(text,
             "Product unavailable - unknown reason %d",
              null_indicator);

      num_bytes_added = add_packet_1(layer1_ptr, i_start, j_start, text);
      if(num_bytes_added == NULL_POINTER)
         return(NULL_POINTER);

      data_layer_len += num_bytes_added;
   }

   /* Write the length of the data layer to the product as an int (4 bytes).
    * According to the Code Manual, Vol. 2, p. 74, the length of
    * the data layer includes everything AFTER the length of
    * the data layer. */

   RPGC_set_product_int ((void *) &(sym_ptr->data_len), data_layer_len);

   /* Set the block length of the symbology block to account for all
    * the layer1 we've just written. The block length includes everything
    * in the symbology block, INCLUDING the divider and block_id. */

   RPGC_set_product_int ((void *) &(sym_ptr->block_len), 16 + data_layer_len);

   /* Write something out, so you can tell when a null product was made */

   sprintf(msg, "Made a null %s\n", prodname);
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LIB002_DEBUG)
      fprintf(stderr, msg);

   return(FUNCTION_SUCCEEDED);

} /* end make_null_symbology_block() ===================================== */

/******************************************************************************
    Filename: dp_packet.c

    Description:
    ============
    add_packet_1 () adds a packet 1 to a buffer. It follows the convention
    that 1 line of text goes to one packet 1.

    Inputs: char* buffer  - where to place the text
            short i_start - i starting point
            short j_start - j starting point
            char* text    - text to place

    Outputs: int number of bytes added

    According to Tom Ganger (20080415):

    1. For non-geographic products, the i and j coordinates were always
       screen pixels (with 0:0 being the upper left of the display screen)
       on non-geographic products. These are products that do not have a
       look-down onto a geographic surface (cross sections, etc).

    2. For geographic products, the i and j coordinates are always 1/4 km
       screen coordinates which are relative to the radar location
       (normally in the center of the screen).

    The text usually does not have a carriage return or line feeds in it,
    and is normally less then 80 characters long, but this is not required.

    See also Figure 3-8b. Text and Special Symbol Packets - Packet Code 1
    (Sheet 1) in the Class I User ICD.

    According to Brian Klein (20090611):

    I believe the traditional character size (i_start, j_start), as defined 
    in the Product Spec ICD was 7 by 9 pixels, 5 by 5 seems pretty small.

    Returns: number of bytes added, or NULL_POINTER (2)

    Called by: append_ascii_layer2(), make_null_symbology_block() 

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    -----------   -------    ---------------    ----------------------
    29 Apr 2008    0000      Ward               Initial implementation, based
                                                upon code written by Cham Pham
                                                for the DSA layer2.
******************************************************************************/

int add_packet_1 (char* buffer, short i_start, short j_start, char* text)
{
   int    num_bytes = 0;
   int    textlen;
   short* short_ptr = NULL;

   /* Check for NULL pointers */

   if(pointer_is_NULL(buffer, "add_packet_1", "buffer"))
      return(NULL_POINTER);

   if(pointer_is_NULL(text, "add_packet_1", "text"))
      return(NULL_POINTER);

   short_ptr = (short *) buffer;

   /**** Write packet header ****/

   *short_ptr = (short) 1; /* Write the packet code (1) */
   ++short_ptr;            /* Move up 1 short           */
   num_bytes += 2;

   /* According to the ICD, p. 3-95, the length of the data block (bytes)
    * does not include the packet code, or the length of the data block */

   textlen = strlen(text);

   /* It appears that you must fill out the text to a full halfword (I didn't
    * see this explicit in the ICD). Otherwise you'll see an error like:
    *
    * UMC: Unrecognized Display Data Packet 172 in Product 21504
    * -orpgumc.c:2024 */

   if((textlen % 2) == 1) /* not a full halfword */
   {
      strcat(text, " ");
      textlen++;
   }

   *short_ptr = (short) (textlen + 4); /* Write the length of the data block */
   ++short_ptr;                        /* Move up 1 short                    */
   num_bytes += 2;

   /**** Write packet data block ****/

   *short_ptr = i_start;  /* Write the i starting point */
   ++short_ptr;           /* Move up 1 short            */
   num_bytes += 2;

   *short_ptr = j_start;  /* Write the j starting point */
   ++short_ptr;           /* Move up 1 short            */
   num_bytes += 2;

   strncpy((char*) short_ptr, text, textlen);
   num_bytes += textlen;

   return(num_bytes);

} /* end add_packet_1() ===================================== */
