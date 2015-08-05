/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:49 $
 * $Id: packet_6.c,v 1.7 2009/05/15 17:52:49 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* packet_6.c */

#include "packet_6.h"

void packet_6_skip(char *buffer,int *offset) 
{
    /*LINUX change*/
    /*short length = read_half(buffer, offset);*/
    short length = read_half_flip(buffer, offset);
    MISC_short_swap(buffer+*offset,length/2);
    *offset += length;
}

void display_packet_6(int packet,int offset) 
{
    short length, i_start, i_end, j_start, j_end;
    int i,num_vectors,count=1;
    float   x_scale, y_scale;
    int center_pixel, center_scanl;
    /* CVG 9.0 */
    int  *type_ptr, msg_type;
    
    int *prod_res_ind;    
    float base_res, prod_res;
    Prod_header *hdr;



    /* CVG.9.0 - ADDED SUPPORT FOR NON_GEOGRAPHIC_PRODUCT */
    
    
    /* get the type of product we have here */
    hdr = (Prod_header *)(sd->icd_product);
    type_ptr = assoc_access_i(msg_type_list, hdr->g.prod_id);
    if(type_ptr == NULL) {
        msg_type = NON_GEOGRAPHIC_PRODUCT; /* default */
    } else {
        msg_type = *type_ptr;
    }


     /* CVG 9.0 */
    if(msg_type == NON_GEOGRAPHIC_PRODUCT) {

        /* CVG 9.0 - calculates x_scale, y_scale, center_pixel, center_scanl */
        non_geo_scale_and_center((float)pwidth, (float)pheight, 
                              &x_scale, &y_scale, &center_pixel, &center_scanl);

        

    /* CVG 9.0 */
    } else if(msg_type == GEOGRAPHIC_PRODUCT) {
        
        /* resolution of the base image displayed */
        base_res = res_index_to_res(sd->resolution);   
    
        /* configured resolution of this product */
        hdr = (Prod_header *)(sd->icd_product);
        prod_res_ind = assoc_access_i(product_res, hdr->g.prod_id);
        prod_res = res_index_to_res(*prod_res_ind);
        
        
        geo_scale_and_center(pwidth, pheight, sd->scale_factor, 
                             sd->x_center_offset, sd->y_center_offset, 
                             base_res, prod_res, &x_scale, &y_scale,
                                          &center_pixel, &center_scanl);
        
    } /* END if GEOGRAPHIC_PRODUCT */


    if(verbose_flag)
        printf("\nPacket 6: Linked Vector Packet (no value)\n");
    offset += 2;
    length = read_half(sd->icd_product, &offset);
    if(verbose_flag)
        printf("Packet 6 Length of Data Block (in bytes) = %hd\n", length);
    i_start = (read_half(sd->icd_product, &offset)) * x_scale + center_pixel;
    j_start = (read_half(sd->icd_product, &offset)) * y_scale + center_scanl;
    if(verbose_flag)
        printf("I Start Point: %hd   J Start Point: %hd\n", i_start, j_start);

    length -= 4; /* account for the starting points */
    num_vectors = length/4;
    printf("Number of Vectors: %i\n", num_vectors);


    XSetForeground(display, gc, display_colors[1].pixel);
     
    for(i=0;i<num_vectors;i++) {
        i_end = (read_half(sd->icd_product, &offset)) * x_scale + center_pixel;
        j_end = (read_half(sd->icd_product, &offset)) * y_scale + center_scanl;
        XDrawLine(display, sd->pixmap, gc, i_start, j_start, i_end, j_end);
    
        if(verbose_flag)
            printf("End Vector Number %4d   I=%5hd  J=%5hd\n", count, i_end, j_end);
        count++;
        i_start = i_end;
        j_start = j_end;
        
    } /* end for num_vectors */
    
    
}



