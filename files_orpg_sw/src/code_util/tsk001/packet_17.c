/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:42 $
 * $Id: packet_17.c,v 1.6 2009/05/15 17:52:42 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/* packet_17.c */

#include <math.h>
#include <Xm/Xm.h>
#include "packet_17_cvg.h"


void packet_17_skip(char *buffer,int *offset) 
{
    unsigned char run;
    int on_linux = FALSE;  
      
    /*LINUX changes*/
    short length, num_row, i;
    MISC_short_swap(buffer+*offset,4);
    *offset += 2*number_of_rows_offset - 2;
    num_row = read_half(buffer, offset);

    

    /*  the following test is because early Linux versions of the ORPG */
    /*  had a byte-swap error reversing the run and level values */
    /*  if the first run is 255 (a level meaning outside the coverage area) */
    /*  the values are reversed */
#ifdef LITTLE_ENDIAN_MACHINE
    on_linux=TRUE;
#endif


    run = (unsigned char) buffer[*offset+2];


    /*  LINUX NOTE: Because the function decode_packet_17() reads all portions of the */
    /*              packet data as an array of shorts, to avoid swapping during the */
    /*              the display logic, on Linux we actually swap the run and level */
    /*              data out of postion while also swapping the radial length field */
    /*  FUTURE IMPROVEMENT: Treat this packet like packet 16. Don't swap out of place */
    /*              in the skip function and cast product data to an (unsigned char *) */
    /*              in the decode function reading the data as bytes */

    if(on_linux==TRUE) { /*  on a Linux platform */

        if(run==255) {  /*  values in incorrect postions so we only swap the length */
        
            fprintf(stderr,"* ERROR ******************************************\n");
            fprintf(stderr,"*   DPA Product Packet 17 Error (Linux platform) *\n");
            fprintf(stderr,"*   Run and Level values are incorrectly placed. *\n");
            fprintf(stderr,"*   CVG is swapping run and level so the product *\n");
            fprintf(stderr,"*   can be displayed.                            *\n");
            fprintf(stderr,"**************************************************\n");
           
            for(i=0; i<num_row; i++) {
                length = read_half_flip(buffer, offset);
                *offset += length;
            }

        } else {  /*  values not in incorrect position so we swap everything */

            for(i=0; i<num_row; i++) {
                length = read_half_flip(buffer, offset);
                MISC_short_swap(buffer+(*offset),length/2);
                *offset += length;
            }        
        
        }
    
    } else { /*  on a Solaris platform (no switching occurs) */
       
        for(i=0; i<num_row; i++) {
            length = read_half_flip(buffer, offset);
            *offset += length;
        }
        
    } /*  end if */
    
}
 

void display_packet_17(int packet, int offset, int replay)
{

/* only decode when not replaying from history */
if(replay == FALSE)
    decode_packet_17((short *)(sd->icd_product), offset);
    
    
    output_packet_17();
    
}



void decode_packet_17(short *product_data, int offset)
{
    int num_halfwords_row, start_index, cell, i, j, k;
    unsigned short halfword;
    unsigned char run, color;
    
    offset /= 2;

/*  DEBUG */
/* fprintf(stderr,"DEBUG DECODE PACKET 17 - BEGINNING OFFSET IS %d\n", offset); */
 
    /* Allocate memory for the raster_rle struture */
    sd->digital_precip = malloc(sizeof(digital_precip_data));
    assert(sd->digital_precip != NULL);

    /* advance the pointer to the beginning of the packet */
    product_data += offset;

    /* get the size of the table */
    sd->digital_precip->number_of_columns = product_data[number_of_columns_offset];
    sd->digital_precip->number_of_rows = product_data[number_of_rows_offset];

    if(verbose_flag) {
        printf("packet id=%d\n", product_data[0]);
        printf("number of rows=%d  number of columns=%d\n", 
                sd->digital_precip->number_of_rows,
                sd->digital_precip->number_of_columns);
    }

    /* Allocate memory for the raster data */
    sd->digital_precip->raster_data = 
                malloc(sd->digital_precip->number_of_rows*sizeof(short *));
    assert(sd->digital_precip->raster_data != NULL);
    for(i=0; i<sd->digital_precip->number_of_rows; i++) {
        sd->digital_precip->raster_data[i] = 
                    malloc(sd->digital_precip->number_of_columns*sizeof(short));
        assert( sd->digital_precip->raster_data[i] != NULL );
    }

/*  DEBUG */
/* fprintf(stderr,"DEBUG DECODE PACKET 17 - FINISHED MALLOC\n"); */


    start_index = p17_header_offset;
    for(i=0; i<sd->digital_precip->number_of_rows; i++) {
        /* Get the number of halfwords this row */
        num_halfwords_row = product_data[ start_index ]/2;
        
        if(verbose_flag)
          printf("row number=%d  number of halfwords=%d\n", i, num_halfwords_row);
        
        /* initialize start_index and bin */
        start_index += p17_row_header_offset;
        cell = 0;
        for(j=0; j<num_halfwords_row; j++, start_index++) {
            halfword = (unsigned short)product_data[ start_index ];
        
            /* Extract RLE data */
            run = halfword >> 8;
            color = halfword & 0xFF;
/*  DEBUG */
/* if(i==0 || i==10) */
/*     fprintf(stderr,"DEBUG DECODE PACKET 17 - run is %d, level is %d\n", run, color); */
            
            for( k = 0; k < run; k++, cell++ )
                    sd->digital_precip->raster_data[i][cell] = color;
        
        } /* end for j < number of halfwords */
        
    } /* end for i < number of rows */
    
} /* end decode_packet_17 */




void delete_packet_17()
{
   int i;

    if(verbose_flag)            
        fprintf(stderr, "*****ENTER delete_packet_17*****\n");  

    if(sd->digital_precip != NULL) {

       for(i=0; i<sd->digital_precip->number_of_rows; i++)
           free(sd->digital_precip->raster_data[i]);
    
       free(sd->digital_precip);
       sd->digital_precip=NULL;
       sd->last_image=NO_IMAGE;
   
    } else 
      fprintf(stderr, "ERROR: attempted to delete packet 17 with NULL structure!\n");   
   
}




void output_packet_17()
{


    int i;
    int j;
    int Color, data_level;

    
    int pixel;
    int scanl;
    int k;

    int n_cols, n_rows;

    /* use screen data for the following */
    /* int tot_num_lvls;                                           */
    /* int num_lead_flags, num_trail_flags;                        */
    /* int leg_min_val, leg_inc, leg_min_val_scale, leg_inc_scale; */
    unsigned long p17_tot_n_lvls;
    unsigned int  p17_n_l_flags; 
    unsigned int  p17_n_t_flags; 

    int num_data_lvls;       /* number of numerical data levels (not flags) */

    int num_flags, first_ind, last_ind, last_num_color;
    double  color_scale; 

    char *legend_filename_ptr, legend_filename[200];
    int digital_flag, *flag_ptr;
    FILE *legend_file;
    
    Prod_header *hdr;
    Graphic_product *gp;

    Widget d;
    XmString xmstr;

    /* CVG 8.7 */
    int legend_error = GOOD_LEGEND;
    

/*  DEBUG */
/* fprintf(stderr,"DEBUG OUTPUT PACKET 17 - \n"); */


    hdr = (Prod_header *)(sd->icd_product);
    gp  = (Graphic_product *)(sd->icd_product + 96);
    
    n_cols = sd->digital_precip->number_of_columns;
    n_rows = sd->digital_precip->number_of_rows;

    sd->last_image = PRECIP_ARRAY_IMAGE;

    p17_g_size = 5;

    p17_orig_x = ( pwidth - (n_cols*p17_g_size) ) / 2;
    p17_orig_y = ( pheight - (n_rows*p17_g_size) ) / 2;
    /*  with g_size == 5 and pwidth == 768, orig is 52 pixels */


    /* if the palette size is, for whatever reason, 0,
     * we don't want to be printing out the product */
    if (palette_size <= 0 ||  palette_size > 256) {
      fprintf(stderr, "* ERROR ********************************************\n");
      fprintf(stderr, "*        Number of colors out of range (1-256)     *\n");
      fprintf(stderr, "*        Aborting product display                  *\n");
      fprintf(stderr, "****************************************************\n");
      return;
    }


    /* see if the product's legend info is encoded like the digital products' */
    flag_ptr = assoc_access_i(digital_legend_flag, hdr->g.prod_id);
    
    if(flag_ptr == NULL)
      digital_flag = 0;
    else
      digital_flag = *flag_ptr;


    
    /* make sure the digital product info is set up correctly */
    if( (digital_flag != 1)  &&  (digital_flag != 2) ) {
            fprintf(stderr,"\nCONFIGURATION ERROR\n");
            fprintf(stderr,"Digital product incorrectly configured\n");
            fprintf(stderr,"as a non-digital product.\n");
            
            d = XmCreateErrorDialog(shell, "Configuration Error", NULL, 0);
            xmstr = XmStringCreateLtoR(
                "Precip Array product incorrectly configured, must use Method 1 or 2.\n"
                "Edit preferences for this product using the CVG Product Specific Info Menu.",
                XmFONTLIST_DEFAULT_TAG);
            XtVaSetValues(XtParent(d), XmNtitle, "Configuration Error", NULL);
            XtVaSetValues(d, XmNmessageString, xmstr, NULL);
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
            XtManageChild(d);
            
        return;
    }
    

    /* read digital data level info in from file */

/*  DEBUG */
/* fprintf(stderr,"DEBUG OUTPUT PACKET 17 - opening legend file\n"); */
 
      legend_filename_ptr = assoc_access_s(digital_legend_file, hdr->g.prod_id);
/*  THIS ERROR CAUGHT DURING PRODUCT POST LOAD  */
/*      if((legend_filename_ptr == NULL) || (strcmp(legend_filename_ptr, ".")==0)) */
/*         return; */

      sprintf(legend_filename, "%s/legends/%s", config_dir, legend_filename_ptr);
      if((legend_file = fopen(legend_filename, "r")) == NULL) {
        fprintf(stderr, "Could not open legend file: %s", legend_filename);
        return;
      }
      

/*  DEBUG */
/* fprintf(stderr,"DEBUG OUTPUT PACKET 17 - reading legend file\n"); */
 
 
      legend_error = read_digital_legend(legend_file, legend_filename, 
                                               digital_flag, PRODUCT_FRAME);
 
      fclose(legend_file);
      
      if(legend_error==LEGEND_ERROR) {
          fprintf(stderr,"ERROR - Display of Digital Precip Product Aborted Due to "
                          " errors in the digital legend file.\n");
          return;
      }

/*  DEBUG */
/* fprintf(stderr,"DEBUG OUTPUT PACKET 17 - reading screen data\n"); */
 
     p17_tot_n_lvls = sd->digital_precip->decode.tot_n_levels;
     p17_n_l_flags  = sd->digital_precip->decode.n_l_flags;
     p17_n_t_flags  = sd->digital_precip->decode.n_t_flags;

    /* If the palette size does not equal the total number of levels, 
     * we need the following inputs to determine which color to use:
     * p17_tot_n_lvls     the total number of data levels including flags
     * palette_size     the number of colors in the palette
     * p17_n_l_flags   the number initial values that are flags (if any)
     * p17_n_t_flags  the number of trailing values that are flags (if any)
     */
    
    /* if out of range, abort product display */
    if ( p17_tot_n_lvls <= 0 ||  p17_tot_n_lvls > 256) {
        p17_tot_n_lvls = 256;
        fprintf(stderr, 
                "* ERROR ********************************************\n");
        fprintf(stderr, 
                "*        Number of data levels out of range (1-256)*\n");
        fprintf(stderr, 
                "*        Aborting product display                  *\n");
        fprintf(stderr, 
                "****************************************************\n");
        return;
    }
    
    /* if the palette size is, for whatever reason, 0,
     * we don't want to be printing the product */
    if ( palette_size <= 0 ||  palette_size > 256) {
       fprintf(stderr, "* ERROR ********************************************\n");
       fprintf(stderr, "*        Number of colors out of range (1-256)     *\n");
       fprintf(stderr, "*        Aborting product display                  *\n");
       fprintf(stderr, "****************************************************\n");
       return;
    }

    num_flags = p17_n_l_flags + p17_n_t_flags;
    num_data_lvls = p17_tot_n_lvls - num_flags;
    /* index of first numerical (non flag) value */
    first_ind = 0 + p17_n_l_flags; 
    /* index of last numerical (non flag) value */
    last_ind = first_ind + (num_data_lvls - 1);
    /* index of last color used for a numerical value */
    last_num_color = palette_size - num_flags;


    /* PROTECTION FOR PALETTE TOO SMALL */
    if(palette_size <= num_flags) {
        color_scale = p17_tot_n_lvls;
        fprintf(stderr, 
            "* ERROR ********************************************************\n");
        fprintf(stderr, 
            "* The color palette is too small for the number of flag values *\n");
        fprintf(stderr, 
            "* The palette must have more colors than number of flags       *\n");
        fprintf(stderr, 
            "* Number of colors is %d, number of flag values is %d          *\n",
                                                        palette_size, num_flags );
        fprintf(stderr, 
            "****************************************************************\n");
         
     } else {    
        color_scale = ceil( (double)(p17_tot_n_lvls - num_flags) / 
                (double)(palette_size - num_flags) );

   }

     
     /* should not have more colors than data levels */
     /* if so, colors are truncated */
     if (color_scale < 1) {
        color_scale = 1;
        fprintf(stderr, 
                "* WARNING ****************************************************\n");
        fprintf(stderr, 
                "* The color palette is larger than the number of data levels *\n");
        fprintf(stderr, 
                "* The excess colors (at top end) are not used                *\n");
        fprintf(stderr, 
                "**************************************************************\n");
     }
    

/*  DEBUG */
/* fprintf(stderr,"DEBUG OUTPUT PACKET 17 - printing data to screen\n"); */


    for(j=0; j<sd->digital_precip->number_of_rows; j++) {

        if((j==64) && (verbose_flag)) {
            printf("row number %d:\n", j+1);
            for(k=0; k<sd->digital_precip->number_of_columns; k++)
                printf("%d ", sd->digital_precip->raster_data[j][k]);
            printf("\n");   
        }

    /* the DPA product is treated as a non-geographic product because 
     * the image is not actually projected on a 1/40 LFM grid . 
     */


        for(i=0; i<sd->digital_precip->number_of_columns; i++) {
            /* Beginning with CVG 6.5, the colors are corectly determined by using
             * data from the legend files
             */
            data_level = sd->digital_precip->raster_data[j][i];
            if(data_level < first_ind)  /* leading flag */
                Color = data_level;
            else if (data_level > last_ind) /* trailing flag */
                Color = last_num_color + (data_level - last_ind);
            else  {
                Color = ( (data_level - p17_n_l_flags) / (int)color_scale ) + 
                           p17_n_l_flags;
                if (Color > last_num_color)
                    Color = last_num_color;
            }
            
                pixel = p17_orig_x + (p17_g_size * i);
                scanl = p17_orig_y + (p17_g_size * j);
                
                XSetForeground(display, gc, display_colors[Color].pixel);
                XFillRectangle(display, sd->pixmap, gc, pixel, scanl, p17_g_size, p17_g_size);
            
        } /*  end for number_of_columns */
        
    } /*  end for number_of_rows */

/*  DEBUG */
/* fprintf(stderr,"DEBUG OUTPUT PACKET 17 - printing overlay grid to screen\n"); */


    XSetForeground(display, gc, white_color);

    for(k = 0; k <= n_rows; k++) /*  horizontal lines */
        XDrawLine(display, sd->pixmap, gc, p17_orig_x, p17_orig_y + (k * p17_g_size), 
                  p17_orig_x + (n_cols * p17_g_size), p17_orig_y + (k * p17_g_size));

    for(k = 0; k <= n_cols; k++) /*  vertical lines */
        XDrawLine(display, sd->pixmap, gc, p17_orig_x + (k * p17_g_size), p17_orig_y, 
                  p17_orig_x + (k * p17_g_size), p17_orig_y + (n_rows * p17_g_size));

    /* for reference, the center bin (approximately the radar origin) is outlined in black */
    
    XSetForeground(display, gc, black_color);
    
    XDrawRectangle(display, sd->pixmap, gc, p17_orig_x + ((n_cols/2-1) * p17_g_size), 
                          p17_orig_y + ((n_rows/2-1) * p17_g_size), 
                          p17_g_size, p17_g_size);                       



    XSetForeground(display, gc, white_color);
    
/*  DEBUG */
/* fprintf(stderr,"DEBUG OUTPUT PACKET 17 - FINISHED\n"); */

    
}

    
