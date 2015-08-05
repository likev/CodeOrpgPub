/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:50 $
 * $Id: packet_8.c,v 1.10 2009/05/15 17:52:50 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */
/* packet_8.c */

#include "packet_8_cvg.h"

void packet_8_skip(char *buffer,int *offset) 
{
    /*LINUX change*/
    /*short length = read_half(buffer, offset);*/
    short length = read_half_flip(buffer, offset);
    MISC_short_swap(buffer+*offset,3);
    *offset += length;
}

/* This packet has four basic uses:
 *    1. In the GAB portion of a Geographic Product (primary use)
 *    2. In a non-geographic product (VAD Wind Profile - ID 35)
 *    3. In a 2.2 NM resolution Geographic Product (SWP - ID 63)
 *    4. In recent overlay products, as a label for features
 *
 * display_packet_8() handles all cases
 */

void display_packet_8(int packet,int offset) 
{
    short length;
    int   i;
    int pixel;
    int scanl;
    float x_scale, y_scale;
    char  p8_str[150];
    int   p8_i_start, p8_j_start, p8_color;
    int center_pixel, center_scanl;
    int  *type_ptr, msg_type;
    Prod_header *hdr;

    int *prod_res_ind;    
    float base_res, prod_res=-1.0;

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



    offset += 2;
    length = read_half(sd->icd_product, &offset);
    p8_color   = read_half(sd->icd_product, &offset);
    p8_i_start = read_half(sd->icd_product, &offset);
    p8_j_start = read_half(sd->icd_product, &offset);

    if(verbose_flag) {
        printf("Decoding Packet 8:\n");
        printf("Length of Data Block (in bytes) = %hd\n",length);
        printf("Color Level of Text:              %hd\n",p8_color);
        printf("I Start Point:%hd   J Start Point:%hd\n",p8_i_start, p8_j_start);
        printf("Text Data: ");       
    }            
                                  
    length -= 6; 
                 
    for(i=0;i<length;i++) {
        unsigned char c;
        c = read_byte(sd->icd_product, &offset);
        p8_str[i] = c;
        
        if(verbose_flag)
            printf("%c",c);
    }
    if(verbose_flag)
        printf("\n");

    
    /* get the type of product we have here */
    hdr = (Prod_header *)(sd->icd_product);
    type_ptr = assoc_access_i(msg_type_list, hdr->g.prod_id);
    if(type_ptr == NULL) {
        msg_type = NON_GEOGRAPHIC_PRODUCT; /* default */
    } else {
        msg_type = *type_ptr;
    }

    
    
    /* When used for the GAB (the typical application of this packet), the 
     * scale factors must account for differences between the PUP and 
     * X-windows graphics and character drawing, so the x_scale looks strange
     */
    if(printing_gab == 1) {
        p8_color = 1;  /* make sure it's white, the GAB in some products use 0 (black) */
        /* x_scale = 1.0; */
        x_scale = 0.86;
        y_scale = 2.0;
        center_pixel = 0; /* not used */
        center_scanl = 0; /* not used */
        
    } else { /*  everything else */
        /* Geographic product, fit it to a 116x116 product (for SWP ID 63) 
        thie use for this packet in a 2.2 NM resolution product */           
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

    } /*  end everything else */

    if(printing_gab == 1) /* line up text with boxes */
        pixel = p8_i_start * x_scale + center_pixel + 2;
    else
        pixel = p8_i_start * x_scale + center_pixel;
        
        
    /* this is where the baseline is, so we need to adjust it downward to 
     * account for the font's height when we plot it */
    scanl = p8_j_start * y_scale + center_scanl;  

    /* set the color and put it up */
    XSetForeground(display, gc, display_colors[p8_color].pixel);

    
    if(prod_res == 999) {       
    /* text packet configured as an overlay */
    
        if(norm_format_flag==TRUE)
            XDrawString(display, sd->pixmap, gc, pixel, scanl+12, p8_str, length);
        else if(bkgd_format_flag==TRUE)
            XDrawImageString(display, sd->pixmap, gc, pixel, scanl+12, p8_str, length);
        
    } else  {  /* text packet not used in overlay */

        XDrawString(display, sd->pixmap, gc, pixel, scanl+12, p8_str, length);
       
    } 
       
}




