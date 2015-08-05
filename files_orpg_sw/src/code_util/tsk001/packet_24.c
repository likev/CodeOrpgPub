/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:45 $
 * $Id: packet_24.c,v 1.5 2009/05/15 17:52:45 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/* packet_24.c */

#include "packet_24.h"

/* THIS PACKET CAN CONTAIN packets 2, 6, and 25 */


void packet_24_skip(char *buffer,int *offset) 
{
short length;
    /*LINUX changes*/
    length = read_half_flip(buffer, offset);
    /* offset not adjusted for length because embedded packets follow */
    
}



void display_packet_24(int packet,int offset) 
{
    /* display information contained within packet 24 */
    short length;
    int end_offset;

    offset += 2;
    length = read_half(sd->icd_product, &offset);

    if(verbose_flag) {
        printf("\nPacket 24: SCIT / MDA Position Data\n");
        printf("Length of Data Block (in bytes) = %hd\n", length);
    }
    
    end_offset = length + offset;

    /* packet codes 2, 6 or 25 follow */
    while(offset<end_offset) {
        int pcode, len;
        /* grab packet code and length of embedded packet */
        /* COULD BE Packet 2, Packet6, or Packet 25 */
        pcode = read_half(sd->icd_product, &offset);
        len = read_half(sd->icd_product, &offset);
        
        /* send off the embedded packet to get displayed and update the offset*/
    
        dispatch_packet_type(pcode, offset-4, TRUE);
        offset += len;
        
    } /* end while */
    
    
}


