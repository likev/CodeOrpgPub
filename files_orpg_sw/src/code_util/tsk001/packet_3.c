/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:47 $
 * $Id: packet_3.c,v 1.7 2009/05/15 17:52:47 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* packet_3.c */

#include <math.h>
#include <Xm/Xm.h>
#include "packet_3.h"


void packet_3_skip(char *buffer, int *offset) 
{
    /*LINUX change*/
    /*short length = read_half(buffer, offset);*/
    short length = read_half_flip(buffer, offset);
    MISC_short_swap(buffer+*offset,length/2);
    *offset += length;
    
}


void display_meso_data(int packet, int offset)
{
    int num_halfwords, num_mesos, xpos, ypos, i;
    short *product_data = (short *)(sd->icd_product);
    int pixel;
    int scanl;
    int size;
    int     center_pixel;
    int     center_scanl;
    float   x_scale, y_scale;
    
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


    /* set up for reading */
    offset /= 2;
    product_data += offset;

    num_halfwords = product_data[meso_msg_len_offset]/2;

    if(verbose_flag) {
        fprintf(stderr, "packet code = %d\n", product_data[0]);
    fprintf(stderr, "length of data (in bytes) = %d\n", num_halfwords*2);
    }

    /* move past the header */
    product_data += meso_header_offset;
    
    /* each data block is 3 halfwords long, so we move in those increments */
    num_mesos = num_halfwords/3;
   
    /* display each symbol */
    for(i=0; i<num_mesos; i++) {
        /* Extract the mesocyclone position. */
        xpos = product_data[meso_xpos_offset];
    ypos = product_data[meso_ypos_offset];
      
    /* Extract the mesocyclone size. */
    size = product_data[meso_size_offset];

    if(verbose_flag) {
        fprintf(stderr, "meso x position = %d  meso y position = %hd\n", xpos, ypos);
        fprintf(stderr, "meso size = %hd\n", size);
    }

    /* get second color of colormap, since the first should be zero */
    XSetForeground(display, gc, display_colors[1].pixel);

/*  The ICD says that meso's are an open circle at at least 7   *
 *  pixels in diameter and having a thickness of 4 pixels.  If  *
 *  the diameter is > 7, then the diameter is increased to that *
 *  value.  We use diameter of 12 and a line of 5 for hi resolution monitors  */
    XSetLineAttributes(display, gc, 5, LineSolid, CapButt, JoinMiter);
    
    pixel = xpos*x_scale + center_pixel;
    scanl = ypos*y_scale + center_scanl;
    
    
    if(verbose_flag)
        printf("printing meso at (%d,%d)\n", pixel, scanl);
        
    /* The circle size is  related to the underlying features.  It must */
    /* be adjusted for both product resolution and display zoom  */

    /* if conconfigured as an overlay and used as an overlay */
    if((prod_res == 999) &&  (base_res != 999)) {
        /* adjust size to resolution of base product */       
        size = size * (0.54 / base_res) * RESCALE_FACTOR * sd->scale_factor;     
    } else { /* use the inherent size resolution of the packet */ 
        size = size * RESCALE_FACTOR * sd->scale_factor; 
    }


    /* enforce the minimum size */
    if(size < 6)
        size = 6;  /* we chose minimum of diameter of 12 rather than 7 */
                   /* to accomodate modern hi resolution monitors      */
                       
    XDrawArc(display, sd->pixmap, gc, pixel-size, scanl-size, 
         size*2, size*2, 0, -(360*64));

    /* reset line width */
    XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);
        
    /* move the pointer to the next block */
    product_data += meso_data_offset;
   }
   
   
}



