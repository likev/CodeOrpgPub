/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:13:44 $
 * $Id: cvt_packet_13.c,v 1.1 2002/08/30 16:13:44 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* cvt_packet_13.c */

#include "cvt_packet_13.h"


void packet_13(char *buffer,int *offset) {
  /* display information contained within packet 13 */
  short length;
  int i,num=0;

    printf("\nPacket 13: Hail Positive (Filled)\n");

    length=read_half(buffer,offset);
    printf("Length of Data Block (in bytes) = %hd\n",length);
    num=length/4;
    
    /* in this packet there are 2 fields (4 bytes) to be 
    written for each symbol */
    
    for(i=0;i<num;i++) {
      printf("I Position: %hd\n",read_half(buffer,offset));
      printf("J Position: %hd\n",read_half(buffer,offset));
      }
    printf("\n");
   
  }


