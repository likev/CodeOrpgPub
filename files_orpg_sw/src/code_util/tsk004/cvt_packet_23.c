/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 18:21:09 $
 * $Id: cvt_packet_23.c,v 1.2 2003/02/06 18:21:09 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* cvt_packet_23.c */

#include "cvt_packet_23.h"

void packet_23(char *buffer,int *offset) {
  /* display information contained within packet 23 */
  short length;
/*  int i,num=0; */

  printf("\nPacket 23: SCIT Past Position Data\n");
  length=read_half(buffer,offset);
  printf("Length of Data Block (in bytes) = %hd\n",length);
  /* packet codes 2, 6 or 25 follow */
  }


