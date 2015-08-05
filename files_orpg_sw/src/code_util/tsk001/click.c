/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/17 15:41:26 $
 * $Id: click.c,v 1.13 2012/09/17 15:41:26 steves Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */
/* click.c */

#include "click.h"


/* action for the on-screen canvas that reports information about the specific
 * bin of data, if a binned product has been plotted 
 */


/* file scope 'click_sd' safer to use than global 'sd' */
screen_data *click_sd;

decode_params_t *decode_ptr=NULL;
color_assign_t  *color_ptr=NULL;



/* takes the location of the click on the canvas and reports the information
 * about the last plotted "base product"
 * input parameters inx and iny are the x and y coordinates of the mouse 
 * postion measured from the upper left of the drawing area
 */
void do_click_info(int screen_num, int inx, int iny)
{
  int c_x, c_y; /* the saved original click location (inx and iny) */

  int atx, aty; /* coordinates used as inputs to the atan2 function */
            /* transformed in order to provide azimuth relative */
            /* to the y axis rather than the x axis             */ 

  int i, i2, /* index for radial number */
      j;     /* index for range bin */
      
  int pixel_start, scanl_start;  /* upper left corner of raster image */
  int col, row; /* calculated row colomn of raster image */
  
  float theta,      /* calculated azimuth in degrees (xy to polar */
        range,      /* calculated range in NM */
        rr,         /* calculated range in row/column units (xy to polar) */
        res,        /* configured product resolution in NM */
        azi,        /* azimuth angle from radial header */
        theta_360;  

  float rangeKM;    /* calculated range in KM */

  char buf[150], rangestr[20], azistr[20], radialstr[10];
  char rangeKMstr[20];

  /* Lat Long presentation */
  float elev_angle = 0.0;      /*  elevation angle in degrees   */
  float x_meters, y_meters;    /*  x y coord in meters */
  float click_Lat, click_Long; /*  Lat Long in decimal degrees */
  char  n_s_char, e_w_char;
  int retval;
  
  XmString xmstr;
  Prod_header *hdr;
  Graphic_product *gp;
  int r,      /* calculated range in pixels (xy to polar) */
      atcol,  /* number of columns from middle offset     */
      atrow;  /* number of rows from middle offset        */
  int n_cols, n_rows; /*  size of digital precip array */
  int *type_ptr, msg_type;
  Boolean overlay_at_base=FALSE;


  char decode_val_str[20] = { '-', '-', '-', '-', '-', 
                              '-', '-', '-', '-', '-',
                              '-', '-', '-', '-', '-',
                              '-', '-', '-', '-', '\0'};


/* DEBUG */
/*fprintf(stderr,"DEBUG - ENTER do_click_info\n"); */


  /* set the globals up correctly */
  if(screen_num == SCREEN_1) {
      click_sd = sd1;
      
  } else if(screen_num == SCREEN_2) {
      click_sd = sd2;
      
  } else if(screen_num == SCREEN_3) {
      click_sd = sd3;
      
  } else {  /* if we don't quit while we're ahead, bad things could happen */
    return;
  }

/* save input click location */
c_x = inx;
c_y = iny;

/* CORRECTION FOR OFF-CENTER IMAGE */
inx = inx + (int) (click_sd->x_center_offset * click_sd->scale_factor);
iny = iny + (int) (click_sd->y_center_offset * click_sd->scale_factor);


/* do not respond to click on legend area */
  if(c_x > pwidth)
      return;



  /* don't try to interrogate an empty or cleared screen */
  if(click_sd->layers == NULL || click_sd->history == NULL) {
       fprintf(stderr,"appears to be an empty or cleared screen!\n");
       return;
  }
  if(click_sd->icd_product == NULL) {
       fprintf(stderr,"no product loaded!\n");
       return;
  }


  /* must be configured as a Geographic Product */
  hdr = (Prod_header *)(click_sd->icd_product);
  gp = (Graphic_product *) (click_sd->icd_product+ 96);
  type_ptr = assoc_access_i(msg_type_list, hdr->g.prod_id);
  msg_type = *type_ptr;

/* get resolution from gen_radial */
/*  TO DO - DO I NEED TO LIMIT TO SIG DIGITS??? */
  if(click_sd->last_image == GENERIC_RADIAL) {
     res = click_sd->gen_rad->range_interval / 1000 * CVG_KM_TO_NM;
  } else  {
     res = res_index_to_res(click_sd->resolution);
  }


/* DEBUG */
/*fprintf(stderr,"DEBUG do_click_info - read message type and resolution\n"); */

   /************************************************************************/
   /* if the resolution is not an overlay product and either:
    *    there is no image displayed (meaning no radial / raster data) or
    *    the product is not geographic in nature (with the exception of DPA)
    * then we do not return data
    */      

  if( ( (click_sd->last_image == NO_IMAGE) || 
        ((msg_type != GEOGRAPHIC_PRODUCT) && (gp->prod_code != 81))  )
     
       && (res != 999) ) { /*  last test required if an overlay is displayed first */
      fprintf(stderr,"no image has been plotted, or not a geographic image!\n");
      if(verbose_flag) 
          fprintf(stderr," last_image = %d, msg_type = %d \n", click_sd->last_image, 
                                                                      msg_type );
      return;
  }      
   /************************************************************************/      



  /* if resolution is zero, it is either unknown or N/A */
  /*      we do not return data , there is an exception */
  /*      for the precip array image                    */
    
  if((res == 0.0) && (click_sd->last_image != PRECIP_ARRAY_IMAGE)) {
        xmstr = XmStringCreateLtoR("", XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(click_sd->data_display, XmNlabelString, xmstr, NULL);
        XmStringFree(xmstr);

        sprintf(buf, "X:%4d\nY:%4d", inx, iny);
        xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(click_sd->data_display,XmNlabelString,xmstr,NULL);
        XmStringFree(xmstr);
    
        xmstr = XmStringCreateLtoR("\nProduct Resolution of 0.0\n"
                                   "Does Not Permit Data Return", 
                                             XmFONTLIST_DEFAULT_TAG);   
        XtVaSetValues(click_sd->data_display_2, XmNlabelString, xmstr, NULL);
        XmStringFree(xmstr);

        if(verbose_flag)
            fprintf(stderr,"No Product Resolution: cannot return meaningful data\n");
        return;
  }

  
  /************************************************************************/  
  
  /* if resolution is 999, it is an overlay product with no underlying base product */
  /*       we assume the overlay product has the standard inherent resolution       */
  if(res == 999) {

     res = CVG_KM_TO_NM;
     
     overlay_at_base=TRUE;
     fprintf(stderr, "DATA QUERY - Overlay Product Displayed at Base Level.\n");
     fprintf(stderr, "             Standard 0.54 NM resolution assumed.\n");
  }      




  /* removes any previous click symbol */
  replot_image(screen_num);
  
  /* draw and show click symbol for overlay at base, for radial RLE, for 
   *         raster RLE, for digital radial, and for generic radial
   *         but not for precip array (LFM grid).
   */
  if( (overlay_at_base==TRUE) || (click_sd->last_image == RLE_IMAGE) ||
      (click_sd->last_image == RASTER_IMAGE) || 
      (click_sd->last_image==DIGITAL_IMAGE) ||
      (click_sd->last_image==GENERIC_RADIAL) ) {
        XSetForeground(display, gc, white_color);
        /*  horizontal */
        XDrawLine(display, click_sd->pixmap, gc, c_x+6, c_y, c_x+16, c_y);
        XDrawLine(display, click_sd->pixmap, gc, c_x-6, c_y, c_x-16, c_y);
        /*  vertical */
        XDrawLine(display, click_sd->pixmap, gc, c_x, c_y-6, c_x, c_y-16);
        XDrawLine(display, click_sd->pixmap, gc, c_x, c_y+6, c_x, c_y+16);
        /*  diagonal */
        XDrawLine(display, click_sd->pixmap, gc, c_x+5, c_y+5, c_x+12, c_y+12);
        XDrawLine(display, click_sd->pixmap, gc, c_x-5, c_y-5, c_x-12, c_y-12);
        XDrawLine(display, click_sd->pixmap, gc, c_x+5, c_y-5, c_x+12, c_y-12);
        XDrawLine(display, click_sd->pixmap, gc, c_x-5, c_y+5, c_x-12, c_y+12); 
  }

  show_pixmap(screen_num); 

  
 /* NOW WE DISPLAY CLICK INFORMATION BASED UPON TYPE OF GEOGRAPHIC PRODUCT */

 /************************************************************************/
 /* click point azimuth, range, and lat long reported for overlay at base*/
 /************************************************************************/
 
  if(overlay_at_base==TRUE) {
    /* A special case.  We report azimuth and range but not */
    /* radial number, bin number, or bin value.             */
    /* find the coordinates of the click given that the origin is in the center 
    of the draw palette. azimuthal angles start due north, so rotate them 90 
    degrees for good measure.  */


    atx = -( iny - (pheight/2) );  /* x' =  y */
    aty = inx - (pwidth/2);   /* y' = -x */

    /* now, transform these to polar coordinates */

    rr = sqrt((double)(atx*atx + aty*aty));

    theta = atan2(aty, atx) * CVG_RAD_TO_DEG;

    /* atan2() returns a value from -180 to 180, we want something from 0 to 360 */
    if(theta < 0)  theta += 360;    

      /* rescale the point on screen to a radial bin -- it's not terribly
       * intuitive, but you can figure it out by working with a radial
       * along one of the axes */

    j = ( rr )/ click_sd->scale_factor;      
       
    range = j * res;

    rangeKM = range / CVG_KM_TO_NM;

    if(range == 0.0f) {
       sprintf(rangestr, "0 nm");
       sprintf(rangeKMstr, "(0 km)");
       sprintf(azistr, "N/A");
    }
    else { /*  overlay not at base */
       sprintf(rangestr, "%6.2fnm", range);
       sprintf(rangeKMstr, "(%6.2fkm)", rangeKM);
       sprintf(azistr, "%6.2f", theta);
    }

    /* convert azran to Lat Long here */
    /*  for now assume elev_angle default value of 0.0 deg */
    retval = _88D_azranelev_to_xy( rangeKM*1000.0, theta, elev_angle,
                                          &x_meters, &y_meters );
    retval = _88D_xy_to_latlon( x_meters/1000.0, y_meters/1000.0, 
                                          &click_Lat, &click_Long );
    if(click_Lat > 0) {
        n_s_char = 'N';
    } else {
        n_s_char = 'S';
        click_Lat = -1.0 * click_Lat;
    }
    if(click_Long > 0) {
        e_w_char = 'E';
    } else {
        e_w_char = 'W';
        click_Long = -1.0 * click_Long;
    }


    /* send the output to the correct field */
    sprintf(buf, "X:%4d\nY:%4d", inx, iny);
    xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(click_sd->data_display,XmNlabelString,xmstr,NULL);
    XmStringFree(xmstr);

    /* cvg internal bin numbers begin with 0, external begin with 1 */
    /* so we report bin number j+1 */
    sprintf(buf, "           \n"
                 "Azimuth: %s\n"
                 "Range: %s  %s\n "
                 "       %6.3f %c       %6.3f %c", 
        azistr, rangestr, rangeKMstr,
        click_Lat, n_s_char, click_Long,  e_w_char);

    xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(click_sd->data_display_2,XmNlabelString,xmstr,NULL);
    XmStringFree(xmstr);      

 
  /************************************************************************/
  /* only one type of raster image: RLE                                   */
  /************************************************************************/
  
  } else if(click_sd->last_image == RASTER_IMAGE) {
    /* if we're dealing with raster data, then we can just transform screen
     * coordinates to raster data coordinates
     */    

    pixel_start = (pwidth - click_sd->raster->number_of_columns * pixel_int) / 2;
    scanl_start = (pwidth - click_sd->raster->number_of_rows * scanl_int) / 2;


    col = (inx - pixel_start) / click_sd->scale_factor;
    row = (iny - scanl_start) / click_sd->scale_factor;

    /* we need to do some gymnastics to get range and azimuth info from this
     * first, we transform the coordinates so that the origin is in the center
     * of the pixmap (which should be the center of the image
     * then, we do about the same funky trigonometry as we do for radial products
     */

    atcol = col - click_sd->raster->number_of_columns/2;
    atrow = row - click_sd->raster->number_of_rows/2;
    rr = sqrt((double)(atcol*atcol + atrow*atrow));

    theta = atan2(atcol, -atrow) * CVG_RAD_TO_DEG;
    
    /* atan2() returns a value from -180 to 180, we want something from 0 to 360 */
    if(theta < 0)  theta += 360;

    /* now we get the range by combining the grid distance with the resolution */
    range = rr * res;

    rangeKM = range / CVG_KM_TO_NM;

    /* _now_ we turn it into a string */
    if(range == 0.0f) {
       sprintf(rangestr, "N/A");
       sprintf(rangeKMstr, " ");
    } else {
       sprintf(rangestr, "%6.2fnm", range);
       sprintf(rangeKMstr, "(%6.2fkm)", rangeKM);
    }


    /* convert azran to Lat Long here */
    /*  for now assume elev_angle default value of 0.0 deg */
    retval = _88D_azranelev_to_xy( rangeKM*1000.0, theta, elev_angle,
                                          &x_meters, &y_meters );
    retval = _88D_xy_to_latlon( x_meters/1000.0, y_meters/1000.0, 
                                          &click_Lat, &click_Long );
    if(click_Lat > 0) {
        n_s_char = 'N';
    } else {
        n_s_char = 'S';
        click_Lat = -1.0 * click_Lat;
    }
    if(click_Long > 0) {
        e_w_char = 'E';
    } else {
        e_w_char = 'W';
        click_Long = -1.0 * click_Long;
    }


    sprintf(buf, "X:%4d\nY:%4d", inx, iny);
    xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(click_sd->data_display,XmNlabelString,xmstr,NULL);
    XmStringFree(xmstr);

    /* if the click is outside of the plotted data, then display nothing */
    if((col<0)||(col>=click_sd->raster->number_of_columns)||
       (row<0)||(row>=click_sd->raster->number_of_rows)) {
       
       sprintf(buf, "Selected Point Not in Product\n"
                    "Azimuth: %6.2f\n"
                    "Range: %s  %s\n "
                    "       %6.3f %c       %6.3f %c",
           theta, rangestr, rangeKMstr,
           click_Lat, n_s_char, click_Long,  e_w_char);         

       if(verbose_flag)
          fprintf(stderr,"RASTER IMAGE: selected data point is not within product area\n");

    } else { /* selected point within product */

    /*          and need to add 1 to row and column for display  */
       sprintf(buf, "Row %03d  Col %03d    Value %03d\n"
                    "Azimuth: %6.2f\n"
                    "Range: %s  %s\n "
                    "       %6.3f %c       %6.3f %c",
          row+1, col+1, click_sd->raster->raster_data[row][col], theta, rangestr, rangeKMstr,
          click_Lat, n_s_char, click_Long,  e_w_char);      
    }

    xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(click_sd->data_display_2,XmNlabelString,xmstr,NULL);
    XmStringFree(xmstr);
    

   /************************************************************************/
   /* no azimuth, range, or lat long reported for LFM grid precip array    */
   /************************************************************************/

  } else if(click_sd->last_image == PRECIP_ARRAY_IMAGE) {  

    /* Beginning with CVG 6.5, the DPA product is treated as a non-geographic product 
     * because the improved image is not actually projected on a 1/40 LFM grid (neither
     * was the prior display).  The data click returns the row and column in the grid
     * and the data level. Azimuth and range are not reported.
     */
     
    n_cols = click_sd->digital_precip->number_of_columns;
    n_rows = click_sd->digital_precip->number_of_rows;
    
    /*  p_17_g_size is the number of pixels (x and y) used for each data point */
    /*  the upper left corner of the grid is at p17_orig_x, p17_orig_y */
    
    col = ( (inx - p17_orig_x) / p17_g_size ) + 1;  /*  the column */
    row = ( (iny - p17_orig_y) / p17_g_size ) + 1;  /*  the row */
    
    if( (inx < p17_orig_x) || (iny < p17_orig_y) ||
        (inx > (p17_orig_x + p17_g_size*n_cols)) || 
        (iny > (p17_orig_y + p17_g_size*n_rows)) ) {

        sprintf(buf, "X:%4d\nY:%4d", inx, iny);
        xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(click_sd->data_display,XmNlabelString,xmstr,NULL);
        XmStringFree(xmstr);
        sprintf(buf, "Selected Point\n"
                     "Not in Product");
        xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);    
        XtVaSetValues(click_sd->data_display_2, XmNlabelString, xmstr, NULL);
        XmStringFree(xmstr);            
        
        if(verbose_flag)
            fprintf(stderr,"PRECIP ARRAY IMAGE IMAGE: selected point is not within product area\n");
        return;
    }
        
    sprintf(buf, "X:%4d\nY:%4d", inx, iny);
    xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(click_sd->data_display,XmNlabelString,xmstr,NULL);
    /*   here we do NOT need to add 1 to the row or column but need */
    /*   to subtract one when indexing the data array */

        sprintf(buf, " Row #%03d    Column #%03d\n"
                 " Bin Value: %03d\n",
        row, col, click_sd->digital_precip->raster_data[row-1][col-1]);       
    xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(click_sd->data_display_2,XmNlabelString,xmstr,NULL);
    XmStringFree(xmstr);



    
   /************************************************************************/
   /* All radial images have a common calculation for Azimuth, the range   */
   /* calculation and display varies by type, digitgal and generic radial  */
   /* images can have decdoded value displayed                             */
   /************************************************************************/

  } else if( (click_sd->last_image == RLE_IMAGE)      || 
             (click_sd->last_image == DIGITAL_IMAGE)  || 
             (click_sd->last_image == GENERIC_RADIAL)
                                                   ) {  
    int number_of_radials=0;
    int data_elements=0;
    float range_interval=0;
    unsigned short index_to_first_range_bin=0; /*  almost allways 0 */
    float  *azimuth_angle=NULL;
    float  *azimuth_width=NULL;

/*  TO DO - support all types for generic radial */
    unsigned short **radial_data=NULL;
    unsigned char  **radial_ubyte_data=NULL;

    char img_title[21];
    
    unsigned int data_val=0;

/* DEBUG */
/* fprintf(stderr,"DEBUG do_click_info - begin radial image processing\n"); */
   
    /* set several variables based upon type of radial product */
    if(click_sd->last_image == RLE_IMAGE) {
        if(verbose_flag)
          fprintf(stderr,"RLE_IMAGE\n");
        number_of_radials = click_sd->radial->number_of_radials;
        data_elements = click_sd->radial->data_elements;
        range_interval = click_sd->radial->range_interval;
        index_to_first_range_bin = 0;
        azimuth_angle = click_sd->radial->azimuth_angle;
        azimuth_width = click_sd->radial->azimuth_width;

        radial_data = click_sd->radial->radial_data;        
        
        strcpy(img_title, "RADIAL RLE IMAGE");
        
    } else if(click_sd->last_image==DIGITAL_IMAGE) {
        if(verbose_flag) 
          fprintf(stderr,"DIGITAL_IMAGE\n");  
        number_of_radials = click_sd->dhr->number_of_radials;
        data_elements = click_sd->dhr->data_elements;
        range_interval = click_sd->dhr->range_interval;
        index_to_first_range_bin = 1;
        azimuth_angle = click_sd->dhr->azimuth_angle;
        azimuth_width = click_sd->dhr->azimuth_width;

        decode_ptr = &click_sd->dhr->decode;
        color_ptr = &click_sd->dhr->color;
        
        radial_data = click_sd->dhr->radial_data;
        
        strcpy(img_title, "DIGITAL RADIAL IMAGE");
    } else if(click_sd->last_image==GENERIC_RADIAL) {
/* DEBUG */
/* fprintf(stderr,"DEBUG do_click_info - setting variables for generic radial\n"); */

        number_of_radials = click_sd->gen_rad->number_of_radials;
        data_elements = click_sd->gen_rad->data_elements;
        range_interval = click_sd->gen_rad->range_interval;
        index_to_first_range_bin = 1;
        azimuth_angle = click_sd->gen_rad->azimuth_angle;
        azimuth_width = click_sd->gen_rad->azimuth_width;

        decode_ptr = &click_sd->gen_rad->decode;
        color_ptr = &click_sd->gen_rad->color;
/*  TO DO - Support all data types  */
        if (click_sd->gen_rad->type_data == DATA_UBYTE) {
        if(verbose_flag)
          fprintf(stderr,"GENERIC RADIAL DATA TYPE UBYTE\n");
          radial_ubyte_data = click_sd->gen_rad->data.ubyte_d;
        }
        else {
        if(verbose_flag)
          fprintf(stderr,"GENERIC RADIAL DATA TYPE USHORT ASSUMED\n");
          radial_data = click_sd->gen_rad->data.ushort_d;
        }
        
        strcpy(img_title, "GENERIC RADIAL IMAGE");        
            
    }

/* DEBUG */
/* fprintf(stderr,"DEBUG do_click_info - finished reading screen data\n"); */

    /* radial data is a little tougher. first, find the coordinates of the click
     * given that the origin is in the center of the draw palette.
     * azimuthal angles start due north, so rotate them 90 degrees for good measure.
     */
        
    atx = -( iny - (pheight/2) );  /* x' =  y */
    aty = inx - (pwidth/2);   /* y' = -x */

    /* now, transform these to polar coordinates */
    r = (int)sqrt((double)(atx*atx + aty*aty));

    theta = atan2(aty, atx) * CVG_RAD_TO_DEG;

    /* atan2() returns a value from -180 to 180, we want something from 0 to 360 */
    if(theta < 0)  theta += 360;

    /* now, we have to deal with two different radial formats here */
    /* For WSR-88D radial data:
         a. The range is the near edge (closest to radar) of the sampled area.
         b. The azimuth angle is the leading edge (clockwise rotation) 
            of the sampled area.
        CVG plots the data in this manner
    */

/* ///////////////////////////////////////////////////////// */

      /* We cycle once through the radial data comparing the click azimuth
         (theta) with the leading edge of the radial */
            
      for(i=0; (azimuth_angle[i] > theta) && 
           (i < number_of_radials) &&
           (azimuth_angle[i] > azimuth_angle[i-1]); i++);
/* DEBUG */
/* if(verbose_flag) fprintf(stderr,"DEBUG do_click_info - finished first loop\n"); */

      /* After the first loop, we are either still at index 0, 
         or just past azimuth 360. The special case is just past theta and 360. */

      for(; (azimuth_angle[i] < theta ) &&           
             (i < number_of_radials) &&                
             (azimuth_angle[i] > azimuth_angle[i-1]); 
             i++);

      /* After the second loop, we are either just past theta,
         or just past azimuth 360. The special case is just past theta and 360. */

       i2 = i; /* special case handled in last conditional of third loop */

      /* if second loop stopped after 360 degrees before passing theta */ 
      /* or if second loop stopped on index 0 (at or just past 360 degrees) */
      if( ( (azimuth_angle[i-1] > theta) && (azimuth_angle[i] < azimuth_angle[i-1]) )
            || ( i2==0 ) )   
         for(; (azimuth_angle[i] < theta ) && (i < number_of_radials); i++);      

      /* After executing these loops, we have either stopped with the first radial
         having a leading azimuth just past theta (clockwise rotation) or when past 
         the last radial. 
         Note: the last radial has an index of number_of_radials - 1.*/          
      i--;
         
      /* if we ever have a radial product without complete angular coverage */
      /* it is possible to click in a blank spot between radial data */
      /* In this case the resulting radial index i is the last radial */
      if(theta < azimuth_angle[i])
         theta_360 = theta + 360.0;
      else
         theta_360 = theta;

/* DEBUG */
/* fprintf(stderr,"DEBUG do_click_info - begin determining range bin\n"); */

      /* for j; rescale the point on screen to a radial bin -- it's not terribly
       * intuitive, but you can figure it out by working with a radial
       * along one of the axes */
      /* range is reported as the near edge of the sample bin. If we wanted */
      /* to use the far edge, we would use j+1 as the second parameter */       
       
      if(click_sd->last_image == RLE_IMAGE) {
          j = ( (float)r ) / click_sd->scale_factor - index_to_first_range_bin;
          range = (j + index_to_first_range_bin) * res;

      } else {
          j = ( (float)r )/ click_sd->scale_factor;
          range = j * res;
      }
      
      rangeKM = range / CVG_KM_TO_NM;
      
      azi = azimuth_angle[i];
     
      /* report azimuth, radial, and data level at 0 NM range */
      if(range == 0.0f) {
         sprintf(rangestr, "0 NM");
         sprintf(rangeKMstr, "(0 KM)");

      }
      else {
         sprintf(rangestr, "%6.2fnm", range);
         sprintf(rangeKMstr, "(%6.2fkm)", rangeKM);

      }

      /* CVG 8.7 reduced azimuth from 1/100 deg to 1/10 deg display precision */
      sprintf(azistr, "%5.1f", azi);
      /* cvg internal radial numbers begin with 0, external begin with 1 */
      /* so we report radial i+1 */
      sprintf(radialstr, "%03d", i+1);

      /*  TO DO - support all types for generic radial          */
      if (click_sd->last_image==GENERIC_RADIAL) {
         if (click_sd->gen_rad->type_data == DATA_UBYTE) {
             data_val = (unsigned int) radial_ubyte_data[i][j];
         }
         else {
             data_val = (unsigned int) radial_data[i][j];
         }
      }
      else {
         data_val = (unsigned int) radial_data[i][j];
      }

      if( (click_sd->last_image == DIGITAL_IMAGE)  || 
          (click_sd->last_image == GENERIC_RADIAL) ) {
         
         if(decode_ptr->decode_flag != NO_DECODE)
            click_decode_level(data_val, decode_val_str);
        
      } 

/* DEBUG */
/* fprintf(stderr,"DEBUG do_click_info - converting azran to lat lon\n");*/

      /*  convert azran to Lat Long here */
      /*  for now assume elev_angle default value of 0.0 deg */
      retval = _88D_azranelev_to_xy( rangeKM*1000.0, azi, elev_angle,
                                            &x_meters, &y_meters );
      retval = _88D_xy_to_latlon( x_meters/1000.0, y_meters/1000.0, 
                                            &click_Lat, &click_Long );
      if(click_Lat > 0) {
          n_s_char = 'N';
      } else {
          n_s_char = 'S';
          click_Lat = -1.0 * click_Lat;
      }
      if(click_Long > 0) {
          e_w_char = 'E';
      } else {
          e_w_char = 'W';
          click_Long = -1.0 * click_Long;
      }

      /* send the output to the correct field */
      sprintf(buf, "X:%4d\nY:%4d", inx, iny);
      xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
      XtVaSetValues(click_sd->data_display,XmNlabelString,xmstr,NULL);
      XmStringFree(xmstr);


      /* if difference between theta and leading azimuth is too large */
      if( (theta_360 - azimuth_angle[i]) >
          (1.5*azimuth_width[i]) ) {          

         sprintf(buf, "Selected Point Not on a Radial\n"
                      "Azimuth: %s\n"
                      "Range: %s %s\n "
                      "       %6.3f %c       %6.3f %c",
             azistr, rangestr, rangeKMstr,
             click_Lat, n_s_char, click_Long,  e_w_char);

         if(verbose_flag)
             fprintf(stderr,"%s: selected data point is not on radial azimuth\n",
                                                                           img_title);


      } else if((j >= data_elements) || (j<0)) {

         sprintf(buf, "Point Exceeds Product Range\n"
                      "Azimuth: %s\n"
                      "Range: %s  %s\n "
                      "       %6.3f %c       %6.3f %c",
             azistr, rangestr, rangeKMstr,
             click_Lat, n_s_char, click_Long,  e_w_char);
    
         if(verbose_flag)
             fprintf(stderr,"%s: selected data point exceeds product range\n",
                                                                            img_title);

      } else {

      /* cvg internal bin numbers begin with 0, external begin with 1 */
      /* so we report bin number j+1 */
/*  TO DO support all types of data for the generic radial */
      sprintf(buf, "Radial %s  Bin %04d    Val %05u\n"
                   "Azimuth: %s   %s\n"
                   "Range: %s  %s\n "
                   "       %6.3f %c       %6.3f %c",
          radialstr, j+1, data_val,

/*  TO DO support all types of data for the generic radial */
          azistr, decode_val_str, 
          rangestr, rangeKMstr,
          click_Lat, n_s_char, click_Long,  e_w_char);

     }

      xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
      XtVaSetValues(click_sd->data_display_2,XmNlabelString,xmstr,NULL);
      XmStringFree(xmstr);   
    
    /* ******************************************************** */

     
  } /* end it is some kind of radial data */

  
  
} /* end do_click_info */







void click_decode_level(unsigned int d_level, char *decode_val)
{


double decode_real = 0;
int label_index = 0;

/* parameters read from screen data */
int   n_l_flags = 0;
int   n_t_flags = 0;
int   n_leg_colors = 0; /* sizes arrays below and used to calculate array index */
/* CVG 8.8 BUG FIX - changed to type float */
float   Scale = 0.0;
float   Offset = 0.0;
unsigned int max_d_lvl = 0; /* used to locate trailing flags */
label_string *l_label;  /* an array of legend threshold labels */
unsigned int *ud_lvl;   /* an array of data levels for threshold labels */

    if(decode_ptr->decode_flag == NO_DECODE) {
        return;
        
    }

    ud_lvl    = color_ptr->data.ui_dl;
    l_label   = color_ptr->dl_label;
    
    n_leg_colors = decode_ptr->n_leg_colors;
    
    if(decode_ptr->decode_flag == FILE_PARAM) {
        n_l_flags = decode_ptr->n_l_flags;
        n_t_flags = decode_ptr->n_t_flags;
        max_d_lvl = decode_ptr->max_level;
        Scale     = decode_ptr->leg_Scale;
        Offset    = decode_ptr->leg_Offset;
        
    } else if(decode_ptr->decode_flag == PROD_PARAM) {
        n_l_flags = decode_ptr->prod_n_l_flags;
        n_t_flags = decode_ptr->prod_n_t_flags;
        max_d_lvl = decode_ptr->prod_max_level;
        Scale     = decode_ptr->prod_Scale;
        Offset    = decode_ptr->prod_Offset;
        
    } else if(decode_ptr->decode_flag == LEG_VELOCITY) {
        /* future support of velocity products with method 5 */
        /* will use hard coded parameters plus velocity mode in PDP */
        return;
        
    }
    
    
    /* protect against division by 0 */
    if(Scale == 0.0) {
        sprintf(decode_val, "See Error Message");
        fprintf(stderr,"CONFIGURATION ERROR - Attempting to use Scale Offset \n"
                       "     for data level decoding with Scale value 0.0\n");
        return;
    }
    /* TO DO - in the future, add the decimal places desired to the screen data */
    /*         for now, we assume 3                                             */
    
    /* DECODING LOGIC - Flag values are not decoded via Scale Offset */
    if( (n_l_flags==0) && (n_t_flags==0) ) {
        decode_real = ( ((double)d_level) - ((double)Offset) ) / (double)Scale;
        sprintf(decode_val, "Decoded %8.4f", decode_real);
        
    } else if( (n_l_flags!=0) && (d_level < n_l_flags) ) {
        /* return leading flag label */
        /* leading flags always begin with data level 0 */
        sprintf(decode_val, "Decoded %s", l_label[d_level]);
        
    } else if( (n_t_flags!=0) && (d_level > (max_d_lvl - n_t_flags)) ) {
        /* return trailing flag label */
        /* could search the ud_lvl array instead of using the following formula */
        label_index = (n_leg_colors - 1) - (max_d_lvl - d_level);
        sprintf(decode_val, "Decoded %s", l_label[label_index]);
        
    } else { /* not a flag value */
        decode_real = ( ((double)d_level) - ((double)Offset) ) / (double)Scale;
        sprintf(decode_val, "Decoded %8.4f", decode_real);
        
    }
    
    
}  /* end click_decode_level */


