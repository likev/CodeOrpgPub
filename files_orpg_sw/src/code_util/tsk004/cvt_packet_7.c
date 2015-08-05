/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 18:18:34 $
 * $Id: cvt_packet_7.c,v 1.2 2003/02/06 18:18:34 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* cvt_packet_7.c */

#include "cvt_packet_7.h"


void packet_7(char *buffer,int *offset) {
  /* display information contained within packet 7 */
  short length; /* color_level; */
  int i,num_vectors;

  printf("\nPacket 7: Unlinked Vector Packet (No Value)\n");

  length=read_half(buffer,offset);
  printf("Packet 7 Length of Data Block (in bytes) = %hd\n",length);
     
  /*length-=2;  account for the color&length values */
  num_vectors=length/8;
  printf("Number of Vectors to Plot: %d\n",num_vectors);
  for(i=0;i<num_vectors;i++) {
    short begI,begJ,endI,endJ;
    
    begI=read_half(buffer,offset);
    begJ=read_half(buffer,offset);
    endI=read_half(buffer,offset);
    endJ=read_half(buffer,offset);
    
    printf("Vector %04d: BegI %5hd BegJ %5hd EndI %5hd EndJ %5hd\n",
      i+1,begI,begJ,endI,endJ);
    }
       
  printf("Packet 7 Complete\n");
  }


