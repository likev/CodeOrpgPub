/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:43 $
 * $Id: packet_2.c,v 1.8 2009/05/15 17:52:43 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */
/* packet_2.c */

#include "packet_2.h"

/* THE RPG CLASS 1 USER ICD DOES NOT COMPLETELY DESCRIBE THIS PACKET */
/* 1.  The use of the character bytes is not explained.   However,   */
/*     for the Storm Tracking Information product and the MDA        */
/*     tracking product, the value of the  first byte indicates      */
/*     which symbol should be drawn.                                 */
/* 2.  The I and J values are not in 1/4 KM Screen Coordinates as    */
/*     indicated - At least for the only known uses of this packet   */
/*     (which is for the Storm Tracking and MDA Tracking Products).  */
/* 3.  Are there any other uses for this packet?                     */

void packet_2_skip(char *buffer,int *offset) 
{
    /*LINUX change*/
    /*short length = read_half(buffer, offset);*/
short length;
    MISC_short_swap(buffer+*offset,3);
length = read_half(buffer, offset);
    *offset += length;
}

void display_packet_2(int packet, int offset) 
{
    short length;
    int   i;
    int pixel, center_pixel;
    int scanl, center_scanl;
    float x_scale, y_scale;
    char  p2_str[150];
    int   p2_i_start, p2_j_start;
    XPoint  X[4];

    int *prod_res_ind;    
    float base_res, prod_res;
    Prod_header *hdr;

    char ud_str[3] = "UD";

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


    /* a new color palette has been defined for
       packet 2 and is associated by default in the palette_list file */ 
    /* initially the colors in symbol_pkt2.plt are :
     * 0 - P2_BLACK,  1 - P2_WHITE, 2 - P2_YELLOW, 3 - P2_RED, 
     * 4 - P2_GREEN, 5 - P2_BLUE, 6 - P2_ORANGE, 7 - P2_GRAY           */ 

    
    offset+=2;
    length = read_half(sd->icd_product, &offset);
    p2_i_start = read_half(sd->icd_product, &offset);
    p2_j_start = read_half(sd->icd_product, &offset);
    
    if(verbose_flag) {
        printf("Decoding Packet 2:\n");
    printf("Length of Data Block (in bytes) = %hd\n",length);
    printf("I Start Point:%hd   J Start Point:%hd\n",p2_i_start, p2_j_start);
    printf("Text Data: ");
    }
    length -= 5;


    for(i=0; i<length; i++) {
        unsigned char c;
    c = read_byte(sd->icd_product, &offset);
    p2_str[i] = c;
    
    if(verbose_flag)
        printf("%d ", c);
    }
    
    if(verbose_flag)
        printf("\n");


   
    
    /* This looks kludgy, however the Legacy rescale factor assumed a 0.54 NM*/
    /*   product displayed on a PUP                                          */
    /* In order to be able to display over other product resolutions : */

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


    pixel = p2_i_start * x_scale + center_pixel;
    scanl = p2_j_start * y_scale + center_scanl;


    /* These special characters are for Storm Tracking Data and for MDA 
     * Tracking Data--we're assuming that they may generalize to other 
     * products, but we do not yet know of any other use of packet 2 
     */
    /* the new packet 20, point feature data, has been created for adding */
    /* adding additional special symbols                                  */
     
     /**************************************************************
     * ACCORDING TO FIGURE 3-8b in Class 1 User ICD
     *
     * 021x or 33 is for Past storm cell position
     * 022x or 34 is for Current storm cell position
     * 023x or 35 is for Forecase storm cell position
     * 024x or 36 is for Past MDA position
     * 025x or 37 is for Forecast MDA position
     *
     ****************************************************************/

/* using the new default palette for packet 2 (symbol_pkt2.plt) */  
   

    XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);   
     
    if(p2_str[0] == 35) {          /* Storm Tracking Forecast */ 
        if(verbose_flag) printf(" Symbol: Storm Track Forecast Position\n");       
    /* draw a plus sign */
        /* improve appearance, increase size by one pixel */
        XSetForeground(display, gc, display_colors[P2_WHITE].pixel);        
        XSetLineAttributes (display, gc, 2, LineSolid, CapButt, JoinMiter);
        XDrawLine(display, sd->pixmap, gc, pixel-4, scanl, pixel+4, scanl);
        XDrawLine(display, sd->pixmap, gc, pixel, scanl-4, pixel, scanl+4);
    } else if(p2_str[0] == 34) {   /* Storm Tracking Current Pos. */
        if(verbose_flag) printf(" Symbol: Storm Track Current Position\n");
        XSetForeground(display, gc, display_colors[P2_WHITE].pixel); 
        XSetLineAttributes (display, gc, 2, LineSolid, CapButt, JoinMiter);
        /* draw circle (diameter 7 pixels) with 'x' in middle */
        /* improve appearance, increase size by one pixel */
        XDrawArc(display, sd->pixmap, gc, pixel-4, scanl-4, 8, 8, 0, -(360*64)); 
        XDrawLine(display, sd->pixmap, gc, pixel-3, scanl-3, pixel+3, scanl+3);
        XDrawLine(display, sd->pixmap, gc, pixel-3, scanl+3, pixel+3, scanl-3);
    } else if(p2_str[0] == 33) {   /* Storm Tracking Past Pos. */
        if(verbose_flag) printf(" Symbol: Storm Track Past Position\n");
        /* draw filled in circle (diameter 5 pixels) */
        /* improve circle appearance, increase size one pixel */
        XSetForeground(display, gc, display_colors[P2_WHITE].pixel); 
        XSetLineAttributes (display, gc, 2, LineSolid, CapButt, JoinMiter);
        XFillArc(display, sd->pixmap, gc, pixel-3, scanl-3, 6, 6, 0, -(360*64));
    } else if(p2_str[0] == 36) {   /* MDA Past Position */
        if(verbose_flag) printf(" Symbol: MDA Track Past Position\n");
        /* draw a filled in yellow diamond similar in size */
        /* to the storm past position */
        XSetForeground(display, gc, display_colors[P2_YELLOW].pixel); 
        XSetLineAttributes (display, gc, 2, LineSolid, CapButt, JoinMiter);
        X[0].x = pixel;
        X[0].y = scanl+4;
        X[1].x = pixel+4;
        X[1].y = scanl;
        X[2].x = pixel;
        X[2].y = scanl-4;
        X[3].x = pixel-4;
        X[3].y = scanl;
        XFillPolygon(display, sd->pixmap, gc, X, 4, Convex, CoordModeOrigin);
    } else if(p2_str[0] == 37) {   /* MDA Forecast Position */
        if(verbose_flag) printf(" Symbol: MDA Track Forecast Position\n");
        XSetForeground(display, gc, display_colors[P2_YELLOW].pixel); 
        /* draw a yellow 'X' similar in size to the storm forecast position */
        XSetLineAttributes (display, gc, 2, LineSolid, CapButt, JoinMiter);
        XDrawLine(display, sd->pixmap, gc, pixel-3, scanl-3, pixel+3, scanl+3);
        XDrawLine(display, sd->pixmap, gc, pixel-3, scanl+3, pixel+3, scanl-3);
    } else {
        if(verbose_flag) printf(" Symbol: Undefined Special Symbol\n");
        /* set the color and print whatever string by default */
        XSetForeground(display, gc, display_colors[P2_BLACK].pixel);
        XSetBackground(display, gc, display_colors[P2_ORANGE].pixel);
        XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter); 
        /* The -5  +5  offset places the two character text over the point */
        XDrawImageString(display, sd->pixmap, gc, pixel-5, scanl+5, ud_str, 2);
    }
    
    XSetForeground(display, gc, white_color);
    XSetBackground(display, gc, black_color);
    XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);
}





