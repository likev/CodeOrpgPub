/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:41 $
 * $Id: packet_12.c,v 1.7 2009/05/15 17:52:41 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* packet_12.c */

#include <math.h>
#include <Xm/Xm.h>
#include "packet_12.h"

void packet_12_skip(char *buffer,int *offset) 
{
    /*LINUX change*/
    /*short length = read_half(buffer, offset);*/
    short length = read_half_flip(buffer, offset);
    MISC_short_swap(buffer+*offset,length/2);
    *offset += length;
}


void display_tvs_data(int packet, int offset)
{
    int num_halfwords, i;
    short *product_data = (short *)(sd->icd_product);
    int num_tvs, xpos, ypos;
    int pixel;
    int scanl;
    int     center_pixel;
    int     center_scanl;
    float   x_scale, y_scale;
    XPoint  X[5];

    int *prod_res_ind;    
    float base_res, prod_res;
    Prod_header *hdr;


    
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
    num_halfwords = product_data[tvs_msg_len_offset]/2;

    if(verbose_flag) {
        printf("packet code = %d\n", product_data[0]);
    printf("length of data (in bytes) = %d\n", num_halfwords*2);
    }

    /* move past the header */
    product_data += tvs_header_offset;

    /* each data block is 2 halfwords long, so we move in those increments */
    num_tvs = num_halfwords / 2;

    /* display each tvs in the packet */
    for(i=0; i<num_tvs; i++) {
        /* Extract the shear position. */
        xpos = product_data[tvs_xpos_offset];
    ypos = product_data[tvs_ypos_offset];

    if(verbose_flag) 
        printf("TVS x position = %d  TVS y position = %hd\n", xpos, ypos);

    /* get second color of colormap, since the first should be zero */
    XSetForeground(display, gc, display_colors[1].pixel);

    /* The ICD says that tvs's are inverted isosceles triangle  with a base of 10 */
    /*     and height of 14.  We use a base of 12 and a height of 19 for today's  */
    /*     hi resolution monitors                                                 */
    XSetLineAttributes (display, gc, 2, LineSolid, CapButt, JoinMiter);
    
    pixel = xpos*x_scale + center_pixel;
    scanl = ypos*y_scale + center_scanl;

    X[0].x = pixel;
    X[0].y = scanl;
    X[1].x = pixel-6;
    X[1].y = scanl-19;
    X[2].x = pixel+6;
    X[2].y = scanl-19;
    X[3].x = pixel;
    X[3].y = scanl;
    
    XFillPolygon(display, sd->pixmap, gc, X, 4, Convex, CoordModeOrigin);

    /* move the pointer to the next block */
    product_data += tvs_data_offset;

   }
   
   /* reset line width to 1 pixel */
    XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);
}
