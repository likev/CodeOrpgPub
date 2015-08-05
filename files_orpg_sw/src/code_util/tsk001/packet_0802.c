/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2004/03/11 22:38:46 $
 * $Id: packet_0802.c,v 1.3 2004/03/11 22:38:46 cheryls Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/* packet_0802.c */

#include "packet_0802.h"

void packet_0802_skip(char *buffer,int *offset)
{
#ifdef LITTLE_ENDIAN_MACHINE
MISC_short_swap(buffer+*offset,2);
#endif
    *offset+=4;
    
}

void display_packet_0802(int packet,int offset) 
{
    int val;

    offset+=2;
    val = read_half(sd->icd_product, &offset);
    contour_color = read_half(sd->icd_product, &offset);

    printf("\nPacket 0x0802: Contour Vector Color Levels\n");
    printf("Color Value Indicator             %x\n", val);
    printf("Color Level of Contour            %hd\n", contour_color);
}


  
