/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 18:03:42 $
 * $Id: cvt_packet_3.c,v 1.2 2003/02/06 18:03:42 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* cvt_packet_3.c */

#include "cvt_packet_3.h"


void packet_3(char *buffer,int *offset) {
    /* display information contained within packet 3 */
    short length,ipos,jpos,radius;
    int i,num=0;

    printf("\nPacket 3: Mesocyclone Packet\n");

    length=read_half(buffer,offset);
    num=length/6;    
    printf("Packet 3: Length=%4hd  Number Included=%hd\n",
      length,num);
      
    
    /* in this packet there are 3 fields (6 bytes) to be 
    written for each symbol */
    
    for(i=0;i<num;i++) {
      ipos=read_half(buffer,offset);
      jpos=read_half(buffer,offset);
      radius=read_half(buffer,offset);    	   	
      printf("  I Pos: %4hd  J Pos: %4hd  Meso Radius: %hd\n",
         ipos,jpos,radius);
         /*
      printf("I Position: %hd\n",read_half(buffer,offset));
      printf("J Position: %hd\n",read_half(buffer,offset));
      printf("Meso Radius: %hd\n",read_half(buffer,offset));
      */
      }
    printf("\n");
   
  }


