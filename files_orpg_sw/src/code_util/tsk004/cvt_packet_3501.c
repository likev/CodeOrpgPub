/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 17:51:52 $
 * $Id: cvt_packet_3501.c,v 1.2 2003/02/06 17:51:52 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* cvt_packet_3501.c */

#include "cvt_packet_3501.h"


void packet_3501(char *buffer,int *offset) {
  /* display information contained within packet 3501 */
  short length;
  short I_start,J_start,I_end,J_end;
  int i,count=1;

  printf("\nPacket 0x3501: Linked Vectors\n");

  length=read_half(buffer,offset);         /* Length of Vectors (multiples of 4)     */
  
  printf("Length of Data Block (in bytes) = %hd or (0x%04x)  Number of Vectors: %d\n",length,
    length,length/8);

  for(i=0;i<=length/8;i++) {
    
    I_start=read_half(buffer,offset);        /* I Coordinate for vector starting point */
    J_start=read_half(buffer,offset);        /* J Coordinate for vector starting point */    
    I_end=read_half(buffer,offset);          /* I Coordinate for vector end point      */
    J_end=read_half(buffer,offset);          /* J Coordinate for vector end point      */

    printf("I Vector %4i   Start: %hd (0x%04x)   End: %hd (0x%04x)\n",count,I_start,I_start,
      I_end,I_end);
    printf("J Vector %4i   Start: %hd (0x%04x)   End: %hd (0x%04x)\n",count,J_start,J_start,
      J_end,J_end);
    
    count++;
    }

  printf("packet 0x3501 complete\n");
  }


