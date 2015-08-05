/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:13:42 $
 * $Id: cvt_packet_12.c,v 1.1 2002/08/30 16:13:42 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* cvt_packet_12.c */

#include "cvt_packet_12.h"


void packet_12(char *buffer,int *offset) {
  /* display information contained within packet 12 */
  short length,ipos,jpos;
  int i,num=0;

    printf("\nPacket 12: Tornado Vortex Signature\n");

    length=read_half(buffer,offset);
    num=length/4;    
    
    printf("TVS Block Length %hd  Number Included %hd\n",length,num);
    
    /* in this packet there are 2 fields (4 bytes) to be 
    written for each symbol */
    
    for(i=0;i<num;i++) {
      
      ipos=read_half(buffer,offset);
      jpos=read_half(buffer,offset);
      printf("  I Pos: %4hd  J Pos: %4hd\n",ipos,jpos);
      /*
      printf("I Position: %hd\n",read_half(buffer,offset));
      printf("J Position: %hd\n",read_half(buffer,offset));
      */
      }
    printf("\n");
   
  }


