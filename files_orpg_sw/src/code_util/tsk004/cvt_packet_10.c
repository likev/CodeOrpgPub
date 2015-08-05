/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 17:59:32 $
 * $Id: cvt_packet_10.c,v 1.2 2003/02/06 17:59:32 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
 
/* cvt_packet_10.c */

#include "cvt_packet_10.h"


void packet_10(char *buffer,int *offset) {
  /* display information contained within packet 10 */
  short length,color_level;
  int i,num_vectors;

  printf("\nPacket 10: Unlinked Vector Packet\n");

     length=read_half(buffer,offset);
     printf("Packet 10 Length of Data Block (in bytes) = %hd\n",length);
     color_level=read_half(buffer,offset);
     printf("Color Level of Vectors (0-15): %hd\n",color_level);
     if(color_level<1 || color_level>16) {
         printf("Data Error: Color level was out of range\n");
         return;
         }
     
     length-=2; /* account for the color&length values */
     num_vectors=length/8;
     for(i=0;i<num_vectors;i++) {
/*       unsigned char c; */
       short begI,begJ,endI,endJ;
    
       begI=read_half(buffer,offset);
       begJ=read_half(buffer,offset);
       endI=read_half(buffer,offset);
       endJ=read_half(buffer,offset);
    
       printf("Vector %03d: BegI:%5hd    BegJ:%5hd  |  EndI:%5hd    EndJ:%5hd\n",
         i+1,begI,begJ,endI,endJ);
       }
       
     printf("Packet 10 Complete\n");
           
  printf("\n");
  }


