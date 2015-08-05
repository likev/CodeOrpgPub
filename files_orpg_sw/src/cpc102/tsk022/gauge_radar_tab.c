/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/04/13 22:53:07 $
 * $Id: gauge_radar_tab.c,v 1.3 2011/04/13 22:53:07 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <rpgc.h>              /* Graphic_product */
#include "gauge_radar_types.h" /* alpha_data_t    */
#include "gauge_radar_common.h"

/******************************************************************************
   Filename: gauge_radar_tab.c

   Description
   ===========
      add_tab_str() adds a string to a TAB page without any special format.

   Input: int   num_leading_spaces - number of leading spaces
          char* str                - string to add (max 80 chars)
          char* buffer_end         - end of the current output buffer

   Output:

   Returns: Pointer to the new output buffer end

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ----------         ----------------------
   20091215    0000       James Ward         Initial version
******************************************************************************/

char* add_tab_str(int num_leading_spaces, char* str, char* buffer_end)
{
   char line[81];
   int  i, len;

   static unsigned int ad_size = sizeof(alpha_data_t);

   alpha_data_t alpha_data; /* output structure */

   /* Note: line is a convenience array for alpha_data.data.
    * alpha_data.data is not really a string, but an array
    * of characters, filled out to 80, with no trailing NULL */

   memset(line, 0, 81);

   /* Add leading spaces */

   for(i=0; i<num_leading_spaces; i++)
      line[i] = ' ';

   len = strlen(line);

   /* Add the string */

   if(str != NULL)
   {
      sprintf(&(line[len]), "%s", str);

      len = strlen(line);
   }

   /* Pad with spaces out to 80. */

   for(i = len; i < 80; i++)
      line[i] = ' ';

   /* Fill alpha_data structure */

   alpha_data.num_char = 80;

   strncpy(alpha_data.data, line, 80);

   /* Copy to output buffer */

   memcpy(buffer_end, &alpha_data, ad_size);

   /* Return new buffer end */

   return(buffer_end + ad_size);

} /* end add_tab_str() ===================================================== */

/******************************************************************************
   Filename: gauge_radar_tab.c

   Description
   ===========
      tab_page_end() does end of TAB page processing

   Input: int   page_num   - page number
          char* buffer_end - end of the current output buffer

   Output:

   Returns: Pointer to the new output buffer end

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ----------         ----------------------
   20091215    0000       James Ward         Initial version
******************************************************************************/

char* tab_page_end(int page_num, char* buffer_end)
{
   short end_flag = (short) -1; /* 0xFFFF = TAB end of page flag */

    /* Copy to output buffer */

   memcpy(buffer_end, &end_flag, sizeof(short));

   return(buffer_end + sizeof(short));

} /* end tab_page_end() ===================================================== */

/******************************************************************************
   Filename: gauge_radar_tab.c

   Description
   ===========
      gauges_tab() is the main generation module for the creation of
   the tabular alphanumeric block which is appended to product 314.
   The block is for demonstration purposes.

   Input: char* tab_start - start of tab block in output buffer

   Output: outbuf with TAB block filled in.

   Returns: Size of tab block

   Change History
   ==============
   DATE        VERSION    PROGRAMMER   NOTES
   --------    -------    ----------   ---------------------------------
   20091211     0000      Ward         Initial version
   20100119     0001      Zhan Zhang  
******************************************************************************/

int gauges_tab(char* tab_start, 
               char header[HEADER_LEN],
               char messages[NUM_MSGS][MSG_LEN])
{
   char*               buffer_end = NULL; /* end of entire buffer         */
   TAB_header_t        tab_hdr;           /* TAB header structure         */
   int                 tab_size = 0;      /* TAB size in bytes            */
   alpha_header_t      alpha_hdr;         /* alpha block header structure */

   static unsigned int graphic_size   = sizeof(Graphic_product);
   static unsigned int tab_hdr_size   = sizeof(TAB_header_t);
   static unsigned int alpha_hdr_size = sizeof(alpha_header_t);
  
   int i;

   /* Our product buffer will look like:
    *
    * ----- Already filled in -----
    * 1. Main message header         (120 bytes)
    * 2. Symbology block             (16+ bytes)
    * ----- Start of TAB -----
    * 3. TAB header                  (  8 bytes)
    * 4. Copy of main message header (120 bytes)
    * ----- TAB data -----
    * 5. Alpha header                (  4 bytes)
    * 6. Alpha data */

   /* Skip past the TAB header and the copy of the main message header */

   buffer_end = tab_start + tab_hdr_size + graphic_size;

   /* Step 5 - Fill in the alpha header.
    * The TAB is divided into pages, we are fitting all our data into page 1. */

   alpha_hdr.divider   = (short) -1;
   alpha_hdr.num_pages = 1; /*update this */

   memcpy(buffer_end, &alpha_hdr, alpha_hdr_size);

   buffer_end += alpha_hdr_size;

   /* Step 6 - Fill in the (page 1) alpha data
    *          Each line will be padded to 80 characters */

   buffer_end = add_tab_str(0, header, buffer_end);

   for(i = 0; i < NUM_MSGS; i++)
       buffer_end = add_tab_str(0, &(messages[i][0]), buffer_end);
          
   /* Every TAB page needs an end */

   buffer_end = tab_page_end(1, buffer_end);

   /* Step 3 - Go back and fill in the TAB header */

   tab_hdr.divider  = (short) -1;
   tab_hdr.block_id = (short)  3;

   tab_size = buffer_end - tab_start;

   RPGC_set_product_int(&(tab_hdr.block_length), tab_size);

   memcpy(tab_start, &tab_hdr, tab_hdr_size);

   return(tab_size);

} /* end gauges_tab() ================================== */
