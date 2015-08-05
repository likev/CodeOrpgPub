/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:14:53 $
 * $Id: cvt_packet_6.c,v 1.1 2002/08/30 16:14:53 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* cvt_packet_6.c */

#include "cvt_packet_6.h"

void packet_6(char *buffer,int *offset) {
  /* display information contained within packet 6 */
  short length;
  int i,num_vectors,count=1;

  printf("\nPacket 6: Linked Vector Packet (no value)\n");

  length=read_half(buffer,offset);
  printf("Packet 6 Length of Data Block (in bytes) = %hd\n",length);
     
  printf("I Starting Point: %hd   J Starting Point: %hd\n",
    read_half(buffer,offset),read_half(buffer,offset));

  length-=4; /* account for the starting points */
  num_vectors=length/4;
  printf("Number of Vectors: %i\n",num_vectors);
      
  for(i=0;i<num_vectors;i++) {
    printf("End Vector Number %4d   I=%5hd  J=%5hd\n",count,
      read_half(buffer,offset),read_half(buffer,offset));
    count++;
    }              
       
  printf("Packet 6 Complete\n");
  }


