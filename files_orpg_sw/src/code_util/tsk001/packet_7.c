/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:49 $
 * $Id: packet_7.c,v 1.5 2009/05/15 17:52:49 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/* packet_7.c */

#include "packet_7.h"

void packet_7_skip(char *buffer,int *offset) 
{
    /*LINUX change*/
    /*short length = read_half(buffer, offset);*/
    short length = read_half_flip(buffer, offset);
    MISC_short_swap(buffer+*offset,length/2);
    *offset += length;
}

void display_packet_7(int packet,int offset) 
{
    short length, i_start, i_end, j_start, j_end;
    int i,num_vectors,count=1;
    float   x_scale, y_scale;
    /* CVG 9.0 */
    int center_pixel, center_scanl;

    /* CVG 9.0 - added for GEOGRAPHIC_PRODUCT support */
    int  *type_ptr, msg_type;
    Prod_header *hdr;
    int *prod_res_ind;    
    float base_res, prod_res;




    /* CVG 9.0 - added for GEOGRAPHIC_PRODUCT support */
    /* get the type of product we have here */
    hdr = (Prod_header *)(sd->icd_product);
    type_ptr = assoc_access_i(msg_type_list, hdr->g.prod_id);
    if(type_ptr == NULL) {
        msg_type = NON_GEOGRAPHIC_PRODUCT; /* default */
    } else {
        msg_type = *type_ptr;
    }


    /* CVG 9.0 - added GEOGRAPHIC_PRODUCT support */
    if(msg_type == GEOGRAPHIC_PRODUCT) {
        /* configured resolution of this product */
        prod_res_ind = assoc_access_i(product_res, hdr->g.prod_id);
        prod_res = res_index_to_res(*prod_res_ind);
        /* CVG 9.0 - added for GEOGRAPHIC_PRODUCT support */
        /* resolution of the base image displayed */
        base_res = res_index_to_res(sd->resolution);   
        
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
        printf("\nPacket 7: Unlinked Vector Packet (no value)\n");
        printf("Packet 7 Length of Data Block (in bytes) = %hd\n", length);
    }

    num_vectors=length/8;
    if(verbose_flag)
        printf("Number of Vectors: %i\n", num_vectors);


    XSetForeground(display, gc, display_colors[1].pixel);
     
    for(i=0; i<num_vectors; i++) {
        /* CVG 9.0 - added centering factor for large screens */
        i_start = read_half(sd->icd_product, &offset) * x_scale + center_pixel;
        j_start = read_half(sd->icd_product, &offset) * y_scale + center_scanl;
        if(verbose_flag)
            printf("I Starting Point: %hd   J Starting Point: %hd\n", i_start, j_start);
        /* CVG 9.0 - added centering factor for large screens */
        i_end = read_half(sd->icd_product, &offset) * x_scale + center_pixel;
        j_end = read_half(sd->icd_product, &offset) * y_scale + center_scanl;
        if(verbose_flag)
            printf("End Vector Number %4d   I=%5hd  J=%5hd\n",count,i_end,j_end);
        XDrawLine(display, sd->pixmap, gc, i_start, j_start, i_end, j_end);
        count++;
        
    }  /* end for num_vectors */
    
} /* end display_packet_7 */


