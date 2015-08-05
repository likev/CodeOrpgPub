/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:42 $
 * $Id: packet_15.c,v 1.9 2009/05/15 17:52:42 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */
/* packet_15.c */

#include <math.h>
#include <Xm/Xm.h>
#include "packet_15_cvg.h"

void packet_15_skip(char *buffer,int *offset) 
{
int i=0;
    /*LINUX change*/
    /*short length = read_half(buffer, offset);*/
    short length = read_half_flip(buffer, offset);
    for(;i<length;i+=6) {
    MISC_short_swap(buffer+*offset+i,2);/* swaps the two shorts for each symbol */
    }
    *offset += length;
}



void display_storm_id_data(int packet, int offset)
{
    char storm_id[10];
    short *product_data = (short *)(sd->icd_product);
    int num_halfwords;
    char *str_buf;
    int num_id, xpos, ypos, i;
    int pixel;
    int scanl;
    int     center_pixel;
    int     center_scanl;
    float   x_scale, y_scale;

    int *prod_res_ind;    
    float base_res, prod_res;
    Prod_header *hdr;

    Boolean norm_format_flag = False;
    Boolean bkgd_format_flag = False;
    
    if(sd==sd1) {
        norm_format_flag = norm_format1;
        bkgd_format_flag = bkgd_format1;
    } else if(sd==sd2) {
        norm_format_flag = norm_format2;
        bkgd_format_flag = bkgd_format2;
    } else if(sd==sd3) {
        norm_format_flag = norm_format3;
        bkgd_format_flag = bkgd_format3;
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
    


    /* CVG 9.0 - use new function for scale and center calculations */
    geo_scale_and_center(pwidth, pheight, sd->scale_factor, 
                     sd->x_center_offset, sd->y_center_offset, 
                     base_res, prod_res, &x_scale, &y_scale,
                                  &center_pixel, &center_scanl);

    /* advance the pointer to the beginning of the packet */
    offset /= 2;
    product_data += offset;
    
    /* get the data length */
    num_halfwords = product_data[storm_id_msg_len_offset]/2;

    if(verbose_flag) {
        printf("packet code = %d\n", product_data[0]);
    printf("length of data (in bytes) = %d\n", num_halfwords*2);
    }

    /* move past the header */
    product_data += storm_id_header_offset;

    /* each data block is 3 halfwords long, so we move in those increments */
    num_id = num_halfwords/3;

    for(i=0; i<num_id; i++) {
        /* Extract the shear position. */
        xpos = product_data[storm_id_xpos_offset];
        ypos = product_data[storm_id_ypos_offset];
       
        /* move pointer to ID chars */
        product_data += storm_id_str_offset;
        str_buf = (char *)product_data;
           
        /* get storm ID */
        storm_id[0] = str_buf[0];
        storm_id[1] = str_buf[1];
    
        if(verbose_flag) {
            printf("Storm ID x position = %d  Storm ID y position = %hd\n", xpos, ypos);
            printf("Storm ID = %c%c\n", storm_id[0], storm_id[1]);
        }
    
        /* there should only be two colors, the first one black */
        XSetForeground(display, gc, display_colors[1].pixel);
        
        pixel = xpos * x_scale + center_pixel;
        scanl = ypos * y_scale + center_scanl;
    
    
        if(norm_format_flag==TRUE)
            XDrawString(display, sd->pixmap, gc, pixel+5, scanl+12, storm_id, 2);
        else if (bkgd_format_flag==TRUE)
            XDrawImageString(display, sd->pixmap, gc, pixel+5, scanl+12, storm_id, 2);
    
        /* move the pointer to the next block */
        product_data++;
    
    }    /*  end for */
    
}

