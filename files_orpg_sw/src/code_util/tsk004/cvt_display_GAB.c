/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 18:23:14 $
 * $Id: cvt_display_GAB.c,v 1.2 2003/02/06 18:23:14 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* cvt_display_GAB.c */

#include "cvt_display_GAB.h"


void display_GAB(char *buffer,int *offset) {
  /* display information contained within the
   Graphic Alphanumeric Block */
/*  short length; */
  int page,numpages,process,gab_off=0,stopval=0; /* i; */
  int block_length=0; 
  short current_page,page_length; /* code; */
  short val;
  int TEST=FALSE;

  /* move offset to beginning of GAB block */
  printf("\nGraphic Alphanumeric Block\n");

  gab_off=2*md.gab_offset;
  if(TEST) {
   printf("Offset value = %d  2*gab_offset=%d\n",
   *offset,gab_off);
   }
 
  printf("Block Divider = %hd\n",read_half(buffer,offset));
  printf("Block ID =      %hd\n",read_half(buffer,offset));
  block_length=read_word(buffer,offset);
  printf("Block Length =  %d bytes\n",block_length);

  numpages=read_half(buffer,offset);
  printf("Number of Pages to follow %hd\n",numpages);

  /* repeat for each page */
  for(page=1;page<=numpages;page++) {
    process=TRUE;
    current_page=read_half(buffer,offset);
    page_length=read_half(buffer,offset);
    
    stopval=*offset+page_length;
    printf("\nCurrent GAB Page: %d\n",current_page);
    
    if(TEST) {
    printf("Current GAB Page: %hd  Length: %hd bytes  Current Offset=%x  Stop Offset=%x\n",
      current_page,page_length,*offset,stopval);
      }
   
    while(*offset<stopval) {
   
      val=read_half(buffer,offset);
      if(TEST) {
            printf("GAB Value=%hd at offset+%d\n",val,*offset);
            }
      
      if(val==8)
         packet_8(buffer,offset);
      else if(val==10)
         packet_10(buffer,offset);
      else
         return;
      
      if(TEST) {
         printf("Returned Offset=%d\n",*offset);
         }
      } /* end while process */

    } /* end for page */

  printf("\n");
  printf("GAB Message Complete\n");
  }


