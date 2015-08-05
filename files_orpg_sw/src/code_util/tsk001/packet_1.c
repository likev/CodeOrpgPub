/* packet_1.c */

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/08/19 15:11:42 $
 * $Id: packet_1.c,v 1.9 2009/08/19 15:11:42 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */
 
#include "packet_1_cvg.h"


void packet_1_skip(char *buffer,int *offset) 
{
    short length;
    /* LINUX change */
    /*length=read_half(buffer, offset); */
    length=read_half_flip(buffer, offset);
    MISC_short_swap(buffer+*offset,2);
    *offset += length;
}

void display_packet_1(int packet, int offset)
{
    short length;
    int   i, j=0, line=0;
    int pixel;
    int scanl;
    float x_scale, y_scale;
    /* CVG 9.0 - added for non_geo center to large screen */
    /*             and for geo_product support */
    int center_pixel, center_scanl;
    /* CVG 9.0 - added for geo_product support */
    int  *type_ptr, msg_type;
    int *prod_res_ind;    
    /* CGG 9.0 - added for geo_product support */
    float base_res, prod_res;
    Prod_header *hdr;
    /* CVG 9.1 - packet 1 coord override flag */
    int geo_coord_override;
    
    /* CVG 9.0 - added special handling of certain Geographic Precip products */
    int non_geo_do_not_center = FALSE;
    
    char  *p1_str;
    int   p1_i_start, p1_j_start;

    int font_height = 12;  /* THIS MAY NOT ALWAYS BE TRUE! */
                           /* Need to determine the real font height */

    /* USAGE NOTE: Packet 1 was initially used in non-geographic products.
     * Packet 1 is now also used in geographic products BUT THE x y 
     * COORDINATES ARE NOT GEOGRAPHIC AS STATED IN THE RPG Class 1 User
     * ICD.  The text is a non-geographic information area. This non-
     * standard use was initially in three precipitation products: 
     * DHR, USP, and DPA. It looks like it will continued to be used
     * in this "non-standard" manner.
     */
     
    /* Because the text data are not displayed geographically, no correction
     * for off-center display is required
     */


    /* CVG.9.0 - ADDED SUPPORT FOR GEOGRAPHIC_PRODUCT */
    
    hdr = (Prod_header *)(sd->icd_product);
    
    /* get the type of product we have here */
    type_ptr = assoc_access_i(msg_type_list, hdr->g.prod_id);
    if(type_ptr == NULL) {
        msg_type = NON_GEOGRAPHIC_PRODUCT; /* default */
    } else {
        msg_type = *type_ptr;
    }

    /* CVG 9.1 - get the value of packet 1 override flag */
    hdr = (Prod_header *)(sd->icd_product);
    type_ptr = assoc_access_i(packet_1_geo_coord_flag, hdr->g.prod_id);
    if(type_ptr == NULL) {
        geo_coord_override = 0; /* default - use 1/4 km for geo */
    } else {
        geo_coord_override = *type_ptr;
    }
    
    

    offset += 2;
    length = read_half(sd->icd_product, &offset);
    p1_i_start = read_half(sd->icd_product, &offset);
    p1_j_start = read_half(sd->icd_product, &offset);

    if(verbose_flag) {
        printf("Decoding Packet 1:\n");
    printf("Length of Data Block (in bytes) = %hd\n", length);
    printf("I Starting Point:                 %hd\n", p1_i_start);
    printf("J Starting Point:                 %hd\n", p1_j_start);
    printf("Message to follow:\n");
    }


    length -= 4;
    
    /* FOR SOME GEOGRAPHIC PRECIP PRODUCTS, NEED TO TREAT AS NON_GEOGRAPHIC */
    /* BECAUSE SCREEN COORDINATES WERE USED RATHER THAN 1/4 KM PIXELS.      */
/*    if( (hdr->g.prod_id == 56)  ||        */ /* USP - NULL product */
/*        (hdr->g.prod_id == 57)  ||        */ /* DHR - with product like a GAB */
/*        (hdr->g.prod_id == 108) ||        */ /* DPA - not meant for overlay */
/*        (hdr->g.prod_id == 138) ||        */ /* DSP - with product like a GAB */
/*                                          */ /* CVG 9.1 - added DP products: */
/*        (hdr->g.prod_id == 169) ||        */ /* OHA - NULL product */
/*        (hdr->g.prod_id == 170) ||        */ /* DAA - NULL product */
/*        (hdr->g.prod_id == 171) ||        */ /* STA - NULL product */
/*        (hdr->g.prod_id == 172) ||        */ /* DSA - NULL product */
/*        (hdr->g.prod_id == 173) ||        */ /* DUA - NULL product */
/*        (hdr->g.prod_id == 175)   ) {     */ /* DSD - NULL product */
/*                                          */
/*        non_geo_do_not_center = TRUE;     */
/*        msg_type = NON_GEOGRAPHIC_PRODUCT;*/
/*                                          */
/*    }                                     */
    
    /* FOR SOME GEOGRAPHIC PRECIP PRODUCTS, NEED TO TREAT AS NON_GEOGRAPHIC */
    /* BECAUSE SCREEN COORDINATES WERE USED RATHER THAN 1/4 KM PIXELS.      */
    /* CVG 9.1 - replaced hard-coded exceptions for using pixel coordinates */
    /* with geographic products with a configuration flag */
    if( (msg_type == GEOGRAPHIC_PRODUCT) && (geo_coord_override == 1) ) {
        
        non_geo_do_not_center = TRUE;
        msg_type = NON_GEOGRAPHIC_PRODUCT;
        
    }
    
    
    /* CVG 9.0 - used new function */
    if( (msg_type == NON_GEOGRAPHIC_PRODUCT) ) {
    
        /* CVG 9.0 - calculates x_scale, y_scale, center_pixel, center_scanl */
        non_geo_scale_and_center((float)pwidth, (float)pheight, 
                              &x_scale, &y_scale, &center_pixel, &center_scanl);
               
    /* CVG 9.0 added formal geographic product support */
    } else if(msg_type == GEOGRAPHIC_PRODUCT) {
        
        /* resolution of the base image displayed */
        base_res = res_index_to_res(sd->resolution);   
    
        /* configured resolution of this product */
        hdr = (Prod_header *)(sd->icd_product);
        prod_res_ind = assoc_access_i(product_res, hdr->g.prod_id);
        prod_res = res_index_to_res(*prod_res_ind);
        
        
        geo_scale_and_center(pwidth, pheight, sd->scale_factor, 
                             sd->x_center_offset, sd->y_center_offset, 
                             base_res, prod_res, &x_scale, &y_scale,
                                          &center_pixel, &center_scanl);
               
    } /* END if GEOGRAPHIC_PRODUCT */

    if(non_geo_do_not_center==TRUE) {
        pixel = p1_i_start * x_scale;
        scanl = p1_j_start * y_scale;
        
    } else { /* normal treatment */
        /* CVG 9.0 - added centering factor for large screens */
        pixel = p1_i_start * x_scale + center_pixel;
        scanl = p1_j_start * y_scale + center_scanl;
        
    } /* end normal treatment */

    if(verbose_flag)
        printf("plotting at (%d, %d)\n", pixel, scanl);



    if((p1_str = (char *) malloc( (size_t) length) ) == NULL)
      return;

/*DEBUG*/
/* fprintf(stderr,"DEBUG - display p1 - setting foreground color\n"); */

    XSetForeground(display, gc, display_colors[1].pixel);

    if(length <= 80) { /* single-line output */
    
        for(i=0; i<length; i++) {
        unsigned char c;
        c = read_byte(sd->icd_product, &offset);
        p1_str[i] = c;
        if(verbose_flag)
            printf("%c",c);
        }
        if(verbose_flag)
            printf("\n");
/*DEBUG*/
/* fprintf(stderr,"DEBUG - display p1 - drawing single line, line = %d\n", line);   */
  
         XDrawString(display, sd->pixmap, gc, pixel, scanl+12, p1_str, length);
    
    } else { /* multi-line output */

        /* read in text to display */
        for(i=0; i<length; i++) {
            unsigned char c;
            c = read_byte(sd->icd_product, &offset);
            p1_str[i] = c;
            if(verbose_flag)
                printf("%c",c);
                
            if(j==79) { /* draw a line */
                if(verbose_flag)
                    printf("\n");

                /* OUTPUT 80 CHAR LINE TO SCREEN */
                scanl += font_height;
/*DEBUG*/
/* fprintf(stderr,"DEBUG - display p1 - drawing multi-line, line = %d\n", line);     */
                
                XDrawString(display, sd->pixmap, gc, pixel, scanl+12, &p1_str[line*80], 80);
                j=0;
                line++;
                
            } else { /* not drawing a line */
                j++;
            }
        } /* end for */
        
        if(verbose_flag)
            printf("\n");
            
        if(j != 0) { /* one more line to draw */
/*DEBUG*/
/* fprintf(stderr,"DEBUG - display p1 - drawing last line, line = %d\n", line);     */

            scanl += font_height;
            
            XDrawString(display, sd->pixmap, gc, pixel, scanl+12, &p1_str[line*80], j);
        
        }
        
    } /* end else multi-line output */


/*DEBUG*/
/* fprintf(stderr,"DEBUG - display p1 - freeing text string\n");     */

    free(p1_str);
/*DEBUG*/
/* fprintf(stderr,"DEBUG - display p1 - end\n");   */
   
}

