/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:13:49 $
 * $Id: cvt_packet_15.c,v 1.1 2002/08/30 16:13:49 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* cvt_packet_15.c */

#include "cvt_packet_15.h"


void packet_15(char *buffer,int *offset,int *flag) {
  /* display information contained within packet 15 */
  short length,ipos,jpos;
  unsigned char a,b;
  int i,num;

  printf("\nPacket 15: Storm ID Data\n");

  length=read_half(buffer,offset);  /* length in bytes */
  /*printf("Length of Data Block (in bytes) = %hd\n",length);*/
  num=length/6 ;
  
  printf("Packet 15: Length=%4hd  Number Included=%hd\n",
      length,num);
  /*printf("Number of Storm ID's to Print: %d\n",num);*/

  for(i=1;i<=num;i++) {

    ipos=read_half(buffer,offset);
    jpos=read_half(buffer,offset);
    a=read_byte(buffer,offset);
    b=read_byte(buffer,offset);
    
    printf("  I Pos: %4hd  J Pos: %4hd  Storm ID: %c%c\n",
      ipos,jpos,a,b);
      /*
    printf("Storm ID %d\n",i);
    printf("I Position: %hd\n",read_half(buffer,offset));
    printf("J Position: %hd\n",read_half(buffer,offset));
    a=read_byte(buffer,offset);
    b=read_byte(buffer,offset);
    printf("Storm ID: %c%c\n",a,b);
    printf("\n");
    */
    }
  /*printf("Message Complete\n");*/
  printf("\n");
  }


