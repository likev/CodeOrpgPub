/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 18:20:21 $
 * $Id: cvt_packet_25.c,v 1.2 2003/02/06 18:20:21 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
 
/* cvt_packet_25.c */

#include "cvt_packet_25.h"


void packet_25(char *buffer,int *offset) {
  /* display information contained within packet 25 */
/*  short length; */
/*  int i,num=0; */

    printf("\nPacket 25: STI Circle\n");
    
    printf("Packet Length: %hd\n",read_half(buffer,offset));    
    printf("I Position: %hd\n",read_half(buffer,offset));
    printf("J Position: %hd\n",read_half(buffer,offset));
    printf("Radius of Circle: %hd\n",read_half(buffer,offset));    

    printf("\n");
   
  }


