/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/02/07 17:29:27 $
 * $Id: cvt_packet_20.c,v 1.2 2003/02/07 17:29:27 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* cvt_packet_20.c */

#include "cvt_packet_20.h"


void packet_20(char *buffer,int *offset) {
    /* display information contained within packet 3 */
    short length,ipos,jpos,type,attribute;
    int i,num=0;

    printf("\nPacket 20: Generic Point Feature\n");

    length=read_half(buffer,offset);
    num=length/8;    
    printf("Packet 20: Length=%4hd  Number Included=%hd\n",
      length,num);
      

    
    /* in this packet there are 4 fields (8 bytes) to be 
    written for each symbol */
    
    for(i=0;i<num;i++) {
      ipos=read_half(buffer,offset);
      jpos=read_half(buffer,offset);
      type=read_half(buffer,offset);
      attribute=read_half(buffer,offset);
      
      printf("  I Pos: %4hd  J Pos: %4hd  Feature Type: %hd  Attribute: %hd\n",
         ipos,jpos,type,attribute);
         
      }
    printf("\n");
   
  }


