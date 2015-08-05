/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:41 $
 * $Id: packet_11.c,v 1.7 2009/05/15 17:52:41 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* packet_11.c */

#include <math.h>
#include <Xm/Xm.h>
#include "packet_11.h"


void packet_11_skip(char *buffer,int *offset) 
{
    /*LINUX change*/
   /* short length = read_half(buffer, offset);*/
  short length=read_half_flip(buffer,offset);
  MISC_short_swap(buffer+*offset,length/2);
  *offset+=length;
  
}


void display_shear_3d_data(int packet, int offset)
{
    int num_halfwords, i;
    int pixel;
    int scanl;
    int     center_pixel;
    int     center_scanl;
    float   x_scale, y_scale;
    short *product_data = (short *)(sd->icd_product);
    int num_shears, xpos, ypos, size;

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
    num_halfwords = product_data[shear_3d_msg_len_offset]/2;

    if(verbose_flag) {
        printf("packet code = %d\n", product_data[0]);
    printf("length of data (in bytes) = %d\n", num_halfwords*2);
    }

    /* move past the header */
    product_data += shear_3d_header_offset;

    /* each data block is 3 halfwords long, so we move in those increments */
    num_shears = num_halfwords/3;

    /* since each packet _can_ have multiple shears in it, we need to do
     * each one seperately
     */
    for(i=0; i<num_shears; i++) {
        /* Extract the shear position. */
        xpos = product_data[shear_3d_xpos_offset];
    ypos = product_data[shear_3d_ypos_offset];
   
    /* Extract the shear size. */
    size = product_data[shear_3d_size_offset];

    if(verbose_flag) {
        printf("3D shear x position = %d  3D shear y position = %hd\n", 
           xpos, ypos);
        printf("3D shear size = %hd\n", size);
    }


    /* get second color of colormap, since the first should be zero */
    XSetForeground(display, gc, display_colors[1].pixel);

    /*  The ICD says that 3D shear's are an open circle at at least 7   *
     *  pixels in diameter and having a thickness of x1 pixel.  If      *
     *  the diameter is > 7, then the diameter is increased to that     *
     *  value.                              */
    XSetLineAttributes (display, gc, 2, LineSolid, CapButt, JoinMiter);
    
    pixel = xpos*x_scale + center_pixel;
    scanl = ypos*y_scale + center_scanl;
        
    /* The circle size is related to the underlying features.  It must be adjusted */ 
    /* for both product resolution and display zoom  */

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


    /* move the pointer to the next block */
    product_data += shear_3d_data_offset;
   }
   
   /* reset line width to 1 pixel */
    XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);
}
