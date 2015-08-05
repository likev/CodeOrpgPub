/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:43 $
 * $Id: packet_19.c,v 1.8 2009/05/15 17:52:43 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */
/* packet_19.c */

#include <math.h>
#include <Xm/Xm.h>
#include "packet_19.h"



void packet_19_skip(char *buffer,int *offset) 
{
    /*LINUX changes*/
    /*short length = read_half(buffer,offset);*/
    short length = read_half_flip(buffer,offset);
    MISC_short_swap(buffer+*offset,length/2);
    *offset += length;
}

void display_hail_data(int packet, int offset)
{
    int num_halfwords;
    short *product_data = (short *)(sd->icd_product);
    int num_hail, xpos, ypos, prob_hail, prob_svr_hail, hail_size;
    int j, i;
    int pixel;
    int scanl;
    int     center_pixel;
    int     center_scanl;
    float   x_scale, y_scale;
    
    int RT_OFFSET;
    
    XPoint  X[5];
    char    buf[20];

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
    num_halfwords = product_data[hail_msg_len_offset]/2;
    if(verbose_flag) {
        printf("packet code = %d\n", product_data[0]);
    printf("length of data (in bytes) = %d\n", num_halfwords*2);
    }

    /* move past the header */
    product_data += hail_header_offset;

    /* each data block is 5 halfwords long, so we move in those increments */
    num_hail = num_halfwords/5;

    /* read in and display each hail symbol accordingly */
    for(i=0; i<num_hail; i++) {
        xpos = product_data[hail_xpos_offset];
    ypos = product_data[hail_ypos_offset];
    prob_hail = product_data[hail_prob_offset];
    prob_svr_hail = product_data[hail_svr_prob_offset];
    hail_size = product_data[hail_size_offset];

    if(verbose_flag) {
        printf("Hail x position = %hd  Hail y position = %hd\n", xpos, ypos);
        printf("Probability of hail = %hd\n", prob_hail);
        printf("Probability of severe hail = %hd\n", prob_svr_hail);
        printf("Hail size = %d\n", hail_size);   
    }

    /* get second color of colormap, since the first should be zero */
    XSetForeground(display, gc, display_colors[1].pixel);
    XSetLineAttributes (display, gc, 2, LineSolid, CapButt, JoinMiter);
    pixel = xpos*x_scale + center_pixel;
    scanl = ypos*y_scale + center_scanl;

    /* ICD calls for small triangle with base of 8 and height 12, and */
    /* a large triangle with base of 16 and height of 20.             */
    /* these are increased slightly for today's hi resolution monitors*/
    /* large 20 b and 24 h,  small  12 b and 16 h  */
    
    /* symbol to the right of the Storm ID as per Product 
     * Specification  ICD */
    if(prob_svr_hail >= svr_hail_thrsh_1) { /* large symbol */
        if(verbose_flag)
            printf("outputting severe hail.\n");
        RT_OFFSET = 26;
/*         X[0].x = pixel; */
        X[0].x = pixel + RT_OFFSET;
        X[0].y = scanl - 12;

        X[1].x = pixel - 10 + RT_OFFSET;
        X[1].y = scanl + 12;

        X[2].x = pixel + 10 + RT_OFFSET;
        X[2].y = scanl + 12;

        X[3].x = pixel + RT_OFFSET;
        X[3].y = scanl - 12;
        
        if(verbose_flag)
            for(j=0; j<4; j++)
            printf("point #%d  (%d, %d)\n", j, X[j].x, X[j].y);

        if(prob_svr_hail >= svr_hail_thrsh_2) 
            XFillPolygon(display, sd->pixmap, gc, X, 4, Convex, CoordModeOrigin);
        else
            XDrawLines (display, sd->pixmap, gc, X, 4, CoordModeOrigin);

    } else if(prob_hail >= hail_thrsh_1) { /* small symbol */
        if(verbose_flag)
            printf("outputting normal hail.\n");
        RT_OFFSET = 22;

        X[0].x = pixel + RT_OFFSET;
        X[0].y = scanl - 8;

        X[1].x = pixel - 6 + RT_OFFSET;
        X[1].y = scanl + 8;

        X[2].x = pixel + 6 + RT_OFFSET;
        X[2].y = scanl + 8;

        X[3].x = pixel + RT_OFFSET;
        X[3].y = scanl - 8;
        
        if(prob_hail >= hail_thrsh_2)
            XFillPolygon(display, sd->pixmap, gc, X, 4, Convex, CoordModeOrigin);
        else
            XDrawLines(display, sd->pixmap, gc, X, 4, CoordModeOrigin);
            
        } else {
            if(verbose_flag)
            printf("not outputting hail product.");
            XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);
            continue;  /* we haven't met the thresholds, 
                * so we don't need to display anything */
        }
        
        /* the ICD says that hail size over 4 should be treated as 4 */
        if(hail_size > 4)
            hail_size = 4;
        if(hail_size > 0)
            sprintf(buf, "%1i", hail_size);
        else
            sprintf(buf, "*");

        XSetForeground(display, gc, white_color);
        XDrawString(display, sd->pixmap, gc, pixel-2 + RT_OFFSET, scanl+8, buf, 1);   
        
        XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);

        /* move the pointer to the next block */
        product_data += hail_data_offset;
    }

}

