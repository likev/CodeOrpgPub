/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 18:19:28 $
 * $Id: cvt_packet_4.c,v 1.2 2003/02/06 18:19:28 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
 
/* cvt_packet_4.c */

#include "cvt_packet_4.h"


void packet_4(char *buffer,int *offset) {
  /* display information contained within packet 4 */
  short length,color_level,x,y,dir,spd;
/*  int i,num_vectors; */

  printf("\nPacket 4: Wind Barb Data Packet\n");

  length=read_half(buffer,offset);
  /*   printf("Packet 4 Length of Data Block (in bytes) = %hd\n",length);*/
  color_level=read_half(buffer,offset);
  /*printf("Color Level of Wind Barb (1-5): %hd\n",color_level);*/
  if(color_level<1 || color_level>5) {
      printf("Data Error: Color level of %hd was out of range\n",color_level);
      return;
      }
   
   x=read_half(buffer,offset);
   y=read_half(buffer,offset);
   dir=read_half(buffer,offset);
   spd=read_half(buffer,offset);
   printf("Packet 4: Length=%4hd  Barb Color=%3hd  X Pos: %4hd  Y Pos: %4hd  Dir: %3hd  Speed: %3hd\n",
      length,color_level,x,y,dir,spd);
   
     /*
     printf("Wind Barb Data: X=%hd Y=%hd  Dir=%hd  Speed=%hd\n",
        read_half(buffer,offset),read_half(buffer,offset),
        read_half(buffer,offset),read_half(buffer,offset));
     
  printf("\n");*/
  }


