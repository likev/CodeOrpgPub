/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:53 $
 * $Id: cvt_packet_9.c,v 1.3 2009/05/15 17:37:53 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/* cvt_packet_9.c */

#include "cvt_packet_9.h"


void packet_9(char *buffer,int *offset) {
  /* display information contained within packet 9 */
  short length; /* value; */
  int i,num_vectors,count=1;

  printf("\nPacket 9: Linked Vector Packet (uniform value)\n");
  printf("WARNING: Not yet validated with product data\n");

  length=read_half(buffer,offset);
  printf("Packet 9 Length of Data Block (in bytes) = %hd\n",length);
     
  printf("Value (Level) of Vector: %hd\n",read_half(buffer,offset));        
     
  printf("I Starting Point: %hd   J Starting Point: %hd\n",
    read_half(buffer,offset),read_half(buffer,offset));
/*CVT 4.4.1 - CORRECT PARSE ERROR */
/*  length-=3;            */ /* account for the starting points & value */
/*  num_vectors=length/2; */
  length-=4; /* account for the starting points & value */
  num_vectors=length/4;
    
  printf("Number of Vectors: %i\n",num_vectors);
      
  for(i=0;i<num_vectors;i++) {
    printf("End Vector Number %d   I=%hd  J=%hd\n",count,
      read_half(buffer,offset),read_half(buffer,offset));
    count++;
    }              
       
  printf("Packet 9 Complete\n");
  }


