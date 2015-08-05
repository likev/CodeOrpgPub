/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:14:57 $
 * $Id: cvt_packet_8.c,v 1.1 2002/08/30 16:14:57 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* cvt_packet_8.c */

#include "cvt_packet_8.h"


void packet_8(char *buffer,int *offset) {
  /* display information contained within packet 8 */
  short length;
  int i;
  short ipos,jpos,color;

  printf("\nPacket 8: Write Text (Uniform Value) Summary Information\n");

  length=read_half(buffer,offset);
  /*printf("Packet 8 Length of Data Block (in bytes) = %hd\n",length);*/
  color=read_half(buffer,offset);
  ipos=read_half(buffer,offset);
  jpos=read_half(buffer,offset);
  printf("Packet 8: Length=%4hd  Text Color=%3hd  I Pos: %4hd  J Pos: %4hd\n",
   length,color,ipos,jpos);
  printf("  Text: ");
  /*
  printf("Color Level of Text (0-15): %hd  I Pos: %hd  J Pos: %hd\n",
   read_half(buffer,offset),read_half(buffer,offset),read_half(buffer,offset));
  */
  length-=6; /* account for the color/I/J Pos values */

  for(i=0;i<length;i++) {
    unsigned char c;
    c=read_byte(buffer,offset);
    printf("%c",c);
    }

  printf("\n");
  }


