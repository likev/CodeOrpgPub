/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:13:27 $
 * $Id: cvt_packet_0E03.c,v 1.1 2002/08/30 16:13:27 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/* cvt_packet_0E03.c */

#include "cvt_packet_0E03.h"


void packet_0E03(char *buffer,int *offset) {
  /* display information contained within packet 0E03 */
  short length,initial_point;
  short I_start,J_start;
  int i,count=1;

  printf("\nPacket 0x0E03: Linked Vectors\n");

  initial_point=read_half(buffer,offset);  /* read initial point indicator           */
  I_start=read_half(buffer,offset);        /* I Coordinate for vector starting point */
  J_start=read_half(buffer,offset);        /* J Coordinate for vector starting point */
  
  length=read_half(buffer,offset);         /* Length of Vectors (multiples of 4)     */
  
  printf("Initial Point Indicator: %x\n",(unsigned short)initial_point);
  printf("I Vector Start Position: %hd or 0x%04x\n",I_start,I_start);
  printf("J Vector Start Position: %hd or 0x%04x\n",J_start,J_start);  
  printf("Length of Data Block (in bytes) = %hd or (0x%04x)  Number of Vectors: %d\n",length,
   length,length/4);

  printf("Listing of Linked Vectors to follow:\n");
  length-=2;

  for(i=0;i<=length/4;i++) {
    short EndI1,EndJ1;
  
    EndI1=read_half(buffer,offset);
    EndJ1=read_half(buffer,offset);
    printf("Vector %04d: End I=%5hd  End J=%5hd\n",count,EndI1,EndJ1);
    count++;
    }

  printf("packet 0x0E03 complete\n");
  }


