/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:47 $
 * $Id: packet_3501.c,v 1.6 2009/05/15 17:52:47 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/* packet_3501.c */

#include "packet_3501.h"

void packet_3501_skip(char *buffer,int *offset) 
{
    /*LINUX changes*/
    /* Length of Vectors (multiples of 4)     */
    short length = read_half_flip(buffer, offset);  
    MISC_short_swap(buffer+*offset,length/2);
    *offset += length;
    
}



void display_packet_3501(int packet,int offset) 
{
    short length, i_start, i_end, j_start, j_end;
    int i,num_vectors,count=1;
    float   x_scale, y_scale;
    
    int *prod_res_ind;    
    float base_res, prod_res;
    Prod_header *hdr;
    
    int center_pixel, center_scanl;

    /* CVG 9.0 - added for GEOGRAPHIC_PRODUCT support */
    int  *type_ptr, msg_type;
    
    
    
        /* CVG 9.0 - added for NON_GEOGRAPHIC_PRODUCT support */
    /* get the type of product we have here */
    hdr = (Prod_header *)(sd->icd_product);
    type_ptr = assoc_access_i(msg_type_list, hdr->g.prod_id);
    if(type_ptr == NULL) {
        msg_type = NON_GEOGRAPHIC_PRODUCT; /* default */
    } else {
        msg_type = *type_ptr;
    }
    
    
    
    /* CVG 9.0 - added NON_GEOGRAPHIC_PRODUCT support */
    if(msg_type == GEOGRAPHIC_PRODUCT) {
        /* resolution of the base image displayed */
        base_res = res_index_to_res(sd->resolution);   
            
        /* configured resolution of this product */
        hdr = (Prod_header *)(sd->icd_product);
        prod_res_ind = assoc_access_i(product_res, hdr->g.prod_id);
        prod_res = res_index_to_res(*prod_res_ind);
        
        /* CVG 9.0 - use new function for scale and center calculations */
        geo_scale_and_center(pwidth, pheight, sd->scale_factor, 
                         sd->x_center_offset, sd->y_center_offset, 
                         base_res, prod_res, &x_scale, &y_scale,
                                      &center_pixel, &center_scanl);

    } else { /* a NON_GEOGRAPHIC_PRODUCT */
       /* CVG 9.0 - calculates x_scale, y_scale, center_pixel, center_scanl */
       non_geo_scale_and_center((float)pwidth, (float)pheight, 
                          &x_scale, &y_scale, &center_pixel, &center_scanl);
                                                  
    }



    offset += 2;
    length = read_half(sd->icd_product, &offset);
    
    if(verbose_flag) {
        fprintf(stderr,
                "\nPacket 0x3501: Unlinked Contour Vector Packet (no value)\n");
        fprintf(stderr,
                "Packet 0x3501 Length of Data Block (in bytes) = %hd\n", length);
    }
    
    num_vectors=length/4;
    
    if(verbose_flag)
        fprintf(stderr,"Number of Vectors: %i\n",num_vectors);


    XSetForeground(display, gc, display_colors[contour_color].pixel);
     
    for(i=0; i<num_vectors; i++) {
        i_start = (read_half(sd->icd_product, &offset)) * x_scale + center_pixel;
    j_start = (read_half(sd->icd_product, &offset)) * y_scale + center_scanl;

    if(verbose_flag)
        fprintf(stderr,"I Starting Point: %hd   J Starting Point: %hd\n", 
                                                         i_start, j_start);
    i_end = (read_half(sd->icd_product, &offset)) * x_scale + center_pixel;
    j_end = (read_half(sd->icd_product, &offset)) * y_scale + center_scanl;

    if(verbose_flag)
        fprintf(stderr,"End Vector Number %4d   I=%5hd  J=%5hd\n", 
                                               count, i_end, j_end);
    XDrawLine(display, sd->pixmap, gc, i_start, j_start, i_end, j_end);
    count++;
    }              
    
}




