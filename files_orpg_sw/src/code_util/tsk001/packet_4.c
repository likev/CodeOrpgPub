/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/08/19 15:11:43 $
 * $Id: packet_4.c,v 1.6 2009/08/19 15:11:43 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/* packet_4.c */

#include "packet_4.h"

void packet_4_skip(char *buffer,int *offset) 
{
    /*LINUX change*/
    /*short length = read_half(buffer, offset);*/
    short length = read_half_flip(buffer, offset);
    MISC_short_swap(buffer+*offset,length/2);
    *offset += length;
}

void display_packet_4(int packet,int offset) 
{
    short        length, color_level, x, y, dir, spd;
    float        x_scale, y_scale;
    /* CVG 9.0 */
    int center_pixel, center_scanl;

    /* CVG 9.0 - added for GEOGRAPHIC_PRODUCT support */
    int  *type_ptr, msg_type;
    Prod_header *hdr;
    int *prod_res_ind;    
    float base_res, prod_res;

    /* CVG 9.0 - reduced size of barb, changed 30.0 to 24.0 */
    /* CVG 9.0 - changed size of barb from 24.0 to 30.0 */
    static float barb_length = 30.0;
    
    XSegment     windbarb[256];
    int        num_segments;




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



    if(verbose_flag)
        printf("\nPacket 4: Wind Barb Data Packet\n");
    offset += 2;
    length = read_half(sd->icd_product, &offset);
    color_level = read_half(sd->icd_product, &offset);
    
    if(color_level<1 || color_level>5) {
        if(verbose_flag)
            printf("Data Error: Color level of %hd was out of range\n",color_level);
        return;
    }
    
    /* CVG 9.0 - added centering factor for large screens */
    x = read_half(sd->icd_product, &offset) * x_scale + center_pixel;
    y = read_half(sd->icd_product, &offset) * y_scale + center_scanl;
    dir = read_half(sd->icd_product, &offset);
    spd = read_half(sd->icd_product, &offset);

    if(verbose_flag)
        printf("Packet 4: Length=%4hd  Barb Color=%3hd  X Pos: %4hd  Y Pos: %4hd  Dir: %3hd  Speed: %3hd\n",
           length, color_level, x, y, dir, spd);

    if(dir >= 0) {

        XSetForeground(display, gc, display_colors[color_level].pixel);
    
        if (spd < 5) {
            XDrawArc (display, sd->pixmap, gc, x-2, y-2, 5, 5, 0, -360*64);
        } else {
            make_windbarb(windbarb, (float)spd, (float)dir, x, y, barb_length, &num_segments);
            XDrawSegments(display, sd->pixmap, gc, windbarb, num_segments);
        }
    
    } /* end if dir>=0 */
    
} /* end display_packet_4 */


#define RAD (3.14159265/180.0)

void make_windbarb(XSegment windbarb[], float wind_spd, float wind_dir, int firstx,
           int firsty, float leng, int *ipts)
{
    #define MFLAG   25.0       /*   metric  messurement for  */
    #define MFLAG1  23.75      /*   metric  messurement for  */
    #define MBARB    5.0       /*   metric  messurement for  */
    #define MBARB1   3.75      /*   metric  messurement for  */
    #define MHBARB   1.25      /*   metric  messurement for  */
    #define EFLAG   50.0       /*   english messurement for  */
    #define EFLAG1  47.75      /*   english messurement for  */
    #define EBARB   10.0       /*   english messurement for  */
    #define EBARB1   7.75      /*   english messurement for  */
    #define EHBARB   2.25      /*   english messurement for  */
 
    int    ibarb, iflag, ihalf, k, i;
    float  fval1, fval2, wspd, wdir1, wdir2, cs1, cs2, sn1, sn2;
    float  FLAG,FLAG1,BARB,BARB1,HBARB;

    static int english_units = 1;

    if (english_units)
    {
        FLAG  =  EFLAG;
        FLAG1 =  EFLAG1;
        BARB  =  EBARB;
        BARB1 =  EBARB1;
        HBARB =  EHBARB;
    }
    else
    {
        FLAG  =  MFLAG;
        FLAG1 =  MFLAG1;
        BARB  =  MBARB;
        BARB1 =  MBARB1;
        HBARB =  MHBARB;
    }

    /* CVG 9.1 - BUG FIX - CHANGED 100 KTS TO 196 KTS */
    if (wind_spd <= 0.0 || wind_spd >= 196.0)
    {
        *ipts = 0;
        return;
    }

    /* CVG 9.1 - added variable length barb based upon speed */
    if (wind_spd <= 110.0)
        leng = 24.0;
    else if(wind_spd <= 150)
        leng = 30.0;
    else
        leng = 36.0;
    
    
    wdir1          = RAD*wind_dir;
    wdir2          = RAD*(wind_dir+60.0);
    cs1            = cos(wdir1);
    cs2            = cos(wdir2);
    sn1            = sin(wdir1);
    sn2            = sin(wdir2);

    if(verbose_flag)
        printf("speed=%6.2f dir=%6.2f len=%6.2f cos1=%6.2f sin1=%6.2f\n",
           wind_spd, wind_dir, leng, cs1, cs2);     
           
    windbarb[0].x1 = firstx;
    windbarb[0].y1 = firsty;
    windbarb[0].x2 = windbarb[0].x1 + (leng*sn1);
    windbarb[0].y2 = windbarb[0].y1 - (leng*cs1);

    if(verbose_flag)
        printf("  SEGMENT:  (%d, %d) => (%d, %d)\n", windbarb[0].x1, windbarb[0].y1,
           windbarb[0].x2, windbarb[0].y2);
           

    i = 1;
    iflag = ibarb = ihalf = 0;

    wspd = wind_spd;
    
    for(k = 0; k < 5; k++) {
       if(wspd > FLAG1) {
          wspd = wspd - FLAG;
          iflag++;
       }
    }
    
    for(k = 0; k < 5; k++) {
       if(wspd > BARB1) {
          wspd = wspd - BARB;
          ibarb++;
       }
    }
    
    if (wspd > HBARB)
        ihalf = 1;   

    for (k = 0; k < iflag; k++) {
       /* CVG 9.0 - reduced size of barb, changed 5.0 to 4.0 */
       /* CVG 9.1 - changed size of barb from 4.0 to 4.5 */
       fval1          = leng - ((float)(i-1) * 4.0);
       fval2          = fval1 - 4.0;
       windbarb[i].x1 = windbarb[0].x1 + (fval1*sn1);
       windbarb[i].y1 = windbarb[0].y1 - (fval1*cs1);
       windbarb[i].x2 = windbarb[i].x1+(10.0*sn2);
       windbarb[i].y2 = windbarb[i].y1-(10.0*cs2);
       if(verbose_flag)
           printf("  SEGMENT:  (%d, %d) => (%d, %d)\n", windbarb[i].x1, windbarb[i].y1,
                                                 windbarb[i].x2, windbarb[i].y2);
       i++;
       windbarb[i].x1 = windbarb[0].x1 + (fval2*sn1);
       windbarb[i].y1 = windbarb[0].y1 - (fval2*cs1);
       windbarb[i].x2 = windbarb[i-1].x2;
       windbarb[i].y2 = windbarb[i-1].y2;
       if(verbose_flag)
           printf("  SEGMENT:  (%d, %d) => (%d, %d)\n", windbarb[i].x1, windbarb[i].y1,
                                                      windbarb[i].x2, windbarb[i].y2);
       i++;
    }
    
    fval2 = 10.0;
    for(k = 0; k < ibarb; k++) {
       /* CVG 9.0 - reduced size of barb, changed 5.0 to 4.0 */
       /* CVG 9.1 - changed size of barb from 4.0 to 4.5 */
       fval1          = leng - ((float)(i-1) * 4.0);
       windbarb[i].x1 = windbarb[0].x1 + (fval1*sn1);
       windbarb[i].y1 = windbarb[0].y1 - (fval1*cs1);
       windbarb[i].x2 = windbarb[i].x1+(fval2*sn2);
       windbarb[i].y2 = windbarb[i].y1-(fval2*cs2);
       if(verbose_flag)
       printf("  SEGMENT:  (%d, %d) => (%d, %d)\n", windbarb[i].x1, windbarb[i].y1,
          windbarb[i].x2, windbarb[i].y2);
       i++;
    }
    
    for (k = 0; k < ihalf; k++) {
       /* CVG 9.0 - reduced size of barb, changed 5.0 to 4.0 */
       /* CVG 9.1 - changed size of barb from 4.0 to 4.5 */
       fval1          = leng - ((float)(i-1) * 4.0);
       if (i == 1 && wspd < (HBARB * 2.0))
           fval1      = fval1 - 2.5;
       fval2          = 4.0;
       windbarb[i].x1 = windbarb[0].x1 + (fval1*sn1);
       windbarb[i].y1 = windbarb[0].y1 - (fval1*cs1);
       windbarb[i].x2 = windbarb[i].x1+(fval2*sn2);
       windbarb[i].y2 = windbarb[i].y1-(fval2*cs2);
       if(verbose_flag)
           printf("  SEGMENT:  (%d, %d) => (%d, %d)\n", windbarb[i].x1, windbarb[i].y1,
                                        windbarb[i].x2, windbarb[i].y2);
       i++;
    }
    
    *ipts = i;
    
    
} /* end make_windbarb */

