/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/13 19:47:45 $
 * $Id: packet_16.c,v 1.15 2012/09/13 19:47:45 steves Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */
/* packet_16.c */

#include <math.h>
#include <Xm/Xm.h>
#include "packet_16_cvg.h"

#include "byteswap.h"


double  color_scale; 
/*  output function more general, could handle unsigned short, etc. */
/*  which is the reason for unsigned int and unsigned long          */
unsigned int num_flags, first_ind, last_ind, last_num_color; 

/* use screen data for the following */
/* unsigned long tot_num_lvls; */  
/* unsigned int num_lead_flags, num_trail_flags; */
unsigned long p16_tot_n_lvls; /* number of data levels including flags */
unsigned int  p16_n_l_flags;  
unsigned int  p16_n_t_flags; 

unsigned long num_data_lvls; /* number of numerical data levels (not flags) */


/* TEST PROTOTYPE TEST FUNCTION */
int test_lookup_gen_color(unsigned int d_level);


void packet_16_skip(char *buffer,int *offset) 
{
    short length, num_rad, num_bin, i;

    if(verbose_flag)
        fprintf(stderr, "entering packet_16_skip(), val=%x offset=%d\n", 
                                                    buffer[*offset], *offset);
    /*LINUX changes*/
    MISC_short_swap(buffer+*offset,6); /* I think that's right.. */
    *offset += 2*number_of_bins_offset -2;
    num_bin = read_half(buffer, offset);
    if(verbose_flag)
        fprintf(stderr, "number of range bins=%d, offset=%d\n", num_bin, *offset);
    
    *offset += 2*number_of_radials_offset - 2 - 2*number_of_bins_offset;
    num_rad = read_half(buffer, offset);

    if(verbose_flag)
        fprintf(stderr, "number of radials=%d, offset=%d\n", num_rad, *offset);

    for(i=0; i<num_rad; i++) {
        /*length = read_half_flip(buffer, offset);*/
        length = read_half_flip(buffer, offset);
        MISC_short_swap(buffer+*offset,2);/* I think this is right also.. */
        if(i==1)
            if(verbose_flag)
                fprintf(stderr, "number of bytes in first radial=%d\n", length);
        *offset += length + 4;
    }

}





void display_packet_16(int packet, int offset, int replay)
{

   if(verbose_flag)
      fprintf(stderr, "*****ENTER display_packet_16*****\n"); 

   /* only decode when not replaying from history */
   if(replay == FALSE) {
      decode_packet_16((short *)(sd->icd_product), offset);
   }
   
   
   output_packet_16();
}





void decode_packet_16(short *product_data, int offset)
{ 
   int num_halfwords_radial, start_index, bin, i, j;
   int nbins_padded=0;
   int padded=FALSE;

   unsigned char *bytesize_pointer;
   unsigned char byte;
   
   unsigned short temp_max_level, temp_n_l_flags, temp_n_t_flags;
   int retval;
   
   if(verbose_flag)
        fprintf(stderr, "*****ENTER decode_packet_16*****\n");
   
   offset /= 2;

   /* Allocate memory for the radial structure */
   sd->dhr = malloc(sizeof(radial_dig));
   assert( sd->dhr != NULL );


   /* read PDB parameters for decoding using the scale offset         */
   /* these values will only be correct for those products using them */
   sd->dhr->decode.decode_flag = NO_DECODE;

   /* the following must be used to read either float or 4-byte integers */
   /* because the first HW always contains the most significant short in */
   /* a WSR-88D product.                                                 */
   retval = read_orpg_product_float( (void*) &product_data[THR_01_OFFSET/2],
                                     (float*) &sd->dhr->decode.prod_Scale);
   retval = read_orpg_product_float( (void*) &product_data[THR_03_OFFSET/2],
                                     (float*) &sd->dhr->decode.prod_Offset);
   
   /* note: the data structure can handle four bytes for the following but  */
   /*       the design of the product limits it to two bytes in the product */
   temp_max_level        = (unsigned short) product_data[THR_06_OFFSET/2];
   
   temp_n_l_flags        = (unsigned short) product_data[THR_07_OFFSET/2];
   temp_n_t_flags        = (unsigned short) product_data[THR_08_OFFSET/2];
   sd->dhr->decode.prod_max_level = (unsigned int) temp_max_level;
   sd->dhr->decode.prod_n_l_flags = (int)          temp_n_l_flags;
   sd->dhr->decode.prod_n_t_flags = (int)          temp_n_t_flags;


   /* CVG 9.1 */
   if(verbose_flag) {
       fprintf(stderr,"NOTE: THE FOLLOWING OUTPUT IS MEANINGLESS UNLESS THE\n");
       fprintf(stderr,"         PRODUCT CONTAINS THE OFFSET SCALE PARAMETERS \n");
       fprintf(stderr,"Packet 16 - Scale: %f   Offset: %f \n"                     
                      "    max level: %u    lead flags: %d    trail flags: %d\n", 
                   sd->dhr->decode.prod_Scale, sd->dhr->decode.prod_Offset,       
                   sd->dhr->decode.prod_max_level,                                
                   sd->dhr->decode.prod_n_l_flags,                                
                   sd->dhr->decode.prod_n_t_flags);                               
   }
   

   /* advance pointer to beginning of packet block */
   product_data = product_data + offset;

   /* Extract number of data elements, range interval, and number of 
      radials */
   sd->dhr->data_elements    = (int)   product_data[number_of_bins_offset];
   sd->dhr->range_interval   = (float) product_data[range_interval_offset];
   sd->dhr->number_of_radials= (int)   product_data[number_of_radials_offset];

   if(verbose_flag) {
       printf("packet code=%hd\n",product_data[0]);
       printf("index of 1st range bin=%hd\n",product_data[1]);
       printf("data elements=%d\n",sd->dhr->data_elements);
       printf("i center of sweep=%hd\n",product_data[3]);
       printf("j center of sweep=%hd\n",product_data[4]);
       printf("range interval=%6.3f\n",sd->dhr->range_interval);
       printf("num of radials=%d\n",sd->dhr->number_of_radials);
   }

   /* we do not check for errors in the packet header relating to number of 
      radials and number of data bins.  Logic could be added to ensure we do not 
      read beyond the end of the data packet */


   /* Allocate memory for the radial data */
   sd->dhr->radial_data = malloc(sd->dhr->number_of_radials * sizeof(short *));
   assert(sd->dhr->radial_data != NULL);
   for( i = 0; i < sd->dhr->number_of_radials; i++ ) {
       /**Allocate one extra space to handle the case of an odd number of data 
          bins being padded**/
       if((sd->dhr->data_elements % 2) == 0) {
           sd->dhr->radial_data[i] = malloc((sd->dhr->data_elements)*sizeof(short));
           nbins_padded = sd->dhr->data_elements;
       } else {
           sd->dhr->radial_data[i] = 
                                 malloc((sd->dhr->data_elements + 1)*sizeof(short));
           nbins_padded = sd->dhr->data_elements+1;
           if(padded==FALSE) {
               fprintf(stderr,
                       "Packet Code 16 - number of data values in radial is odd\n");
               fprintf(stderr,"                 padded number of bins to %d\n",
                         sd->dhr->data_elements+1);
           }
           padded=TRUE;
       }
       
       assert( sd->dhr->radial_data[i] != NULL );
       
   } /*  end for number_of_radials */



   /* Allocate memory for the azimuth data */
   sd->dhr->azimuth_angle = malloc( sd->dhr->number_of_radials*sizeof(float) );
   assert( sd->dhr->azimuth_angle != NULL );

   /* Allocate memory for the azimuth width data */
   sd->dhr->azimuth_width = malloc( sd->dhr->number_of_radials*sizeof(float) );
   assert( sd->dhr->azimuth_width != NULL );

   start_index = rad_num_bytes_offset;
   
/*   FUTURE ENHANCEMENT: Include looking for beginning of next radial and/or */
/*                       end of data packet for consistency check */
   
   for(i=0; i<sd->dhr->number_of_radials; i++ ) {
    
      /* Get the number of halfwords this radial */
      /* consistency check */
      if(product_data[ start_index ] != nbins_padded) {
          fprintf(stderr,
                  "PACKET CODE 16 ERROR - number of bins expected in radial %d\n",
                  nbins_padded);
          fprintf(stderr,
                  "   is not equal to the number of bytes (%d) for radial %d\n",
                  product_data[ start_index ], i+1);
      }
      
      num_halfwords_radial = product_data[ start_index ]/2;
                                            
      /* Get the radial azimuth and azimuth delta */
      sd->dhr->azimuth_angle[i] = (float)       
             product_data[ start_index + azimuth_angle_offset ]/10.0;
      sd->dhr->azimuth_width[i] = (float)       
             product_data[ start_index + azimuth_delta_offset ]/10.0;
                                            
      /* initialize start_index and bin */      
      start_index += radial_data_array_offset; 
      bin = 0; 
      /*LINUX changes*/ 
      bytesize_pointer=(unsigned char *)product_data;  
                           
      for( j = 0; j <  num_halfwords_radial; j++, start_index++ ){ 
         /* we read two bytes at a time, packet 16 has an even number of bytes */
         /* the screen data structure is an array of unsigned short  */ 
         byte=(unsigned char) bytesize_pointer[start_index*2]; 
         sd->dhr->radial_data[i][bin++] = byte; 
         byte=(unsigned char) bytesize_pointer[start_index*2+1]; 
         sd->dhr->radial_data[i][bin++] = byte; 
      }   /*  end for each halfword  */
          
   } /*  end for each radial     */
                                
}  /* end decode_packet_16() */ 





                                
void delete_packet_16()         
{                               
  int i;                        
                                
    if(verbose_flag)            
        fprintf(stderr, "*****ENTER delete_packet_16*****\n");  
                                
    if(sd->dhr != NULL) {       
        
      for(i=0; i < sd->dhr->number_of_radials; ++i)
          free(sd->dhr->radial_data[i]);
      
      free(sd->dhr->radial_data);
      free(sd->dhr->azimuth_angle);
      free(sd->dhr->azimuth_width);
      free(sd->dhr);
      sd->dhr = NULL;
      sd->last_image=NO_IMAGE;
  
    } else 
      fprintf(stderr, 
              "ERROR - attempted to delete packet 16 with NULL structure!\n"); 
  
}







void output_packet_16()
{
    int beam;
    int i;
    float   azimuth1, azimuth2;
/* in case we support unsigned short in future */
unsigned int data_level;    
    int Color;
    int old_Color;

    XPoint  X[12];
    float   sin1, sin2;
    float   cos1, cos2;
    float   scale_x, scale_y;

    int      center_pixel;  
    int      center_scanl; 


    char *legend_filename_ptr=NULL, legend_filename[200];
    int digital_flag, *flag_ptr;
    FILE *legend_file=NULL;
    
    int legend_error = GOOD_LEGEND;


    Prod_header *hdr;
    Graphic_product *gp;

    
    Widget d;
    XmString xmstr;



    /***********************************************************************/
    /*  SECTION 1 PRELIMINARIES                                            */
    /***********************************************************************/
    
    if(verbose_flag)
        fprintf(stderr, "*****ENTER output_packet_16*****\n");
        
    hdr = (Prod_header *)(sd->icd_product);
    gp  = (Graphic_product *)(sd->icd_product + 96);

    sd->last_image = DIGITAL_IMAGE;

    scale_x =  sd->scale_factor;
    scale_y = -sd->scale_factor;

    center_pixel = pwidth/2;  /* initialized to center of canvas */
    center_scanl = pheight/2; /* initialized to center of canvas */

    /* correct center pixel for the off_center value (if any) */
    center_pixel = center_pixel - (int) (sd->x_center_offset * sd->scale_factor);  
    center_scanl = center_scanl - (int) (sd->y_center_offset * sd->scale_factor);    
/* TEST */
/* fprintf(stderr,"TEST - display p16, x center is %d, y center is %d\n", */
/*                               center_pixel, center_scanl); */
/* fprintf(stderr,"TEST -             x offset is %d, y offset is %d, zoom is %f\n", */
/*                      sd->x_center_offset, sd->y_center_offset, sd->scale_factor); */


    
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

/* DEBUG */
/*fprintf(stderr,"DEBUG output_packet_16 - digital flag is %d\n", digital_flag); */

    /* make sure the digital product info is set up correctly */
    if(digital_flag == 0) {
            fprintf(stderr,"\nCONFIGURATION ERROR\n");
            fprintf(stderr,"Digital product incorrectly configured\n");
            fprintf(stderr,"as a non-digital product.\n");
            
            d = XmCreateErrorDialog(shell, "Configuration Error", NULL, 0);
            xmstr = XmStringCreateLtoR("Digital product incorrectly configured "
                                       "as a non-digital product.\nEdit "
                                       "preferences for this product using the "
                                       "CVG Product Specific Info Menu.",
                XmFONTLIST_DEFAULT_TAG);
            XtVaSetValues(XtParent(d), XmNtitle, "Configuration Error", NULL);
            XtVaSetValues(d, XmNmessageString, xmstr, NULL);
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
            XtManageChild(d);
            
        return;
    }

    /***********************************************************************/
    /*  SECTION 2 READ DIGITAL LEGEND FILE                                 */
    /***********************************************************************/


    /* read digital data level info in from file */
    
    /* open correct legend file */
    if(digital_flag !=3) { 
             
      legend_filename_ptr = assoc_access_s(digital_legend_file, hdr->g.prod_id);
/*  THIS ERROR CAUGHT DURING PRODUCT POST LOAD */
/*      if((legend_filename_ptr == NULL) || (strcmp(legend_filename_ptr, ".")==0)) */
/*         return; */

      sprintf(legend_filename, "%s/legends/%s", config_dir, legend_filename_ptr);
      if((legend_file = fopen(legend_filename, "r")) == NULL) {
        fprintf(stderr, "Could not open legend file: %s", legend_filename);
        return;
      }
      

    } else if(digital_flag == 3) { /* digital velocity product */             

      if(gp->level_1 == -635 ) {
        legend_filename_ptr = assoc_access_s(digital_legend_file, hdr->g.prod_id);     
      } else if(gp->level_1 == -1270 ) {
        legend_filename_ptr = assoc_access_s(dig_legend_file_2, hdr->g.prod_id);        
      } else { /*  unexpected value - use first legend file */
        legend_filename_ptr = assoc_access_s(digital_legend_file, hdr->g.prod_id);        
      }
              
      sprintf(legend_filename, "%s/legends/%s", config_dir, legend_filename_ptr);
      if((legend_file = fopen(legend_filename, "r")) == NULL) {
        fprintf(stderr, "Could not open legend file: %s", legend_filename);
        return;
      }


/***********************************************************************/
  }

  /* replaced manual reading with external function which */
  /*           also copies information into screen data decode area */
  /*  This (along with other changes permits not reading when the   */
  /*  legend color bars are displayed.                              */
  /*  NEED TO MAKE THE SAME CHANGES TO packet 17                    */


  if( (digital_flag==1) || (digital_flag==2) || (digital_flag==3) ) {   

      
      legend_error = read_digital_legend(legend_file, legend_filename, 
                                               digital_flag, PRODUCT_FRAME);
 
      fclose(legend_file);
      
      
      if(legend_error==LEGEND_ERROR) {
          fprintf(stderr,"ERROR - Display of Digital Product Aborted Due to "
                          " errors in the digital legend file.\n");
          return;
      }
      


    } else if( (digital_flag==4) || (digital_flag==5) || (digital_flag==6) ) {
      
      
      legend_error = read_generic_legend(legend_file, legend_filename, 
                                              digital_flag, PRODUCT_FRAME);
      
      fclose(legend_file); 
      
      if(legend_error==LEGEND_ERROR) {
          fprintf(stderr,"ERROR - Display of Digital Product Aborted Due to "
                          " errors in the generic legend file.\n");
          return;
      }
    
    } /* end read generic legend */
    
    
    
    
   
    /***********************************************************************/
    /*  SECTION 3 CALCULATE COLOR ASSIGNMENTS (color_scale)                */
    /***********************************************************************/

    /*  The color scale method of assigning colors only supports the digital */
    /*  legend flag values 1, 2, and 3.  Another method is required for the */
    /*  generic legend flag values 4, 5, and 6. */

    /* if there are fewer colors than data levels:                        */
        /* We assign a color uniquely to each non-numerical (flag) value.
         * The remaining colors are spread out evenly amoung the numerical
         * (non-flag) data values.  This is typically the case with legend
         * flag 1 (calculated legend values). This would also be the case
         * with the future legend flags 4, 5, & 6 (generic data legend 
         * values).
         */
    /* if the number of colors is >= the number of levels:                */
        /* The color index is the same as the data index. 
         * Any additional colors (if any) are not used.  This is typically
         * the case with legend flags 2 & 3 (explicitly stated legend 
         * values).
         */
  
    /* If the palette size does not equal the total number of levels, 
     * we need the following inputs to determine which color to use:
     * p16_tot_n_lvls     the total number of data levels including flags
     * palette_size     the number of colors defined in the cvg palette
     *                  this is not the number of unique colors defined
     * p16_n_l_flags   the number initial values that are flags (if any)
     * p16_n_t_flags  the number of trailing values that are flags (if any)
     */
    
    
    /* use the screen data as the source of parameters */
    /* permits future change in replay history         */
   
    if( (digital_flag==1) || (digital_flag==2) || (digital_flag==3) ) {
       p16_tot_n_lvls = sd->dhr->decode.tot_n_levels;
       p16_n_l_flags  = sd->dhr->decode.n_l_flags;
       p16_n_t_flags  = sd->dhr->decode.n_t_flags;
       
    } else if( (digital_flag==4) || (digital_flag==5) || (digital_flag==6) ) {
       if(sd->dhr->decode.decode_flag == PROD_PARAM ) {
           p16_tot_n_lvls = sd->dhr->decode.prod_max_level + 1;
           p16_n_l_flags  = sd->dhr->decode.prod_n_l_flags;
           p16_n_t_flags  = sd->dhr->decode.prod_n_t_flags;
       } else {
           p16_tot_n_lvls = sd->dhr->decode.max_level + 1;
           p16_n_l_flags  = sd->dhr->decode.n_l_flags;
           p16_n_t_flags  = sd->dhr->decode.n_t_flags;
       }
       
    } /* end if generic method */
    
/* TEST */
/*fprintf(stderr,"TEST output_packet_16 - parameters read from legend file are\n" */
/*              "     p16_tot_n_lvls %lu,  p16_n_l_flags %d,  p16_n_t_flags %d\n",*/
/*              p16_tot_n_lvls, p16_n_l_flags, p16_n_t_flags);                    */
/*if(sd->dhr->decode.decode_flag == PROD_PARAM)                                   */
/*    fprintf(stderr,"     decode_flag is PROD_PARAM\n");                         */
/*else if(sd->dhr->decode.decode_flag == FILE_PARAM)                              */
/*    fprintf(stderr,"     decode_flag is FILE_PARAM\n");                         */
/*else if(sd->dhr->decode.decode_flag == NO_DECODE)                               */
/*    fprintf(stderr,"     decode_flag is NO_DECODE\n");                          */
/*else if(sd->dhr->decode.decode_flag == LEG_VELOCITY)                            */
/*    fprintf(stderr,"     decode_flag is LEG_VELOCITY\n");                       */
    
    /* if number of data levels is out of range, abort product display */
    /* NOTE: IF LOGIC EXTENDED TO HANDLE unsigned shorts, WOULD NEED */
    /*       TO TEST FOR MAX NUM LEVELS OF 65536 */
   
    if ( p16_tot_n_lvls <= 0 ||  p16_tot_n_lvls > 256) {
        p16_tot_n_lvls = 256;

        fprintf(stderr, 
                "* ERROR ********************************************\n");
        fprintf(stderr, 
                "* Number of data levels out of range (1-256)\n");
        fprintf(stderr, 
                "*        Aborting product display                  \n");
        fprintf(stderr, 
                "****************************************************\n");
        return;
    }




    /******************************************************************/
    /* IMPORTANT NOTE:                                                */
    /*     The maximum number of data levels that could be used with  */
    /*     a digital legend flag of 1, 2 or 3, is 512..  If this is   */
    /*     attempted the legend blocks will not display               */
    /******************************************************************/
    
    
    /* if the palette size is, for whatever reason, 0,
     * we don't want to be printing the product */
    if ( palette_size <= 0 ||  palette_size > 256) {
       fprintf(stderr, "* ERROR ********************************************\n");
       fprintf(stderr, "*        Number of colors out of range (1-256)     *\n");
       fprintf(stderr, "*        Aborting product display                  *\n");
       fprintf(stderr, "****************************************************\n");
       return;
    }


    if( (digital_flag==1) || (digital_flag==2) || (digital_flag==3) ) {   
        color_scale = calculate_color_scale();
    }
    
    
    
    

    /***********************************************************************/
    /*  SECTION 4 DRAW THE RADIAL IMAGE                                    */
    /***********************************************************************/

/* TEST LOOKUP COLOR FUNCTION */
/*for(i=0; i <= sd->dhr->decode.max_level; i++) {                */
/*    fprintf(stderr,"DEBUG LOOKUP COLOR - index %d, color %d\n",*/
/*                   i, lookup_gen_color(i) );                   */
/*}                                                              */
/* END TEST LOOKUP COLOR */

 

 /* For each beam in the product, display it using the product  *
  * value as a unique color in the display.             */
    for (beam=0;beam<sd->dhr->number_of_radials;beam++) {
        float extra_width = 1.5; /* Added to eliminate black speckles between radials */ 
        if((beam==0) && (verbose_flag)) { 
            int n; 
        
            printf("Print radial %d: angle %6.2f delta %6.2f\n", 
            beam+1, sd->dhr->azimuth_angle[beam],
                       sd->dhr->azimuth_width[beam]);
             
            for(n=0;n<sd->dhr->data_elements;n++)
                printf("%03i ",sd->dhr->radial_data[beam][n]); 
            printf("\n"); 
        
        }
        /* Don't draw such a wide beam for the last radial */
        if (beam == sd->dhr->number_of_radials -1) extra_width = 0.5; 
        azimuth1 = sd->dhr->azimuth_angle [beam];
        azimuth2 = azimuth1 + sd->dhr->azimuth_width [beam] + extra_width;
  
        sin1 = sin ((double) (azimuth1+90)*DEGRAD);
        sin2 = sin ((double) (azimuth2+90)*DEGRAD);
        cos1 = cos ((double) (azimuth1-90)*DEGRAD);
        cos2 = cos ((double) (azimuth2-90)*DEGRAD);
  
        old_Color = -1;
        Color = -1;

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
        
        for (i=0; i < sd->dhr->data_elements; i++) {
           /* handle setting the color */
           data_level = (unsigned int) sd->dhr->radial_data[beam][i];


           if( (digital_flag==1) || (digital_flag==2) || (digital_flag==3) ) {   

                Color = data_level_to_color(data_level);
                
           } else if((digital_flag==4) || (digital_flag==5) || (digital_flag==6)) {
            
                Color = lookup_gen_color(data_level);
           }

           if(Color == -1) {
                Color = 0;
                fprintf(stderr,
                        "ERROR Pkt 16 - Color for data level %u not found\n",
                                                            data_level);
           }
            
/*  THE FOLLOWING ASSUMES THAT COLOR 0 IS BACKGROUND (I.E. BLACK or WHITE) */

            /*  If successive colors are the same we draw as one large segement */
            if (Color != old_Color) {

                X[2].x = i * cos2 * scale_x + center_pixel;
                X[2].y = i * sin2 * scale_y + center_scanl;
                X[3].x = i * cos1 * scale_x + center_pixel;
                X[3].y = i * sin1 * scale_y + center_scanl;
    
                XSetForeground(display, gc, display_colors[old_Color].pixel);
                XFillPolygon(display, sd->pixmap, gc, X, 4, Convex, 
                                                            CoordModeOrigin);
    
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
        i = sd->dhr->data_elements;        
        X[2].x = (i-1) * cos2 * scale_x + center_pixel;
        X[2].y = (i-1) * sin2 * scale_y + center_scanl;
        X[3].x = (i-1) * cos1 * scale_x + center_pixel;
        X[3].y = (i-1) * sin1 * scale_y + center_scanl;
       
        XSetForeground(display, gc, display_colors[old_Color].pixel);
        XFillPolygon(display, sd->pixmap, gc, X, 4, Convex, CoordModeOrigin);

    }   /* end for each beam do */
    
    
} /* end output_packet_16() */




/*******************************************************************************/
/*** HELPER FUNCTIONS                                                        ***/


double calculate_color_scale()
{
    
double scale_val;
    
    num_flags = p16_n_l_flags + p16_n_t_flags;
    num_data_lvls = p16_tot_n_lvls - num_flags;
    /* index of first numerical (non flag) value */
    first_ind = 0 + p16_n_l_flags; 
    /* index of last numerical (non flag) value */
    last_ind = first_ind + (num_data_lvls - 1);
    /* index of last color used for a numerical value */
    last_num_color = palette_size - num_flags;


     /* PROTECTION FOR PALETTE TOO SMALL */
     if(palette_size <= num_flags) {
         scale_val = p16_tot_n_lvls;
         fprintf(stderr, 
             "* ERROR Packet 16***********************************************\n");
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
     
         scale_val = ceil( (double)(p16_tot_n_lvls - num_flags) / 
                 (double)(palette_size - num_flags) );
     }

     
     /* should not have more colors than data levels */
     /* if so, colors are truncated */
     if (scale_val < 1) {
        scale_val = 1;
        fprintf(stderr, 
              "* WARNING ****************************************************\n");
        fprintf(stderr, 
              "* The color palette is larger than the number of data levels *\n");
        fprintf(stderr, 
              "* The excess colors (at top end) are not used                *\n");
        fprintf(stderr, 
              "**************************************************************\n");
     }
    
    return(scale_val);    
    
} /*  end calculate_color_scale */






int data_level_to_color(unsigned int d_level)
{
unsigned int calc_color;

            
    if(d_level < first_ind)  /* leading flag */
        calc_color = d_level;
    else if (d_level > last_ind) /* trailing flag */
        calc_color = last_num_color + (d_level - last_ind);
    else  {
        calc_color = ( (d_level - p16_n_l_flags) / (int)color_scale ) + 
                   p16_n_l_flags;
        if (calc_color > last_num_color)
            calc_color = last_num_color;
    }
    
    return((int)calc_color);
    
} /*  end data_level_to_color */




int lookup_gen_color(unsigned int d_level)
{
int i;
    

    for(i=1; i < sd->dhr->decode.n_leg_colors; i++ ) {
       if(d_level < sd->dhr->color.data.ui_dl[i]) { 
          return(sd->dhr->color.dl_clr[i-1]); 
       }
    }

    if(d_level >= sd->dhr->color.data.ui_dl[sd->dhr->decode.n_leg_colors-1])
       return(sd->dhr->color.dl_clr[sd->dhr->decode.n_leg_colors-1]);
    
    return(-1);
    
    
} /*  end lookup_gen_color */

