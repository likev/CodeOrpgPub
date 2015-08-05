/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 18:01:13 $
 * $Id: cvt_packet_11.c,v 1.2 2003/02/06 18:01:13 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
 
/* cvt_packet_11.c */

#include "cvt_packet_11.h"


void packet_11(char *buffer,int *offset) {
  /* display information contained within packet 11 */

  short length,ipos,jpos,radius;
  int i,num=0;

    printf("\nPacket 11: 3D Correlated Shear Packet\n");

    length=read_half(buffer,offset);
  /*    printf("Length of Data Block (in bytes) = %hd\n",length); */
    num=length/6;
    printf("Packet 11: Length=%4hd  Number Included=%hd\n",
      length,num);
   
    /* in this packet there are 3 fields (6 bytes) to be 
    written for each symbol */
    
    for(i=0;i<num;i++) {
      ipos=read_half(buffer,offset);
      jpos=read_half(buffer,offset);
      radius=read_half(buffer,offset);    	
      printf("  I Pos: %4hd  J Pos: %4hd  Meso Radius: %hd\n",
         ipos,jpos,radius);
         
      }
    printf("\n");
   
  }


