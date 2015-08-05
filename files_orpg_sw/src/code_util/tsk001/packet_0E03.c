/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:40 $
 * $Id: packet_0E03.c,v 1.6 2009/05/15 17:52:40 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/* packet_0E03.c */

#include "packet_0E03.h"

void packet_0E03_skip(char *buffer,int *offset)
{
    short len;
    /* below added for LINUX */
    MISC_short_swap(buffer+*offset,3);
    *offset+=6; /*skip over data*/
    len=read_half_flip(buffer, offset);
    /* below added for LINUX */
    MISC_short_swap(buffer+*offset,len/2);
    *offset+=len;
    
}


void display_packet_0E03(int packet,int offset) 
{
    short      length, i_start, i_end, j_start, j_end, i0, j0;
    int        i, num_vectors, count=1;
    float      x_scale, y_scale;


    int *prod_res_ind;    
    float base_res, prod_res;
    Prod_header *hdr;

    int center_pixel, center_scanl;



/*  CVG 8.4 now we treat this as an overlay product  */
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

    if(verbose_flag)
        fprintf(stderr,"\nPacket 0x0E03: Linked Contour Vectors\n");
    offset += 4;  /* skip over packet code and other junk */
    i0 = i_start = (read_half(sd->icd_product, &offset)) * x_scale + center_pixel;
    j0 = j_start = (read_half(sd->icd_product, &offset)) * y_scale + center_scanl;

    if(verbose_flag)
        fprintf(stderr,"I Starting Point: %hd   J Starting Point: %hd\n", 
                                                         i_start, j_start);
    length = read_half(sd->icd_product, &offset);
    if(verbose_flag)
        fprintf(stderr,"Packet 0x0E03 Length of Data Block (in bytes) = %hd\n", 
                                                                         length);

    length-=4; /* account for the starting points */
    num_vectors=length/4;

    if(verbose_flag)
         fprintf(stderr,"Number of Vectors: %i\n",num_vectors);


    XSetForeground(display, gc, display_colors[contour_color].pixel);

    if(verbose_flag)
        fprintf(stderr,"Color Level=%d\n", contour_color);

    for(i=0; i<num_vectors; i++) {
        i_end = (read_half(sd->icd_product, &offset)) * x_scale + center_pixel;
        j_end = (read_half(sd->icd_product, &offset)) * y_scale + center_scanl;
        XDrawLine(display, sd->pixmap, gc, i_start, j_start, i_end, j_end);
    
        if(verbose_flag)
            fprintf(stderr,"End Vector Number %4d   I=%5hd  J=%5hd\n",
                                                     count,i_end,j_end);
    
        count++;
        i_start = i_end;
        j_start = j_end;
        
    }  /*  END FOR */

    /* connect the beginning with the end */
    XDrawLine(display, sd->pixmap, gc, i_start, j_start, i0, j0);
}
