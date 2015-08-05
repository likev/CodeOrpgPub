/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2004/09/20 21:50:39 $
 * $Id: cvt_packet_18.c,v 1.2 2004/09/20 21:50:39 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* cvt_packet_18.c */

#include "cvt_packet_18.h"


void packet_18(char *buffer,int *offset,int *flag) {
  /* display information contained within packet 18 */
  short spare,boxes,rows,numbytes;
  int i,j;
  int k;

  printf("Packet 18: Precipitation Rate Data Array Packet\n");
  spare=read_half(buffer,offset);
  spare=read_half(buffer,offset);
  boxes=read_half(buffer,offset);
  printf("Number of LFM Boxes in Row = %hd\n",boxes );
  rows=read_half(buffer,offset);
  printf("Number of Rows = %hd\n",rows);
 
  for(i=0;i<rows;i++) {
    numbytes=read_half(buffer,offset);
    printf("Number of Bytes in Row %d = %hd\n",i+1,numbytes);

    k=0;

    for(j=0;j<numbytes;j++) {
      unsigned char c=read_byte(buffer,offset);
      unsigned char run,level;

      run=c>>4;
      level=c & 0x0f;
      printf("Run(%03hd) Level(%03hd)  ",run,level);
      if(k==2) {
          printf("\n");
          k=0;
      } else {
           k++;
          }
      }
    printf("\n");
    }

  printf("\n");
  printf("Message 18 Complete\n");
  }


