/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:36 $
 * $Id: display_GAB.c,v 1.6 2009/05/15 17:52:36 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/* display_GAB.c */

#include "display_GAB.h"

/* display information contained within the Graphic Alphanumeric Block 
 * this is done by plotting all of the packets contained within it,
 * since it is essentially just a wrapper around a bunch of 
 * ICD-compliant packets
*/
void display_GAB(int offset) 
{
  int page, numpages, stopval = 0;
  int block_length = 0, num_gab_pages;
  short current_page, page_length;
  short val, divider, bid;
  short packet_length=0;

  XRectangle  clip_rectangle;

  
  if(verbose_flag)
      printf("Page to output:  %d\n", sd->gab_page);

  /* if the currently selected GAB page is set to '0',
   * that means that we don't want to display it right now
   */
  if(sd->gab_page == 0)
      return;

  /* this line may be obselete now */
  sd->gab_displayed=TRUE;

  XSetForeground(display,gc,black_color);
  XFillRectangle(display,sd->pixmap,gc,0,0,427,103);

  clip_rectangle.x       = 0;
  clip_rectangle.y       = 0;
  clip_rectangle.width   = 427;
  clip_rectangle.height  = 103;
  XSetClipRectangles(display, gc, 0, 0, &clip_rectangle, 1, Unsorted);


  /* CVG 9.0 - should not be required, but temporary work-around */
  XSetForeground(display,gc,white_color);


  /* move offset to beginning of GAB block */
  divider = read_half(sd->icd_product, &offset);
  bid = read_half(sd->icd_product, &offset);
  block_length = read_word(sd->icd_product, &offset);

  
  if(verbose_flag) {
      printf("\nGraphic Alphanumeric Block\n");
      printf("Block Divider = %hd\n", divider);
      printf("Block ID =      %hd\n", bid);
      printf("Block Length =  %d bytes\n",block_length);
  }

  num_gab_pages = numpages = read_half(sd->icd_product,&offset);

  if(verbose_flag)
      printf("Number of Pages to follow %hd\n", numpages);

  /* make sure we have a valid number of pages */
  if( (sd->gab_page > num_gab_pages) || (sd->gab_page < 0) ) {
      if(verbose_flag)
      printf("Invalid GAB Page: %d\n", sd->gab_page);
      sd->gab_page = 1;
  }

  /* this flag is used to do some adjustments to the packets that make up
   * the GAB, since the GAB is special, and our display is a bit
   * different from that of a PUP
   */ 
  printing_gab = TRUE;
  /* repeat for each page */
  for(page=1; page<=numpages; page++) {
    /* get the page header information - the current page number 
     * and how long the page data is
     */
    current_page = read_half(sd->icd_product,&offset);
    page_length = read_half(sd->icd_product,&offset);

    /* stop to display the one page that we want to */
    if(current_page == sd->gab_page) {
        stopval = offset + page_length; /* figure the end of the page data buffer */
    if(verbose_flag)
      printf("Current GAB Page: %hd Length: %hd bytes Current Offset=%x Stop Offset=%x\n",
         current_page,page_length,offset,stopval);
   
      while(offset < stopval) {
      /* read in packet ID */
      val = read_half(sd->icd_product, &offset);
      if(verbose_flag)
          printf("GAB Value=%hd at offset+%d\n",val,offset);
      
      /* display the packet */
      dispatch_packet_type(val, offset-2, TRUE);
      /*LINUX changes */
      /* trying to make it skip over the packet without using */
      /*skip_over_packet(val, &offset, sd->icd_product);*/
      packet_length=read_half(sd->icd_product, &offset);      
      offset+=packet_length;

      
      if(verbose_flag)
          printf("Returned Offset=%d\n",offset);
      } /* end while process */
    } else {  /* if it's not the page we want, skip over it */
      offset += page_length;
    }
  } /* end for page */


  /* restore the full area of the digital canvas for drawing */
  clip_rectangle.x       = 0;
  clip_rectangle.y       = 0;
  clip_rectangle.width   = pwidth + barwidth;
  clip_rectangle.height  = pheight;
  XSetClipRectangles(display, gc, 0, 0, &clip_rectangle, 1, Unsorted);


  /* restore non-GAB output status */
  printing_gab = FALSE;
  
  
  
}
