/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2004/03/03 17:59:58 $
 * $Id: cvt_packet_1.c,v 1.3 2004/03/03 17:59:58 cheryls Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/* cvt_packet_1.c */

#include "cvt_packet_1.h"


void packet_1(char *buffer,int *offset) {
  /* display information contained within packet 1 */
  short length;
  int i; /* start_offset=*offset,end_offset=0; */
  int j;

    printf("\n\nPacket 1: Write Text (No Value) Summary Information\n");

    length=read_half(buffer,offset);
    printf("Length of Data Block (in bytes) = %hd\n",length);
    printf("I Starting Point:                 %hd\n",read_half(buffer,offset));
    printf("J Starting Point:                 %hd\n",read_half(buffer,offset));
    printf("Message to follow:\n");
    
    length-=4;
    j=0;
    
    for(i=0;i<length;i++) {
      unsigned char c;
      
      c=read_byte(buffer,offset);
      printf("%c",c);
      /* Precip Products have long strings intended to be 80 chars per line */
      if(j==79) {
          printf("\n");
          j=0;
      } else {
           j++;
      }
    }

    printf("\n");
   
  }


