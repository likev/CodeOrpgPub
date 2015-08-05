/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:14:27 $
 * $Id: cvt_packet_26.c,v 1.1 2002/08/30 16:14:27 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* cvt_packet_26.c */

#include "cvt_packet_26.h"


void packet_26(char *buffer,int *offset) {
  /* display information contained within packet 26 */
  short length;
  int i,num=0;

    printf("\nPacket 26: Elevated Tornado Vortex Signature\n");

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


