/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:44 $
 * $Id: packet_23.c,v 1.4 2009/05/15 17:52:44 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/* packet_23.c */

#include "packet_23.h"

/* THIS PACKET CAN CONTAIN packets 2, 6, and 25 */

void packet_23_skip(char *buffer,int *offset) 
{
short length;

    length = read_half_flip(buffer, offset);

}


void display_packet_23(int packet,int offset) 
{
    short length;
    int end_offset;
    
    offset += 2;
    length=read_half(sd->icd_product, &offset);

    if(verbose_flag) {
        printf("\nPacket 23: SCIT / MDA Position Data\n");
    printf("Length of Data Block (in bytes) = %hd\n",length);
    }

    end_offset = length+offset;
    /* packet codes 2, 6 or 25 follow */
    while(offset<end_offset) {
      int pcode, len;
      /* grab packet code and length of embedded packet */
      pcode = read_half(sd->icd_product, &offset);
      len = read_half(sd->icd_product, &offset);
      
      /* send off the embedded packet to get displayed and update the offset*/
      /* COULD BE Packet 2, Packet6, or Packet 25 */
      dispatch_packet_type(pcode, offset-4, TRUE);
      offset += len;
      
    } /* end while */
    
    
}


