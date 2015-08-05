/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/13 19:47:46 $
 * $Id: radial_rle.c,v 1.14 2012/09/13 19:47:46 steves Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */
/* radial_rle.c */

#include <math.h>
#include <Xm/Xm.h>
#include "radial_rle.h"
#include <unistd.h>



void packet_AF1F_skip(char *buffer,int *offset)
{
    short length, num_rad, num_bin, i;
    /*LINUX changes*/
    MISC_short_swap(buffer+*offset,6);

    if(verbose_flag)
        fprintf(stderr, "entering packet_AF1F_skip(), val=%x offset=%d\n", 
        buffer[*offset], *offset);

    *offset += 2*number_of_bins_offset -2;
    num_bin = read_half(buffer, offset);    
    if(verbose_flag)
        fprintf(stderr, "number of range bins=%d, offset=%d\n", num_bin, *offset);
    
    *offset += 2*number_of_radials_offset - 2 - 2*number_of_bins_offset;
    num_rad = read_half(buffer, offset);

    if(verbose_flag)
        fprintf(stderr, "number of radials=%d, offset=%d\n", num_rad, *offset);

    for(i=0;i<num_rad;i++) {
        /*length = read_half(buffer, offset);*/
        length = read_half_flip(buffer, offset);
        MISC_short_swap(buffer+*offset,2);/* flips the first two shorts after length */
        *offset += 2*length + 4;/* the length doesn't include the first two shorts */
    }
  
}




void display_radial_rle(int packet, int offset, int replay)
{

    /* only decode when not replaying from history */
    if(replay == FALSE)
        decode_radial_rle((short *)(sd->icd_product), offset);
 
    output_radial_rle();
    
}




void decode_radial_rle(short *product_data, int offset)
{
    int num_halfwords_radial, start_index, i, j, k, error, some_errors;

    unsigned char run, color;
    int n_bins=0; /*  number of bins malloc'd */
    int bin; /*  running counter for bins unpacked */
    int last_bin=0; /*  record last good bin unpacked // CVG 8.2 change */
    unsigned char *bytesize_pointer=(unsigned char *)(product_data);
    unsigned char byte;
 
 
 int is_padded = FALSE;
 
   
    offset /= 2;
 
    /* Allocate memory for the radial_rle struture */
    sd->radial = malloc( sizeof(radial_rle) );
    assert(sd->radial != NULL);

    /* advance the pointer to the beginning of the packet */
    product_data += offset;

    /* Extract the number of data elements, range interval, and number 
     * of radials 
     */
    /*  for the support of products that have an index to first range bin (59 and 69) */
    sd->radial->index_to_first_range_bin=product_data[1];

    sd->radial->data_elements = product_data[ number_of_bins_offset ];
    sd->radial->range_interval = product_data[ range_interval_offset ]/1000.0;
    sd->radial->number_of_radials = product_data[ number_of_radials_offset ];

    if(verbose_flag) {
        fprintf(stderr,"packet code=%hx\n",product_data[0]);
        fprintf(stderr,"index of 1st range bin=%hd\n",product_data[1]);
        fprintf(stderr,"data elements=%d\n",sd->radial->data_elements);
        fprintf(stderr,"i center of sweep=%hd\n",product_data[3]);
        fprintf(stderr,"j center of sweep=%hd\n",product_data[4]);
        fprintf(stderr,"range interval=%6.3f\n",sd->radial->range_interval);
        fprintf(stderr,"num of radials=%d\n",sd->radial->number_of_radials);
    }

   /* we do not check for errors in the packet header relating to number of radials 
   and number of data bins.  Logic could be added to ensure we do not read beyond the
   end of the data packet */

   /* however, because of an error in the Legacy base relfectivity product,
   when unpacking the RLE data, we terminate filling the internal data structure
   when after filling the last bin */

    /* Allocate memory for the radial data */
    sd->radial->radial_data = malloc(sd->radial->number_of_radials*sizeof(short *));
    assert( sd->radial->radial_data != NULL );

    for( i = 0; i < sd->radial->number_of_radials; i++ ){
        /**Allocate one extra space to handle the case of an odd number of data bins
           being padded.  NOT SURE IF THIS IS NEEDED**/
        if((sd->radial->data_elements % 2) == 0)
        {
            sd->radial->radial_data[i] = 
                                  malloc((sd->radial->data_elements)*sizeof(short));
            n_bins = sd->radial->data_elements;

        } else  {
        
        is_padded = TRUE; 
            sd->radial->radial_data[i] = 
                             malloc((sd->radial->data_elements + 1)*sizeof(short));
            n_bins = sd->radial->data_elements + 1;
        }
        assert( sd->radial->radial_data[i] != NULL );
    } /*  end for numbre_of_radials */

    /* Allocate memory for the azimuth data */
    sd->radial->azimuth_angle = 
                             malloc( sd->radial->number_of_radials*sizeof(float) );
    assert( sd->radial->azimuth_angle != NULL );
    /* Allocate memory for the azimuth width data */
    sd->radial->azimuth_width = 
                             malloc( sd->radial->number_of_radials*sizeof(float) );
    assert( sd->radial->azimuth_width != NULL );

    start_index = radial_rle_offset;
    
    some_errors = FALSE;

    /*   FUTURE ENHANCEMENT: Include looking for beginning of next radial and/or */
    /*                       end of data packet for consistency check */
    
    for( i = 0; i < sd->radial->number_of_radials; i++ ) {
        /* Get the number of halfwords this radial */
        num_halfwords_radial = product_data[ start_index ];
      
        /* Get the radial azimuth and azimuth delta */
        sd->radial->azimuth_angle[i] = 
          (float)product_data[start_index+azimuth_angle_offset]/10.0;
        sd->radial->azimuth_width[i] = 
          (float)product_data[start_index+azimuth_delta_offset]/10.0;
    
        /* initialize start_index and bin */
        start_index += radial_data_array_offset;
        bin = 0;

        bytesize_pointer=(unsigned char *)(product_data);

        error=FALSE;

        for( j = 0; j <  num_halfwords_radial; j++, start_index++ ) {

            /* reading byte by byte, sort of */ 
                
            /* Extract first nibble of RLE data */
            byte=bytesize_pointer[start_index*2];
            
            run=byte/16;
            color=byte & 0x0f;

            if(error == FALSE) { 

                for( k = 0; k < run; k++, bin++ ) {
                    
                    if(bin < n_bins) {
                        sd->radial->radial_data[i][bin] = color; 
                    
                    } else {
                        if(verbose_flag) fprintf(stderr, 
                                "DATA ERROR (L1) Packet AF1F, Radial %d - number of "
                                "unpacked bins exceeds number of data elements %d\n"
                                "           HW per radial is %d, reading RLE HW %d\n",
                                i+1, n_bins, num_halfwords_radial, j+1);
                        error=TRUE;
                        some_errors = TRUE; 
                        last_bin = bin -1; 
                        break; 
                    }
    
                } /*  end for run */

            } /*  end if error FALSE */
    
            /* Extract second nibble of RLE data */
            byte=bytesize_pointer[start_index*2+1];
            
            run=byte/16;
            color=byte & 0x0f;

            if(error == FALSE) { 
    
                for( k = 0; k < run; k++, bin++ ) {
                    
                    if(bin < n_bins) {
                        sd->radial->radial_data[i][bin] = color;
                    
                    } else {
                        if(verbose_flag) fprintf(stderr, 
                                "DATA ERROR (L2) Packet AF1F, Radial %d - number of "
                                "unpacked bins exceeds number of data elements %d\n"
                                "           HW per radial is %d, reading RLE HW %d\n",
                                i+1, n_bins, num_halfwords_radial, j+1);
                        error=TRUE;
                        some_errors = TRUE; 
                        last_bin = bin -1; 
                        break; 
                    } /*  end else exceeded number of bins */
    
                } /*  end for run */

            } /*  end if error FALSE */
                
        } /*  end for each halfword */
        
        
        if(error == FALSE)
            last_bin = bin -1;
            
        /*  this is our correction for product 59 to get the actual number of bins */
        /*  as opposed to the number of bins there would be if the index to first bin */
        /*  were 0.. this lets the display work properly */
        sd->radial->data_elements=last_bin;

    } /*  end for each radial */

/*  DEBUG */
/* if(is_padded==TRUE) */
/*      fprintf(stderr, "DEBUG decode_radial_rle - padded bins to %d\n", n_bins); */
 
    if(some_errors == TRUE) 
        fprintf(stderr,"\nWARNING - PRODUCT ERROR - number of unpacked "
                                                  "bins exceeds number of bins in\n"
                       "          at least one radial. Select Verbose "
                                                  "Output for more information.\n");
    
} /* end decode_radial_rle() */




void delete_radial_rle()
{
    int i;

    if(verbose_flag)            
        fprintf(stderr, "*****ENTER delete_radial_rle*****\n");  

    if(sd->radial != NULL) {

        for(i=0; i < sd->radial->number_of_radials; i++)
                free(sd->radial->radial_data[i]);
                
        if(verbose_flag)            
            fprintf(stderr, "*****freed all radials*****\n");  
           
        free(sd->radial->radial_data);
        free(sd->radial->azimuth_angle);
        free(sd->radial->azimuth_width);
        
        free(sd->radial);
        
        sd->radial = NULL;
        sd->last_image=NO_IMAGE;

    } else 
    
      fprintf(stderr, "ERROR - attempted to delete radial rle with NULL structure!\n");     
    
}




void output_radial_rle()
{
    int beam;
    int i;
    float   azimuth1, azimuth2;
    int old_Color;
    int Color;
    XPoint  X [12];
    float   sin1, sin2;
    float   cos1, cos2;

    float   scale_x;
    float   scale_y;

    int      center_pixel;
    int      center_scanl;
    int digital_flag, *flag_ptr;
    
    Widget d;
    XmString xmstr;
    
    Prod_header *hdr = (Prod_header *)(sd->icd_product);
    
    sd->last_image = RLE_IMAGE;

    scale_x =  sd->scale_factor;
    scale_y = -sd->scale_factor;

    center_pixel = pwidth/2;  /* initialized to center of canvas */
    center_scanl = pheight/2; /* initialized to center of canvas */

    /* correct center pixel for the off_center value (if any) */
    center_pixel = center_pixel - (int) (sd->x_center_offset * sd->scale_factor);  
    center_scanl = center_scanl - (int) (sd->y_center_offset * sd->scale_factor);    



    /* see if the product's legend info is encoded like the rle products' */
    flag_ptr = assoc_access_i(digital_legend_flag, hdr->g.prod_id);
    digital_flag = *flag_ptr;

    /* make sure the rle product info is set up correctly */
    if(digital_flag != 0) {
            fprintf(stderr,"\nCONFIGURATION ERROR\n");
            fprintf(stderr,"RLE product incorrectly configured\n");
            fprintf(stderr,"as a digital product.\n");
            
            d = XmCreateErrorDialog(shell, "Configuration Error", NULL, 0);
            xmstr = XmStringCreateLtoR("Non-digital product incorrectly configured "
                                      "as a digital product.\nEdit preferences for "
                            "this product using the CVG Product Specific Info Menu.",
                XmFONTLIST_DEFAULT_TAG);
            XtVaSetValues(XtParent(d), XmNtitle, "Configuration Error", NULL);
            XtVaSetValues(d, XmNmessageString, xmstr, NULL);
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
            XtManageChild(d);
            
        return;
    }


    /*  For each beam in the product, display it using the product  *
     *  value as a unique color in the display.             */

    for(beam=0;beam<sd->radial->number_of_radials;beam++) {
        float extra_width = 1.5; /* Added to eliminate black speckels between radials */
  
        /*  reduced padding to accomdate registered radials */
        /*  from ORDA and to handle future 0.5 deg radials.  Artifacts avoided */
        /*  by drawing BLACK, the background color, for all segments. */
        /*  Don't draw such a wide beam on the last radial.  */
        if (beam == sd->radial->number_of_radials-1) extra_width = 0.5;
        azimuth1 = sd->radial->azimuth_angle [beam];
        azimuth2 = azimuth1 + sd->radial->azimuth_width [beam] + extra_width;

        sin1 = sin ((double) (azimuth1+90)*DEGRAD);
        sin2 = sin ((double) (azimuth2+90)*DEGRAD);
        cos1 = cos ((double) (azimuth1-90)*DEGRAD);
        cos2 = cos ((double) (azimuth2-90)*DEGRAD);
        
        if((beam==0) && (verbose_flag==TRUE)) { 
            int n; 
        
            fprintf(stderr,"angle %6.2f delta %6.2f\n", 
                 sd->radial->azimuth_angle[beam], sd->radial->azimuth_width[beam]);
               
            for(n=0;n<sd->radial->data_elements;n++)
                fprintf(stderr,"%03i ",sd->radial->radial_data[beam][n]); 
            fprintf(stderr,"\n"); 
        }  
        
        old_Color = -1;
        
        Color = -1;

        /*  THE FOLLOWING ASSUMES THAT COLOR 0 IS BACKGROUND (I.E. BLACK ) */
        /*  FUTURE CHANGE - If Color == 0, set color = Assigned Background */

        /*  Unpack the data at each gate along the beam.  
         *  To reduce the number of XPolygonFill operations, 
         *  only paint gate(s) when a color change occurs.
         */

        /* modified to draw background color as well  */
                                      
        /*                 X[0],X[4]      X[3] */
        /*                 - - * - - - - - * - - */
        /*     PREVIOUS BIN <= |           | => NEXT BIN */
        /*               - - - * - - - - - * - - */
        /*                    X[1]        X[2] */

        /*  Initialize near points of sector to be drawn        */
        X[0].x = X[1].x = X[4].x = center_pixel;
        X[0].y = X[1].y = X[4].y = center_scanl;

        for (i=0;i<sd->radial->data_elements;i++) { 
            
            Color = sd->radial->radial_data[beam][i];

            /*  If successive colors are the same we draw as one large segement */
            if (Color != old_Color) {

                X[2].x = (i+sd->radial->index_to_first_range_bin) 
                                              * cos2 * scale_x + center_pixel;
                X[2].y = (i+sd->radial->index_to_first_range_bin) 
                                              * sin2 * scale_y + center_scanl;
                X[3].x = (i+sd->radial->index_to_first_range_bin) 
                                              * cos1 * scale_x + center_pixel;
                X[3].y = (i+sd->radial->index_to_first_range_bin) 
                                              * sin1 * scale_y + center_scanl;

                XSetForeground(display, gc, display_colors[old_Color].pixel);
                XFillPolygon(display, sd->pixmap, gc, X, 4, Convex, CoordModeOrigin);

                X[0].x = X[3].x;
                X[0].y = X[3].y;
                X[1].x = X[2].x;
                X[1].y = X[2].y;
                X[4].x = X[0].x;
                X[4].y = X[0].y;
                    
            } /*  end Color != old_Color */
            
            old_Color = Color;
        
        } /* end unpack for each gate */

        /* draw last segmemt in case there was no change in color */
        i = sd->radial->data_elements; 
        X[2].x = (i-1+sd->radial->index_to_first_range_bin) 
                                        * cos2 * scale_x + center_pixel;
        X[2].y = (i-1+sd->radial->index_to_first_range_bin) 
                                        * sin2 * scale_y + center_scanl;
        X[3].x = (i-1+sd->radial->index_to_first_range_bin) 
                                        * cos1 * scale_x + center_pixel;
        X[3].y = (i-1+sd->radial->index_to_first_range_bin) 
                                        * sin1 * scale_y + center_scanl;

        XSetForeground(display, gc, display_colors[Color].pixel);
        XFillPolygon(display, sd->pixmap, gc, X, 4, Convex, CoordModeOrigin);

    } /* end for each beam do */
    

    
} /* end output_radial_rle() */


