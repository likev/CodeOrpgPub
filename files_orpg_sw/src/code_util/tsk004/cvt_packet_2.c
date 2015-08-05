/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:14:10 $
 * $Id: cvt_packet_2.c,v 1.1 2002/08/30 16:14:10 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* cvt_packet_2.c */

#include "cvt_packet_2.h"


void packet_2(char *buffer,int *offset,int *flag) {
  /* display information contained within packet 2: Write Special Symbol (no value) */
  short length;
  int i;

  printf("Packet 2: Write Special Symbols (No Value) Summary Information\n");

  length=read_half(buffer,offset);
  printf("Length of Data Block (in bytes) = %hd\n",length);
  printf("I Starting Point: %hd\n",read_half(buffer,offset));
  printf("J Starting Point: %hd\n",read_half(buffer,offset));

  length-=4;
  /*printf("length of packet 2 string=%d\n",length);*/
  printf("Message Begins at next line:\n");

  for(i=0;i<length;i++) {
    unsigned char c;

    c=read_byte(buffer,offset);
    printf("%c",c);
    }

  printf("\n");
  printf("Message Complete\n");
  }


