/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:45:44 $
 * $Id: cvt_packet_17.c,v 1.4 2008/03/13 22:45:44 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/* cvt_packet_17.c */

#include "cvt_packet_17.h"


void packet_17(char *buffer,int *offset,int *flag) {
  /* display information contained within packet 17 */
  short spare,boxes,rows,numbytes;
  int i,j;
  int k;
  unsigned char run;
  unsigned char level;

  printf("Packet 17: Digital Precipitation Data Array Packet\n");
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

    for(j=0;j<numbytes/2;j++) { 
      run = (unsigned char) read_byte(buffer,offset);
      level = (unsigned char) read_byte(buffer,offset);
      /*  report pre-build 8 byte swap error */
      if((i==0) && (j==0) && (run==255))
          printf("Linux DPA Error, the Run and Level values are reversed in packet 17.\n");
      printf("Run(%03hd) Level(%03hd)  ", run, level);
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
  printf("Message 17 Complete\n");
  
  }


