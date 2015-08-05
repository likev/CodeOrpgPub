/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 18:02:32 $
 * $Id: cvt_packet_19.c,v 1.2 2003/02/06 18:02:32 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* cvt_packet_19.c */

#include "cvt_packet_19.h"


void packet_19(char *buffer,int *offset,int *flag) {
  /* display information contained within packet 19 */
/*  short length; */
    short length,ipos,jpos,prob,prob_sevr,m_size;  
  int i,num;

  printf("\nPacket 19: HDA Hail Data\n");
  printf("Value of -999 indicates that the cell is beyond the maximum\n");
  printf("  range for algorithm processing\n");

  length=read_half(buffer,offset);  /* length in bytes */
  num=length/10;  
  printf("Length of Data Block (in bytes) = %hd Number included=%d\n",length,num);

  for(i=1;i<=num;i++) {
      ipos=read_half(buffer,offset);
      jpos=read_half(buffer,offset);
      prob=read_half(buffer,offset);
      prob_sevr=read_half(buffer,offset);
      m_size=read_half(buffer,offset);
 
      printf("  I Pos: %4hd  J Pos: %4hd  Prob of Hail: %hd  of Severe Hail: %hd  Max Size (in): %hd\n",
         ipos,jpos,prob,prob_sevr,m_size);
/*      
    printf("Hail Symbol %d\n",i);
    printf("I Pos: %4hd  J Pos=%4hd\n",read_half(buffer,offset),read_half(buffer,offset));
    printf("Probability of Hail:        %hd%%\n",read_half(buffer,offset));
    printf("Probability of Severe Hail: %hd%%\n",read_half(buffer,offset));
    printf("Maximum Hail Size (inches): %hd\n",read_half(buffer,offset));
    printf("\n");
*/
    }
  /*printf("Message Complete\n");*/
  }


