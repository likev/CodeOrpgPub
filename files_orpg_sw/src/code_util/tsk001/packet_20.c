/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:44 $
 * $Id: packet_20.c,v 1.8 2009/05/15 17:52:44 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */
/* packet_20.c */

#include <math.h>
#include <Xm/Xm.h>
#include "packet_20.h"

/* internal functions */
/* types 1, 2, 3, and 4 are displayed  with  */
/*               the draw_meso_ru() function */
/* types 5, 6, 7, and 8 are displayed  with  */
/*               the draw_tvs_ru() function  */
/* types 9, 10, and 11 are displayed  with   */
/*               the draw_mda() function     */
void draw_meso_ru(int xpos, int ypos, int type, int attr);
void draw_tvs_ru(int xpos, int ypos, int type, int attr);
void draw_mda(int xpos, int ypos, int type, int attr);
void draw_undefined_symbol(int xpos, int ypos, int type, int attr);





void packet_20_skip(char *buffer,int *offset) 
{
  /*LINUX changes*/
  /*short length=read_half(buffer,offset);*/
  short length=read_half_flip(buffer,offset);
  MISC_short_swap(buffer+*offset,length/2);
  *offset+=length;
}



void display_packet_20(int packet, int offset)
{

    int num_halfwords, i;
    int hw_per_symbol = point_feature_data_offset;

    short *product_data = (short *)(sd->icd_product);
    int num_symbols, xposition, yposition, attribute;
    int symbol_type = -1;


    /* advance the pointer to the beginning of the packet */
    offset /= 2;
    product_data += offset;

            
    /* get the data length */
    num_halfwords = product_data[point_feature_msg_len_offset]/2;

    if(verbose_flag) {
        printf("packet code = %d\n", product_data[0]);
    printf("total length of data (in bytes) = %d\n", num_halfwords*2);
    }

    /* move past the header */
    product_data += point_feature_header_offset;

    num_symbols = num_halfwords/hw_per_symbol;


    /* since each packet _can_ have multiple shears in it, we need to do
     * each one seperately
     */
    for(i=0; i<num_symbols; i++) {
        /* Extract the point feature position. */
        
        if(verbose_flag)
            fprintf(stderr, "offset to current symbol data is %d \n",
                    (int) product_data);
        
        xposition = product_data[point_feature_xpos_offset];
        yposition = product_data[point_feature_ypos_offset];
   
        /* Extract the point feature type. */
        symbol_type = product_data[point_feature_type_offset];;
       
        /* Extract the point feature attribute. */
        attribute = product_data[point_feature_attribute_offset];

        if(verbose_flag) {
            printf("point feature x position = %d  point feature y position = %d\n", 
                xposition, yposition);
            printf("point feature type = %d  point feature attribute = %d\n",
                symbol_type, attribute);
        }


        /* a color palette has been defined for packet 20 and
           is associated by default in the palette_list file */
           
        /* initially the colors in symbol_pkt20.plt are :
         * 0 - P20_BLACK,  1 - P20_WHITE, 2 - P20_YELLOW, 3 - P20_RED, 
         * 4 - P20_GREEN, 5 - P20_BLUE, 6 - P20_ORANGE, 7 - P20_GRAY       */



    
        switch(symbol_type) {
       
            case 1:
            case 2:
            case 3:
            case 4:
                draw_meso_ru(xposition, yposition, symbol_type, attribute);
                break;
            case 5:
            case 6:
            case 7:
            case 8:
                draw_tvs_ru(xposition, yposition, symbol_type, attribute);
                break;
            case 9:
            case 10:
            case 11:
                draw_mda(xposition, yposition, symbol_type, attribute);
                break;
            default:
                if(verbose_flag)
                    fprintf(stderr,"WARNING, Packet 20 symbol type UNDIFINED\n");
                draw_undefined_symbol(xposition, yposition, symbol_type, attribute);
        }  /* end switch */
    

        /* move the pointer to the next block */
        product_data += point_feature_data_offset;
    
    } /* end for */
   
} /*end display_packet_20 */



void draw_meso_ru(int xpos, int ypos, int type, int attr) {

    int     center_pixel;
    int     center_scanl;
    float   x_scale, y_scale;
    int pixel;
    int scanl;
    
    int size;
    unsigned int width;

    int *prod_res_ind;    
    float base_res, prod_res;
    Prod_header *hdr;



    /* for meso (types 1 & 3) line width is 4 pixels */
    /* for 3d shear (types 2 & 4) line with is 1 pixel */
    /* we use 5 pixels and 2 pixels for today's hi resolution monitors */
    if((type == 1) || (type == 3))
         width = 5;
    else
         width = 2;
    
    /* set up display */

    /* This looks kludgy, however the Legacy rescale factor assumed a 0.54 NM*/
    /*   product displayed on a PUP                                          */
    /* In order to be able to display over other product resolutions : */
    
    /* resolution of the base image displayed */
    base_res = res_index_to_res(sd->resolution);   
        
    /* configured resolution of this product */
    hdr = (Prod_header *)(sd->icd_product);
    prod_res_ind = assoc_access_i(product_res, hdr->g.prod_id);
    prod_res = res_index_to_res(*prod_res_ind);

    /* for types 1-4, attribute is size */
    size = attr;
    
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
    


    /* CVG 9.0 - use new function for scale and center calculations */
    geo_scale_and_center(pwidth, pheight, sd->scale_factor, 
                     sd->x_center_offset, sd->y_center_offset, 
                     base_res, prod_res, &x_scale, &y_scale,
                                  &center_pixel, &center_scanl);
                                  
    
    /* a color palette has been defined for packet 20 and
       is associated by default in the palette_list file */
    XSetForeground(display, gc, display_colors[P20_YELLOW].pixel);

    XSetLineAttributes (display, gc, width, LineSolid, CapButt, JoinMiter);

    pixel = xpos*x_scale + center_pixel;
    scanl = ypos*y_scale + center_scanl;


    if((type == 3) || (type == 4)) {
        XDrawArc(display, sd->pixmap, gc, pixel-size, scanl-size, size*2, size*2, 
                   0, -(360*64));
    } else { /* coasted features (types 1 & 2) are segmented circles */
        XDrawArc(display, sd->pixmap, gc, pixel-size, scanl-size, size*2, size*2, 
                   55*64, 70*64);
        XDrawArc(display, sd->pixmap, gc, pixel-size, scanl-size, size*2, size*2, 
                   145*64, 70*64);
        XDrawArc(display, sd->pixmap, gc, pixel-size, scanl-size, size*2, size*2, 
                   235*64, 70*64);
        XDrawArc(display, sd->pixmap, gc, pixel-size, scanl-size, size*2, size*2, 
                   325*64, 70*64);
    }
    
    /* reset line width to 1 pixel */
    XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);
    XSetForeground(display, gc, white_color);
    
    
} /* end draw_meso_ru */



void draw_tvs_ru(int xpos, int ypos, int type, int attr) {
    
    int     center_pixel;
    int     center_scanl;
    float   x_scale, y_scale;
    int pixel;
    int scanl;
    XPoint  X[5];
    
    
    int *prod_res_ind;    
    float base_res, prod_res;
    Prod_header *hdr;



    /* set up display */

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
    
    /* a color palette has been defined for packet 20 and
       is associated by default in the palette_list file */
    XSetForeground(display, gc, display_colors[P20_RED].pixel);

    XSetLineAttributes (display, gc, 2, LineSolid, CapButt, JoinMiter);

    pixel = xpos*x_scale + center_pixel;
    scanl = ypos*y_scale + center_scanl;

    /* Extrapolated Data (either TVS or ETVS) */
    if((type == 5) || (type == 6)) {
    X[0].x = pixel;
    X[0].y = scanl;
    X[1].x = pixel;
    X[1].y = scanl-18;
    X[2].x = pixel+18;
    X[2].y = scanl;
    X[3].x = pixel;
    X[3].y = scanl;
    } 
       
    /* Normal / Updated Data (either TVS or ETVS) */
    if((type == 7) || (type == 8)) {
    X[0].x = pixel;
    X[0].y = scanl;
    X[1].x = pixel-6;
    X[1].y = scanl-19;
    X[2].x = pixel+6;
    X[2].y = scanl-19;
    X[3].x = pixel;
    X[3].y = scanl;
    }
    
    /* TVS */
    if((type == 5) || (type == 7))
        XFillPolygon(display, sd->pixmap, gc, X, 4, Convex, CoordModeOrigin); 
    /* ETVS */    
    if((type == 6) || (type == 8))
        XDrawLines(display, sd->pixmap, gc, X, 4, CoordModeOrigin); 


    /* reset line width to 1 pixel */
    XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);    
    XSetForeground(display, gc, white_color);
    
    
}  /* end draw_tvs_ru */



void draw_mda(int xpos, int ypos, int type, int attr) {
    

    int     center_pixel;
    int     center_scanl;
    float   x_scale, y_scale;
    int pixel;
    int scanl;
    
    XSegment seg[4];
    int numseg=4;
    int spike;
    
    int size;
    unsigned int width;

    int *prod_res_ind;    
    float base_res, prod_res;
    Prod_header *hdr;




    /* for strong MDA (types 9 & 10) line width is 4 pixels */
    /* for weaker MDA (type 11) line with is 1 pixel */
    /* we use 5 pixels and 2 pixels for today's hi resolution monitors */
    if((type == 9) || (type == 10))
         width = 5;
    else
         width = 2;
    
    /* set up display */

    /* This looks kludgy, however the Legacy rescale factor assumed a 0.54 NM*/
    /*   product displayed on a PUP                                          */
    /* In order to be able to display over other product resolutions : */
    
    /* resolution of the base image displayed */
    base_res = res_index_to_res(sd->resolution);   
        
    /* configured resolution of this product */
    hdr = (Prod_header *)(sd->icd_product);
    prod_res_ind = assoc_access_i(product_res, hdr->g.prod_id);
    prod_res = res_index_to_res(*prod_res_ind);
    
        /* for types 9-11, attribute is size */
        size = attr;
    
    
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
    


    /* CVG 9.0 - use new function for scale and center calculations */
    geo_scale_and_center(pwidth, pheight, sd->scale_factor, 
                     sd->x_center_offset, sd->y_center_offset, 
                     base_res, prod_res, &x_scale, &y_scale,
                                  &center_pixel, &center_scanl);
    
    /* a color palette has been defined for packet 20 and
       is associated by default in the palette_list file */
    XSetForeground(display, gc, display_colors[P20_YELLOW].pixel);  /* yellow */

    XSetLineAttributes (display, gc, width, LineSolid, CapButt, JoinMiter);

    pixel = xpos*x_scale + center_pixel;
    scanl = ypos*y_scale + center_scanl;

    
    XDrawArc(display, sd->pixmap, gc, pixel-size, scanl-size, size*2, size*2, 
                   0, -(360*64));                 
    


    if(type == 9) {  /* add spikes to thick yellow circle */
    
         /* length of spike is unspecified, 
            the following gives an attractive length */
         if(size < 10)
            spike = 6;
         else if(size < 12)
            spike = 7;
         else if(size < 15)
            spike = 8;  
         else if(size < 18)
            spike = 9; 
         else if(size < 22)
            spike = 10;
         else 
            spike = 11;
    
         seg[0].x1 = pixel+size;
         seg[0].y1 = scanl;
         seg[0].x2 = pixel+size+spike;
         seg[0].y2 = scanl;        
         seg[1].x1 = pixel-size;
         seg[1].y1 = scanl;
         seg[1].x2 = pixel-size-spike;
         seg[1].y2 = scanl;         
         seg[2].x1 = pixel;
         seg[2].y1 = scanl+size;
         seg[2].x2 = pixel;
         seg[2].y2 = scanl+size+spike;         
         seg[3].x1 = pixel;
         seg[3].y1 = scanl-size;
         seg[3].x2 = pixel;
         seg[3].y2 = scanl-size-spike;         
         
         XDrawSegments(display, sd->pixmap, gc, seg, numseg);   
    }
    
    
    /* reset line width to 1 pixel */
    XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);
    XSetForeground(display, gc, white_color);
    
    
} /* end draw_mda */



void draw_undefined_symbol(int xpos, int ypos, int type, int attr) {

    int     center_pixel;
    int     center_scanl;
    float   x_scale, y_scale;
    int pixel;
    int scanl;
    
    int size;
    

    int *prod_res_ind;    
    float base_res, prod_res;
    Prod_header *hdr;
    
    char ud_str[3] = "UD";
    
    /* provided in order to display a symbol for a new type not yet implemented */  
    
    
    /* set up display */

    /* This looks kludgy, however the Legacy rescale factor assumed a 0.54 NM*/
    /*   product displayed on a PUP                                          */
    /* In order to be able to display over other product resolutions : */
    
    /* resolution of the base image displayed */
        base_res = res_index_to_res(sd->resolution);   
        
    /* configured resolution of this product */
    hdr = (Prod_header *)(sd->icd_product);
    prod_res_ind = assoc_access_i(product_res, hdr->g.prod_id);
    prod_res = res_index_to_res(*prod_res_ind);

    /* for types 1-4, attribute is size */
    size = attr;

   

    /* CVG 9.0 - use new function for scale and center calculations */
    geo_scale_and_center(pwidth, pheight, sd->scale_factor, 
                     sd->x_center_offset, sd->y_center_offset, 
                     base_res, prod_res, &x_scale, &y_scale,
                                  &center_pixel, &center_scanl);


    pixel = xpos*x_scale + center_pixel;
    scanl = ypos*y_scale + center_scanl;

    /* a color palette has been defined for packet 20 and
       is associated by default in the palette_list file */
    XSetForeground(display, gc, display_colors[P20_BLACK].pixel);
    XSetBackground(display, gc, display_colors[P20_ORANGE].pixel);
    XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);
    /* The -5  +5  offset places the two character text over the point */
    XDrawImageString(display, sd->pixmap, gc, pixel-5, scanl+5, ud_str, 2);

    
    /* reset line width to 1 pixel */
    XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);
    XSetForeground(display, gc, white_color);
    XSetBackground(display, gc, black_color);
    
    
} /* end draw_undefined_symbol */

