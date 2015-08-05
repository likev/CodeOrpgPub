/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 18:19:06 $
 * $Id: cvt_packet_5.c,v 1.2 2003/02/06 18:19:06 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* cvt_packet_5.c */

#include "cvt_packet_5.h"


void packet_5(char *buffer,int *offset) {
  /* display information contained within packet 4 */
  short length,icoord,jcoord,arrow_dir,arrow_len,head_len;
  int i, num=0; /* num_vectors; */

  printf("\nPacket 5: Vector Arrow Data Packet\n");

  length=read_half(buffer,offset);
  printf("Packet 5 Length of Data Block (in bytes) = %hd  (%04x hex)\n",
    length,length);
    
  num=length/10;
  printf("Number of Vector Arrows to Print: %d\n",num);
   
  for(i=0;i<num;i++) {
     icoord=read_half(buffer,offset);
     jcoord=read_half(buffer,offset);
     arrow_dir=read_half(buffer,offset);
     arrow_len=read_half(buffer,offset);
     head_len=read_half(buffer,offset);  
     printf("#%04d I Pos: %4hd  J Pos: %4hd  Arrow Dir: %3hd  Len: %hd  Head Len: %3hd\n",
        i+1,icoord,jcoord,arrow_dir,arrow_len,head_len);
    }     
  /* printf("\n"); */
  }


