/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:43 $
 * $Id: packet_18.c,v 1.6 2009/05/15 17:52:43 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/* packet_18.c */

#include <math.h>
#include <Xm/Xm.h>
#include "packet_18.h"


void packet_18_skip(char *buffer,int *offset) 
{
    /*LINUX changes*/
    short length, num_row, i;
MISC_short_swap(buffer+*offset,4);
    *offset+=2*number_of_rows_offset-2;
    num_row = read_half(buffer, offset);
    for(i=0;i<num_row;i++) {
        /*length = read_half(buffer, offset);*/
        length = read_half_flip(buffer, offset);
    *offset += length;
    }
}

/*  the precip_rate_data structure is not malloc'd, only the */
/*  the data bins are malloc's.  The data are freed at the end  */
/*  of the display_packet_18 function, no delete function required */




/* CVG provides special support for packet 18:
 *
 * 1. This product should be displayed non-geographically in a
 *    manner similar to packet 17.  The grid size should be
 *    10 times the grid size of packet 17 (1/4 LFM vice 1/40 LFM).
 *
 * 2. Threshold label support would have to be hard-coded because DPA 
 *    uses the CVG legend configuration mechanism for packet 17. Rather
 *    than jury-rig a legend, the levels are simply printed in the blocks.
 *
 * 3. Color support must use the default associated palette which
 *    must be restricted to 8 colors (one color per data level).
 */
 
void display_packet_18(int packet, int offset)
{
    int num_halfwords_row, start_index, cell, i, j, k;
    unsigned short halfword;
    unsigned char run, color;
    short *product_data = (short *)(sd->icd_product);
    precip_rate_data precip_rate_image;
    int Color;
    int pixel;
    int scanl;
    int n_cols, n_rows;
    char buf[20];
    float th = 0.0;
    

    /* advance the pointer to the beginning of the packet */
    offset /= 2;
    product_data += offset;

    /* get the size of the table */
    precip_rate_image.number_of_columns = product_data[number_of_columns_offset];
    precip_rate_image.number_of_rows = product_data[number_of_rows_offset];

    if(verbose_flag) {
        printf("packet id=%d\n", product_data[0]);
    printf("number of rows=%d  number of columns=%d\n", 
           precip_rate_image.number_of_rows,
           precip_rate_image.number_of_columns);
    }

    /* Allocate memory for the raster data */
    for(i=0; i<precip_rate_image.number_of_rows; i++) {
        precip_rate_image.raster_data[i] = 
        malloc(precip_rate_image.number_of_columns*sizeof(short));
        assert( precip_rate_image.raster_data[i] != NULL );
    }

    start_index = p18_header_offset;
    for(i=0; i<precip_rate_image.number_of_rows; i++) {
        /* Get the number of halfwords this row */
        num_halfwords_row = product_data[ start_index ]/2;

        if(verbose_flag)
            printf("row number=%d  number of halfwords=%d\n", i, num_halfwords_row);
    
        /* initialize start_index and bin */
        start_index += p18_row_header_offset;
        cell = 0;
        
        for(j=0; j<num_halfwords_row; j++, start_index++) {
            halfword = (unsigned short)product_data[ start_index ];
      
            /* Extract first nibble of RLE data */
            run =  (halfword >> 8)/16;
            color = (halfword >> 8) & 0x0F;
            for( k = 0; k < run; k++, cell++ )
                    precip_rate_image.raster_data[i][cell] = color;
      
            /* Extract second nibble of RLE data */
            run = (halfword & 0xff)/16;
            color = (halfword & 0xff) & 0x0F;
            for( k = 0; k < run; k++, cell++ )
                    precip_rate_image.raster_data[i][cell] = color;
        } /*  end for number of halfwords */
        
    } /*  end for number of rows     */



    /***** display the raster data ********************************************/

    n_cols = precip_rate_image.number_of_columns;
    n_rows = precip_rate_image.number_of_rows;
    
    p18_g_size = 50;

    p18_orig_x = ( pwidth - (n_cols*p18_g_size) ) / 2;
    p18_orig_y = ( pheight - (n_rows*p18_g_size) ) / 2;    
    
    
    for (j=0; j<precip_rate_image.number_of_rows; j++) {
        if((j==7) && (verbose_flag)) {
            printf("row number %d:\n", j);
            for(k=0; k<precip_rate_image.number_of_columns;k++)
                printf("%d ", precip_rate_image.raster_data[j][k]);
            printf("\n");   
        }
        for(i=0; i<precip_rate_image.number_of_columns; i++) {
            Color = precip_rate_image.raster_data[j][i];
            pixel = p18_orig_x + (p18_g_size * i);
            scanl = p18_orig_y + (p18_g_size * j);   
                 
            XSetForeground(display, gc, display_colors[Color].pixel);
            XFillRectangle(display, sd->pixmap, gc, pixel, scanl, p18_g_size, p18_g_size);
            
            XSetForeground(display, gc, white_color);           
            switch (Color) {
                case 0: th = 0.0; break;
                case 1: th = 0.1; break;
                case 2: th = 0.3; break;
                case 3: th = 0.5; break;
                case 4: th = 1.0; break;
                case 5: th = 2.0; break;
                case 6: th = 4.0; break;
                case 7: th = 0.0; break;
            }
            if (Color==7)
                sprintf(buf, "ND");
            else
                sprintf(buf, "%1.1f", th);
            XDrawString(display, sd->pixmap, gc, pixel + 15, scanl + 30, buf, strlen(buf));

        } /*  end for number of columns */
    } /*  end for number of rows */

    XSetForeground(display, gc, white_color);

    for(k = 0; k <= n_rows; k++) /*  horizontal lines */
        XDrawLine(display, sd->pixmap, gc, p18_orig_x, p18_orig_y + (k * p18_g_size), 
                  p18_orig_x + (n_cols * p18_g_size), p18_orig_y + (k * p18_g_size));

    for(k = 0; k <= n_cols; k++) /*  vertical lines */
        XDrawLine(display, sd->pixmap, gc, p18_orig_x + (k * p18_g_size), p18_orig_y, 
                  p18_orig_x + (k * p18_g_size), p18_orig_y + (n_rows * p18_g_size));


    /* cleanup */
    for(i=0; i<precip_rate_image.number_of_rows; i++)
        free(precip_rate_image.raster_data[i]); 

}




