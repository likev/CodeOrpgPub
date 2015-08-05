/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:48 $
 * $Id: packet_5.c,v 1.5 2009/05/15 17:52:48 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/* packet_5.c */

#include "packet_5.h"

void packet_5_skip(char *buffer,int *offset) 
{
    /*LINUX change*/
    /*short length = read_half(buffer, offset);*/
    short length = read_half_flip(buffer, offset);
    MISC_short_swap(buffer+*offset,length/2);
    *offset += length;
}

void display_packet_5(int packet,int offset) 
{
    short length,icoord,jcoord,arrow_dir,arrow_len,head_len;
    int i, num=0;
    float x_scale, y_scale, dir_sin, dir_cos;
    /* CVG 9.0 */
    int center_pixel, center_scanl;
    int i_coord_adjusted, j_coord_adjusted;
    
    XPoint pts[5];

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




    offset+=2;
    length=read_half(sd->icd_product, &offset);
    if(verbose_flag) {
        printf("\nPacket 5: Vector Arrow Data Packet\n");
        printf("Packet 5 Length of Data Block (in bytes) = %hd  (%04x hex)\n",
        length, length);
    }

    num=length/10;
    if(verbose_flag)
        printf("Number of Vector Arrows to Print: %d\n", num);
        

    /* CVG 9.0 - modified to use color palette rather than just white */
    XSetForeground(display, gc, display_colors[1].pixel);
    
    for(i=0; i<num; i++) {
        icoord = read_half(sd->icd_product, &offset);
        jcoord = read_half(sd->icd_product, &offset);
        arrow_dir = read_half(sd->icd_product, &offset);
        arrow_len = read_half(sd->icd_product, &offset);
        head_len = read_half(sd->icd_product, &offset);  
        arrow_len *= 2;
        head_len *= 2;
        
        if(verbose_flag)
            printf("#%04d I Pos: %4hd  J Pos: %4hd  Arrow Dir: %3hd  "
                   "Len: %hd  Head Len: %3hd\n",
                   i+1, icoord, jcoord, arrow_dir, arrow_len, head_len);
    
        dir_sin = sin((double)(arrow_dir*DEGRAD));
        dir_cos = cos((double)(arrow_dir*DEGRAD));
    
        if(verbose_flag)
            printf("sin=%6.2f  cos=%6.2f  sin*len=%d  cos*len=%d\n", dir_sin, dir_cos,
               (int)(dir_sin*(float)arrow_len), (int)(dir_cos*(float)arrow_len));
    
        /* CVG 9.0 - adjust coordinates for scale factor and center adjustment */
        i_coord_adjusted = icoord * x_scale + center_pixel;
        j_coord_adjusted = jcoord * y_scale + center_scanl;
    
        /* draw the tail first */
        /* CVG 9.0 - added centering factor for large screens */ 
        XDrawLine(display, sd->pixmap, gc, (int)(i_coord_adjusted), 
                                           (int)(j_coord_adjusted), 
              (int)(i_coord_adjusted - (int)(dir_sin*(float)arrow_len)),
              (int)(j_coord_adjusted + (int)(dir_cos*(float)arrow_len)) );
    
        if(verbose_flag)
            /* CVG 9.0 - added centering factor for large screens */ 
            printf("  LINE: (%d,%d) => (%d,%d)\n", (int)(i_coord_adjusted), 
                                                   (int)(j_coord_adjusted), 
               (int)(i_coord_adjusted-(int)(dir_sin*(float)arrow_len)),
               (int)(j_coord_adjusted+(int)(dir_cos*(float)arrow_len)) );
    
        /* now draw the head */
        /* CVG 9.0 - added centering factor for large screens */ 
        /*           replaced i_coord with i_coord_adjusted, etc. */
        pts[0].x = i_coord_adjusted;
        pts[0].y = j_coord_adjusted;
        pts[1].x = i_coord_adjusted - (int)(dir_cos*(float)head_len);
        pts[1].y = j_coord_adjusted - (int)(dir_sin*(float)head_len);
        pts[2].x = i_coord_adjusted + (int)(dir_sin*(float)head_len);
        pts[2].y = j_coord_adjusted - (int)(dir_cos*(float)head_len);
        pts[3].x = i_coord_adjusted + (int)(dir_cos*(float)head_len);
        pts[3].y = j_coord_adjusted + (int)(dir_sin*(float)head_len);

        pts[4].x = pts[0].x;
        pts[4].y = pts[0].y;
        
        XFillPolygon(display, sd->pixmap, gc, pts, 5, Convex, CoordModeOrigin);
    
        if(verbose_flag)
            printf("(%d,%d) (%d,%d) (%d,%d) (%d,%d)\n", pts[0].x, pts[0].y,
               pts[1].x, pts[1].y, pts[2].x, pts[2].y, pts[3].x, pts[3].y);  
               
    } /* end for i<num */
    
    
    
}


