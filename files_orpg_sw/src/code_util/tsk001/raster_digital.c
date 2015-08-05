/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:47:37 $
 * $Id: raster_digital.c,v 1.4 2008/03/13 22:47:37 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/* raster_digital.c */

#include <math.h>
#include <Xm/Xm.h>
#include "raster_digital.h"


void digital_raster_skip(char *buffer,int *offset) 
{
	/*LINUX changes*/
    short length, num_row, i;
MISC_short_swap(buffer+*offset,7);
    /**offset+=2;*/
    num_row = read_half(buffer, offset);
    printf("* number of rows to skip = %d\n", num_row);
    for(i=0;i<num_row;i++) {
        /*length = read_half(buffer, offset);*/
        length = read_half_flip(buffer, offset);
	*offset += length-2;/* length must include itself in this packet.. how consistent */
    }
}

void display_digital_raster(int packet, int offset)
{
    int i, j;
    short *product_data = (short *)(sd->icd_product);
    char *data_bins;
    float	pixel_int;
    float	scanl_int;
    int	pixs;
    int	Color;
    int	pixel;
    int	scanl;
    int num_rows;
    int num_cols;
    int row_offset;
    int color_scale;
    
    /* set up for display */

    pixel_int = sd->scale_factor;
    scanl_int = sd->scale_factor;
    pixs      = pixel_int + 1;

    /* advance the pointer to the beginning of the packet */
    offset /= 2;
    product_data += offset;

    /* get the size of the table */
    num_rows = product_data[1];

    /* we assume that the product has 256 data levels, so we try to fit
     * those levels into however many colors we have availible
     */
    color_scale = 256 / palette_size;

    if(verbose_flag) {
        printf("packet id=%d\n", product_data[0]);
	printf("number of rows=%d\n", num_rows);
    }

    row_offset = 2;
    /* display the raster data */
    for (i=0; i<num_rows; i++) {
        num_cols = product_data[row_offset++] - 2;
	data_bins = (char *)&(product_data[row_offset]);
	row_offset += num_cols/2;

	/*printf("number of columns = %d\n", num_cols);*/

	for(j=0; j<num_cols; j++) {
	    Color = data_bins[j] / color_scale;
	    pixel = i*pixel_int;
	    scanl = pwidth/8 + j*scanl_int;
	
	    /*    
	    if(Color != 0)
	        printf("data = %d at (%d, %d)\n", Color, i, j);
		*/
	    XSetForeground(display, gc, display_colors[Color].pixel);
	    XFillRectangle(display, sd->pixmap, gc, pixel, scanl, pixs, pixs);
	}
    }

}




