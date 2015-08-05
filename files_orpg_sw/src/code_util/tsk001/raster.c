/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:55 $
 * $Id: raster.c,v 1.9 2009/05/15 17:52:55 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */
/* raster.c */

#include <math.h>
#include <Xm/Xm.h>
#include "raster.h"



void packet_BA07_skip(char *buffer,int *offset)
{
    /*LINUX changes*/
    short length, num_row, i;
    MISC_short_swap(buffer+*offset,10);/* I think this is right.. */

    *offset+=16;
    num_row = read_half(buffer, offset);
    *offset += 2;  /*skip over packing descriptor*/

    for(i=0; i<num_row; i++) {
        length = read_half_flip(buffer, offset);
        *offset += length;
    }

}




void display_raster(int packet, int offset, int replay)
{
    
    /* only decode when not replaying from history */
    if(replay == FALSE)
        decode_raster((short *)(sd->icd_product), offset);
    
    
    output_raster();

}




void decode_raster(short *product_data, int offset)
{

    int num_halfwords_row, start_index, cell, i, j, k;
    int error, some_errors;
    
    unsigned char run, color;
    unsigned char *bytesize_product_data=(unsigned char *)product_data;
    unsigned char byte;
   
    offset /= 2;
 
     /* Allocate memory for the raster_rle struture */
    sd->raster = malloc( sizeof(raster_rle) );
    assert( sd->raster != NULL );

    /* advance the pointer to the beginning of the packet */
    product_data += offset;
    /* Extract the i,j start coordinates, the x,y scale factors, and i
     * number of rows */
    sd->raster->x_start = product_data[ i_start_offset ];
    sd->raster->y_start = product_data[ j_start_offset ];
    sd->raster->x_scale = product_data[ x_scale_offset ];
    sd->raster->y_scale = product_data[ y_scale_offset ];
    sd->raster->number_of_rows = product_data[ number_of_rows_offset ];

    /* run through the first column to see how long it is
     * we assume that all columns are the same length
     * (or at least no longer than the first)
     */
    
    start_index = raster_rle_offset;
    num_halfwords_row = product_data[ start_index ]/2;
    start_index += raster_rle_header_offset;

    bytesize_product_data=(unsigned char *)product_data;

    /* run through the first column to see how long it is
     * we assume that all columns are the same length
     * (or at least no longer than the first)
     */
    sd->raster->number_of_columns = 0;

    for(j=0; j<num_halfwords_row; j++, start_index++) {
        
        /* Extract first nibble of RLE data */
        byte=bytesize_product_data[start_index*2];
        run=byte/16;

        for( k = 0; k < run; k++ ) {
            sd->raster->number_of_columns++;
        } /*  end for run */
        
        /* Extract second nibble of RLE data */
        byte=bytesize_product_data[start_index*2+1];
        run=byte/16;
 
        for( k = 0; k < run; k++ ) {
            sd->raster->number_of_columns++; 
        } /*  end for run */
    
    } /*  end for num_halfwords_row */


    if(verbose_flag) {
        printf("packet id=%hx  op flags=%x %x\n", product_data[0], 
           product_data[1], product_data[2]);
        printf("i start offset=%d  j start offset=%d\n", 
           sd->raster->x_start, sd->raster->y_start);
        printf("x scale offset=%d  y scale offset=%d\n", 
           sd->raster->x_scale, sd->raster->y_scale);
        printf("number of rows = %d  columns=%d\n", sd->raster->number_of_rows,
           sd->raster->number_of_columns);
    }


    /* Allocate memory for the raster data */
    sd->raster->raster_data = malloc(sd->raster->number_of_rows * sizeof(short *));
    assert( sd->raster->raster_data != NULL );
    
    for(i=0; i<sd->raster->number_of_rows; i++) {
        sd->raster->raster_data[i] = 
                            malloc( sd->raster->number_of_columns*sizeof(short) );
        assert( sd->raster->raster_data[i] != NULL );
    }


    /* now we process the rle data array */
    start_index = raster_rle_offset;
    
    some_errors = FALSE;

    /*   FUTURE ENHANCEMENT: Include looking for beginning of next row and/or */
    /*                       end of data packet for consistency check */

    /* for each row */
    for(i=0; i<sd->raster->number_of_rows; i++) {
        /* Get the number of halfwords this row */
        num_halfwords_row = product_data[ start_index ]/2;
       
        /* initialize start_index and bin */
        start_index += raster_rle_header_offset;
        cell = 0;
        
        error=FALSE;

        /* for each halfword of rle data */
        for(j=0; j<num_halfwords_row; j++, start_index++) {
    
            /* Extract first nibble of RLE data */
            byte=bytesize_product_data[start_index*2];
            run=byte/16;
            color=byte&0x0f;
            /*fprintf(stderr, "run = %d val = %d\n", run, color);*/

            if(error == FALSE) {
            
                for( k = 0; k < run; k++, cell++ ) {
                    
                    if(cell < sd->raster->number_of_columns) { 
                        sd->raster->raster_data[i][cell] = color;
                    
                    } else { 
                        if(verbose_flag) fprintf(stderr, 
                                "DATA ERROR (L1) Packet BA0F, Row %d - number of "
                                "bins exceeds number columns in first row %d\n"
                                "           HW per row is %d, reading RLE HW %d\n",
                                i+1, sd->raster->number_of_columns, 
                                              num_halfwords_row, j+1);
                        error=TRUE;  
                        some_errors=TRUE; 
                        break; 
                    }
                    
                } /*  end for run */
            
            } /*  end if error FALSE */
            
            /* Extract second nibble of RLE data */
            byte=bytesize_product_data[start_index*2+1];
            run=byte/16;
            color=byte&0x0f;
            /*fprintf(stderr, "run = %d val = %d\n", run, color);*/

            if(error == FALSE) {
            
                for( k = 0; k < run; k++, cell++ ) {
                    
                    if(cell < sd->raster->number_of_columns) { /*  CVG 8.2 change */
                        sd->raster->raster_data[i][cell] = color;
                    
                    } else { 
                        if(verbose_flag) fprintf(stderr, 
                                "DATA ERROR (L1) Packet BA0F, Row %d - number of "
                                "bins exceeds number columns in first row %d\n"
                                "           HW per row is %d, reading RLE HW %d\n",
                                i+1, sd->raster->number_of_columns, 
                                              num_halfwords_row, j+1);
                        error=TRUE;  
                        some_errors=TRUE; 
                        break; 
                    }
                    
                } /*  end for run */

            } /*  end if error FALSE */

        } /*  end for num_halfwords_row */
    
    } /*  end for number_of_rows */


    if(some_errors == TRUE) 
        fprintf(stderr,"\nWARNING - PRODUCT ERROR - number of bins  "
                                                  "in at least one row exceeds \n"
                       "          the number of columns in the first row. "
                                                  "Verbose Output for details.\n");


}




void delete_raster()
{
    int i;

    if(verbose_flag)            
        fprintf(stderr, "*****ENTER delete_raster*****\n");  

    if(sd->raster != NULL) {

        for(i=0; i<sd->raster->number_of_rows; i++)
            free(sd->raster->raster_data[i]);
        free(sd->raster);
        sd->raster = NULL;
        sd->last_image=NO_IMAGE;
    
    } else 
      fprintf(stderr, "ERROR - attempted to delete raster rle with NULL structure!\n");     
    
}



void output_raster()
{
    int pixs=1, scans=1;
    int i;
    int j;
    int     k;
    int Color;
    int pixel;
    int scanl;
    int     pixel_start;
    int     scanl_start;
    int     *type_ptr, msg_type;
    
    Prod_header *hdr;

    
    /* get the type of product we have here */
    hdr = (Prod_header *)(sd->icd_product);
    type_ptr = assoc_access_i(msg_type_list, hdr->g.prod_id);
    if(type_ptr == NULL) {
        msg_type = GEOGRAPHIC_PRODUCT; /* default */
    } else {
        msg_type = *type_ptr;
    }

    /* if the product is a non-geographic one, then try to draw the
     * raster product like the PUP would (WWPD?)
     */
    if(msg_type == NON_GEOGRAPHIC_PRODUCT) {
        /* CVG 9.0 */
        float x_scale=1.0, y_scale=1.0;
        int center_pixel=0, center_scanl=0;
        
        /* CVG 9.0 - calculates x_scale, y_scale, center_pixel, center_scanl */
        non_geo_scale_and_center((float)pwidth, (float)pheight, 
                              &x_scale, &y_scale, &center_pixel, &center_scanl);

        /* before CVG 9.0 the combined moment product required special handling */
        /* CVG 9.0 - new logic. after correct calculation of x_scale */
        /*  and y_scale all fudge factors were removed */
            
        pixel_int = (float)sd->raster->x_scale * x_scale;
        scanl_int = (float)sd->raster->y_scale * y_scale;
        /* CVG 9.0 - added centering factor for large screens */
        pixel_start = (float)(sd->raster->x_start) * x_scale + center_pixel;
        scanl_start = (float)(sd->raster->y_start) * y_scale + center_scanl; 
        
    } else { /* a geographic product */
        
        /*  we do the standard one bin-one pixel with scaling factor (zoom & res) */
        pixel_int = sd->scale_factor;
        scanl_int = sd->scale_factor;
        
        /* this centers the raster product on the CVG drawable screen */
        pixel_start = (pwidth - sd->raster->number_of_columns * pixel_int) / 2;
        scanl_start = (pwidth - sd->raster->number_of_rows * scanl_int) / 2;
        
        /* CVG 9.0 - moved inside of condional */
        pixel_start = pixel_start - (int) (sd->x_center_offset * sd->scale_factor);
        scanl_start = scanl_start - (int) (sd->y_center_offset * sd->scale_factor);
        
    }

    /* 1 must be added for the X-windows fill rectangle to work properly */
    pixs      = pixel_int + 1; 
    scans     = scanl_int + 1; 

    sd->last_image = RASTER_IMAGE;



    for (j=0;j<sd->raster->number_of_rows;j++) {
        if((j==99) && (verbose_flag)){
            printf("row number %d:\n", j+1);
            for(k=0; k<sd->raster->number_of_columns; k++)
                printf("%d ", sd->raster->raster_data[j][k]);
            printf("\n");   
        }
        
        for (i=0; i<sd->raster->number_of_columns; i++) {
            Color = sd->raster->raster_data[j][i];
            
            /*  THE FOLLOWING ASSUMES THAT COLOR 0 IS BACKGROUND (BLACK or WHITE) */
            /*  FUTURE CHANGE - If Color == 0, set color = Assigned Background  */

            if (Color > 15) {
                printf ("ERROR: Value at cell (%i,%i) = %i\n", i, j, 
                sd->raster->raster_data[j][i]);
            } else {
                pixel = pixel_start + i*pixel_int;
                scanl = scanl_start + j*scanl_int;
                XSetForeground(display, gc, display_colors[Color].pixel);
                XFillRectangle(display, sd->pixmap, gc, pixel, scanl, pixs, scans);
            }

        } /*  end for number_of_columns */
        
    } /*  end for number_of_rows */



}

