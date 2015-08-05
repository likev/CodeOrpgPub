/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:39 $
 * $Id: overlay.c,v 1.10 2009/05/15 17:52:39 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */
/* overlay.c */
/* functions for putting informational overlays on top of the final image */

#include "overlay.h"



/* CVG 9.0 */
/*  used in draw_az_lines() and draw_range_rings() */
/***
static FILE *the_palette_file=NULL;
static char filename[256]; 
***/  
static int use_cvg_colors = FALSE;  





/* puts up azimuth lines every 45 degrees */
void draw_az_lines(int screen)
{
int x_ctr_offset=0,  y_ctr_offset=0;  /*  CVG 8.3 */

int retval;

screen_data *azsd=NULL;
    
    
 
    if(screen == SCREEN_1) {
        azsd = sd1;
        
    } else if(screen == SCREEN_2) {
        azsd = sd2;
        
    } else if(screen == SCREEN_3) {
        azsd = sd3;
    }
    
    
if(azsd->icd_product != NULL) { 
    x_ctr_offset = - (int) (azsd->x_center_offset * azsd->scale_factor);
    y_ctr_offset = - (int) (azsd->y_center_offset * azsd->scale_factor); 
}     
    


/*  EXISTING BUG:  When the product is zoomed and offset so that the radar is     */
/*           offthe drawing area, the drawing of azimuth lines labels breaks down.*/
/*  CVG 8.5 WORKAROUND: Stop display if center_pixel AND center_scanl are negative*/
/*                      or exceed the dwawing area.                               */
    if( (x_ctr_offset < -pwidth)/2 || (x_ctr_offset > pwidth/2) || 
        (y_ctr_offset < -pheight/2) || (y_ctr_offset > pheight/2) )
        return; 

    /* CVG 9.0 */
    /*  setup azimuth line palette file here */
    /*  text_1.plt     */
    /* 0-black 1-white */

    retval = open_default_palette(AZIMUTH_LINE_RANGE_RING);
    
    if(retval==FALSE) {
        use_cvg_colors = TRUE;
        fprintf(stderr,"Using internal colors for azimuth line display");
    }

    if(use_cvg_colors == TRUE) 
         XSetForeground(display, gc, white_color);
    else
         XSetForeground(display, gc, display_colors[1].pixel);
    XSetLineAttributes (display, gc, 1, LineSolid, 
                                 CapButt, JoinMiter); 



/*  VERSION 2 AVOID GAB AND DESCRIPTION AREAS */
/*     XDrawLine(display, azsd->pixmap, gc, 0.09*pwidth,  */
/*                                       0.09*pheight, */
/*                                 pwidth-0.09*pwidth,  */
/*                               pheight-0.09*pheight);  */
/*     XDrawLine(display, azsd->pixmap, gc, 0.09*pwidth,  */
/*                               pheight-0.09*pheight,  */
/*                                 pwidth-0.09*pwidth,  */
/*                                       0.09*pheight);  */
/*  */

/*  VERSION 3 OFF CENTER DISPLAY */
    /* diagonal lines (could use a better fudge factor than 460) */
    XDrawLine(display, azsd->pixmap, gc, (pwidth + x_ctr_offset) + 460, 
                                      (pheight + y_ctr_offset) + 460,
                                       (0 + x_ctr_offset) - 460, 
                                      (0 + y_ctr_offset)  - 460); 
    XDrawLine(display, azsd->pixmap, gc, (pwidth + x_ctr_offset) + 460 , 
                                      (0 + y_ctr_offset) - 460, 
                                       (0 + x_ctr_offset) - 460, 
                                      (pheight + y_ctr_offset) + 460); 



    /* horizontal lines */
    XDrawLine(display, azsd->pixmap, gc, pwidth/2 + x_ctr_offset, 
                                              0, 
                                       pwidth/2 + x_ctr_offset, 
                                        pheight);
    XDrawLine(display, azsd->pixmap, gc, 0, 
                               pheight/2 + y_ctr_offset, 
                                  pwidth, 
                               pheight/2 + y_ctr_offset);
    
    /* we want white lines */
    XSetForeground(display, gc, white_color);
    XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);
    
    
} /* end draw_az_lines */



/* draws range rings with labels */
void draw_range_rings(int screen)
{
    int i; 
    float ring_inc;   /* ring spacing in NM */ 
    float bin_res, r;
    float screen_scale;   /* NM per pixel */
    int  center_pixel;
    int  center_scanl;
    char buf[10];
    
    screen_data *rrsd=NULL;
    
    int retval;
    
    
    
    if(screen == SCREEN_1) {
        rrsd = sd1;
        
    } else if(screen == SCREEN_2) {
        rrsd = sd2;
        
    } else if(screen == SCREEN_3) {
        rrsd = sd3;
    }

    center_pixel = pwidth/2;  /* initialized to center of canvas */
    center_scanl = pheight/2; /* initialized to center of canvas */

    /* correct center pixel for the off_center value (if any) */
    if(rrsd->icd_product != NULL) { /*  CVG 8.3 */
        center_pixel = center_pixel - (int) (rrsd->x_center_offset * rrsd->scale_factor);  
        center_scanl = center_scanl - (int) (rrsd->y_center_offset * rrsd->scale_factor);   
    } 
/* TEST */
/* fprintf(stderr,"TEST - display rings, x center is %d, y center is %d\n", */
/*                               center_pixel, center_scanl); */
/* fprintf(stderr,"TEST -              x offset is %d, y offset is %d, zoom is %f\n", */
/*                      rrsd->x_center_offset, rrsd->y_center_offset, rrsd->scale_factor); */

    Boolean norm_format_flag = False;
    Boolean bkgd_format_flag = False;
    
    if(rrsd==sd1) {
        norm_format_flag = norm_format1;
        bkgd_format_flag = bkgd_format1;
    
    } else if(rrsd==sd2) {
        norm_format_flag = norm_format2;
        bkgd_format_flag = bkgd_format2;
    
    } else if(rrsd==sd3) {
        norm_format_flag = norm_format3;
        bkgd_format_flag = bkgd_format3;
    }

    /* if no product displayed, use inherent resolution otherwise */
    /* if resolution is zero, it is either unknown or N/A         */
    /*       we do not display range rings                        */
    
    /* if resolution is 999, it is an overlay product with no underlying base product */
    /*       we assume the overlay product has the standard inherent resolution       */

    /*  get resolution from gen_radial */
    /*  TO DO - DO I NEED TO LIMIT TO SIG DIGITS??? */
    if(sd->last_image == GENERIC_RADIAL) {
        bin_res = sd->gen_rad->range_interval / 1000 * CVG_KM_TO_NM;
    } else  {
        bin_res = res_index_to_res(rrsd->resolution);
    }
    
    
    
    if(rrsd->icd_product == NULL) /*  CVG 8.3 */
        bin_res = 0.54;
    else if(bin_res == 0.0) /* cannot display range without resolution information */
         return;
    
    
    if(bin_res == 999) {/* if overlay product displayed at root level, use inherent resolution */
         bin_res = 0.54;
         fprintf(stderr, "DISPLAY RINGS - Overlay Product Displayed at Base Level.\n");
         fprintf(stderr, "                Standard 0.54 NM resolution assumed.\n");
    }    
    
    
    /* detemine an appropriate distance / increment between rings (in NM) */
    /* based upon screen_scale, which is  NM/pixel  */
    screen_scale = bin_res / rrsd->scale_factor;

    /* Adjust for new higher zoom factors (+16 and +32) */
    /*         new lower zoom factors are OK. */
    if(screen_scale < 0.02)
         ring_inc = 2.0;
    else if(screen_scale < 0.05)
         ring_inc = 5.0;
/*     if(screen_scale < 0.15) */
    else if(screen_scale < 0.12)
         ring_inc = 10.0;
    else if(screen_scale < 0.30)
         ring_inc = 25.0;
    else if(screen_scale < 0.70)
         ring_inc = 50.0;
    else if(screen_scale < 1.50)
         ring_inc = 100.0;
    else 
         ring_inc = 200.0;
 

    
    /* CVG 9.0 */
    /*  setup range ring palette file here */
    /*  text_1.plt     */
    /* 0-black 1-white */
    
    retval = open_default_palette(AZIMUTH_LINE_RANGE_RING);
    
    if(retval==FALSE) {
        use_cvg_colors = TRUE;
        fprintf(stderr,"Using internal colors for range ring display");
    }

    if(use_cvg_colors == TRUE) 
         XSetForeground(display, gc, white_color);
    else
         XSetForeground(display, gc, display_colors[1].pixel);
    XSetLineAttributes (display, gc, 1, LineSolid, 
                                 CapButt, JoinMiter); 


    
    /* regardless of the actual image _size_, we want to draw rings until just before
     * they run off the edges of the "digital canvas"
     */
     /* FUTURE COSMETIC IMPROVEMENT:
      * if you want rings in the corners of a zoomed product without going through
      * the legend, then draw rings to a larger radius before displaying the GAB or
      * the legend, and then draw 'black' before the GAB and the legend */

/*  EXISTING BUG:  When the product is zoomed and offset so that the radar is off */
/*                 the drawing area, the drawing of range rings and placement of */
/*                 labels breaks down.   */
/*  CVG 8.5 WORKAROUND: Stop display if center_pixel AND center_scanl are negative */
/*                      or exceed the dwawing area. */
    if( (center_scanl < 0) || (center_scanl > pheight) || 
        (center_pixel < 0) || (center_pixel > pwidth) )
        return;
      
    for(i=1; TRUE; i++) {       
        r = (float)(i*ring_inc) / screen_scale;  /* calculate the radius in pixels */

        if ( (r * screen_scale)  > 460)
             break;
        XDrawArc(display, rrsd->pixmap, gc, center_pixel-(int)r, center_scanl-(int)r,
                                               (int)(2*r), (int)(2*r), 0, -(360*64));

        sprintf(buf, "%d", i*(int)ring_inc);
        
        if(norm_format_flag==TRUE) {
            XDrawString(display, rrsd->pixmap, gc, center_pixel, 
                        center_scanl-(int)r, buf, strlen(buf));
            XDrawString(display, rrsd->pixmap, gc, center_pixel, 
                        center_scanl+(int)r, buf, strlen(buf));
            XDrawString(display, rrsd->pixmap, gc, center_pixel-(int)r, 
                        center_scanl, buf, strlen(buf));
            XDrawString(display, rrsd->pixmap, gc, center_pixel+(int)r,
                        center_scanl, buf, strlen(buf));
        } else if(bkgd_format_flag==TRUE) {
            XDrawImageString(display, rrsd->pixmap, gc, center_pixel, 
                             center_scanl-(int)r, buf, strlen(buf));
            XDrawImageString(display, rrsd->pixmap, gc, center_pixel, 
                            center_scanl+(int)r, buf, strlen(buf));
            XDrawImageString(display, rrsd->pixmap, gc, center_pixel-(int)r, 
                             center_scanl, buf, strlen(buf));
            XDrawImageString(display, rrsd->pixmap, gc, center_pixel+(int)r, 
                             center_scanl, buf, strlen(buf));           
        }
        
    } /* end for */

    XSetForeground(display, gc, white_color);
    XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);


} /* END draw_range_rings */





