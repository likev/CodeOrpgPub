/* packet_25.c */

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:45 $
 * $Id: packet_25.c,v 1.8 2009/05/15 17:52:45 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */
/* NOT VERIFIED */
/* EXISTING STI PRODUCT USES PACKET 2 FOR ALL THREE CASES: */
/* PAST, PRESENT, AND FORECAST */
/* NO EXAMPLE OF USE OF THIS PACKET HAS BEEN FOUND */

#include "packet_25.h"

void packet_25_skip(char *buffer,int *offset) 
{
    /*LINUX change*/
    short length = read_half_flip(buffer,offset);
    MISC_short_swap(buffer+*offset,length/2);
    *offset += length;
    
}



void display_packet_25(int packet, int offset) 
{
    short length,x,y,rad;

    
    /* previous version did not adjust to center of screen */
    int     center_pixel;
    int     center_scanl;    
    
    int pixel, scanl;
    
    float x_scale, y_scale;

    int *prod_res_ind;    
    float base_res, prod_res;
    Prod_header *hdr;




    offset+=2;
    
    length = read_half(sd->icd_product, &offset);    
    x = read_half(sd->icd_product, &offset);
    y = read_half(sd->icd_product, &offset);
    rad = read_half(sd->icd_product, &offset);    


    
    if(verbose_flag) {
        printf("\nPacket 25: STI Circle\n");
    printf("Packet Length: %hd\n", length);
    printf("I Position: %hd\n", x);
    printf("J Position: %hd\n", y);
    printf("Radius of Circle: %hd\n", rad);
    }


  
    /* This looks kludgy, however the Legacy rescale factor assumed a 0.54 NM*/
    /*   product displayed on a PUP                                          */
    /* In order to be able to display over other product resolutions : */

    /* resolution of the base image displayed */
    base_res = res_index_to_res(sd->resolution);   
    
    /* configured resolution of this product */
    hdr = (Prod_header *)(sd->icd_product);
    prod_res_ind = assoc_access_i(product_res, hdr->g.prod_id);
    prod_res = res_index_to_res(*prod_res_ind);

    
    /* Accoarding to the Class 1 User ICD, the radius of the 
     * circle is stated in pixels not in 1/4 KM.  Therefore the
     * radius should NOT expand with product zoom
     */   

    /* CVG 9.0 - use new function for scale and center calculations */
    geo_scale_and_center(pwidth, pheight, sd->scale_factor, 
                     sd->x_center_offset, sd->y_center_offset, 
                     base_res, prod_res, &x_scale, &y_scale,
                                  &center_pixel, &center_scanl);


    pixel = x * x_scale + center_pixel;
    scanl = y * y_scale + center_scanl;
    

    XSetForeground(display, gc, display_colors[1].pixel);
  
    /* to improve appearance, increase radius by 2 pixels */
    rad += 2;


    XDrawArc(display, sd->pixmap, gc, pixel-rad, scanl-rad, rad*2, rad*2, 0, -(360*64));
    
}

