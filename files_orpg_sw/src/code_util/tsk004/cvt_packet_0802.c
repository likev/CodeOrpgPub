/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 18:22:21 $
 * $Id: cvt_packet_0802.c,v 1.2 2003/02/06 18:22:21 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/* cvt_packet_0802.c */

#include "cvt_packet_0802.h"

void packet_0802(char *buffer,int *offset) {
  /* display information contained within packet 0802 */
/*  short length; */
/*  int i,start_offset=*offset,end_offset=0; */
/*  int process=TRUE,code; */

  printf("\nPacket 0x0802: Contour Vector Color Levels\n");
  printf("Color Value Indicator             %x\n",read_half(buffer,offset));
  printf("Color Level of Contour            %hd\n",read_half(buffer,offset));

  printf("\n");
  }


