/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/08/19 15:11:45 $
 * $Id: prod_disp_legend.c,v 1.2 2009/08/19 15:11:45 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/* prod_disp_legend.c */

#include "prod_disp_legend.h"



/****************************************************************************/
/* SECTION A - DIGITAL LEGENDS                                              */
/****************************************************************************/



/*  HELPER FUNCTION     Method2 1, 2 & 3                                    */
/****************************************************************************/
int read_digital_legend(FILE *leg_file, char *digital_legend_filename, 
                                               int method, int frame_location)
{


int dig_error = FALSE;
int count=0, j, i;
char buf[100], l_str[65];

decode_params_t *sd_decode_ptr=NULL;
color_assign_t  *sd_color_ptr=NULL;

  if(frame_location != PREFS_FRAME) {
     if(sd->last_image==DIGITAL_IMAGE) {
         sd_decode_ptr = &sd->dhr->decode;
         sd_color_ptr = &sd->dhr->color;
     } else if(sd->last_image==PRECIP_ARRAY_IMAGE) {
         sd_decode_ptr = &sd->digital_precip->decode;
         sd_color_ptr = &sd->digital_precip->color;
     }
  }


      /*  FUTURE INPROVEMENT: read each item using read_to_eol which permits */
      /*          comments and improves robustness AND add error printouts   */
      
  if( (method==0) || (method==4) || (method==5) || (method==6) ) {
      fprintf(stderr,"ERROR, read_digital_legend called with invalid method %d\n",
                                                                           method);
      return (LEGEND_ERROR);
  } 
  
  
  
  
     /* Eliminates strange results in certain situations */
     /*  initialize arrays */
     for(i=0; i<=MAX_N_COLOR; i++) {
     /*  the last data level is not a threshold, it is the largest */
     /*  expected data level - TO BE REMOVED */
        /*  FUTURE TO DO - ADD SUPPORT FOR OTHER TYPES */
        ui_dl[i] = 0;
        si_dl[i] = 0;
        d_dl[i] = 0.0;

        bar_color[i] = 0;
        leg_label[i][0] = '\0';
     }
  
  if(method == 1) {   
      
      fscanf(leg_file, "%lu", &tot_num_lvls);
      fscanf(leg_file, "%d", &num_lead_flags);
      fscanf(leg_file, "%d", &num_trail_flags);
      fscanf(leg_file, "%d", &leg_min_val);
      fscanf(leg_file, "%d", &leg_inc);
      fscanf(leg_file, "%d", &leg_min_val_scale);
      fscanf(leg_file, "%d", &leg_inc_scale);
      
  } else if( (method == 2) || (method == 3) ) {
    
      fscanf(leg_file, "%lu", &tot_num_lvls);
      fscanf(leg_file, "%d", &num_lead_flags);
      fscanf(leg_file, "%d", &num_trail_flags);
      leg_min_val = 0;
      leg_inc = 0;
      leg_min_val_scale = 0;
      leg_inc_scale =0;
      
      /*      READ THE EXPLICIT LABELS HERE AND ENTER INTO            */
      /*               SCREEN DATA SO LEGEND ONLY HAS TO BE READ ONCE */
      
      count = 0;
      /* find where the label goes keep reading the legend */
      /* label index(read dl_ind) while there is one */
      while(fscanf(leg_file, "%d", &ui_dl[count]) == 1) {
         /* and read the label itself */
         read_to_eol(leg_file, buf);
      
         /* skip initial white space */
         j=0;
         while(buf[j] == ' ') 
             j++;
                
         strcpy(l_str, &buf[j]); 
        
        /* CVG 8.7 - increased from 8 to 9 */
         if(strlen(l_str)>9) { /*  truncate to string of 9 characters max */
            l_str[9] = '\0';
            fprintf(stderr,"NOTE: Threshold label truncated to 9 characters\n");
         }
        
         strcpy(leg_label[count], l_str);
        
         count++;
      }

    } /* end else Method 2 or Method 3 */
    
          
      /* IMPORTANT NOTE:                                                */
      /*     The maximum number of data levels that should be used with */
      /*     a digital legend flag of 1, 2 or 3 is 512..  If this is    */
      /*     attempted the legend blocks will not display               */
      /******************************************************************/
      if(tot_num_lvls > 512) {
  
          dig_error = TRUE;
          fprintf(stderr,
                  "* WARNING *****************************************\n");
          fprintf(stderr, 
                  "* Number of data levels (%lu) exceeds 512.\n",
                                                             tot_num_lvls);
          fprintf(stderr, 
                  "*        Using more than 512 data levels with a digital\n"
                  "*        legend METHOD of 1, 2, or 3 will result in the\n"
                  "*        legend bars not being displayed.\n"
                  "*        One of the Generic legend files should be used\n"
                  "*        with products containing more than 256 levels.");
          fprintf(stderr, 
                  "****************************************************\n");
      }
      
  
          /*  FUTURE IMPROVEMENT: ADD MORE TESTS FOR ERRORS */
     
      if(dig_error==TRUE) 
          return (LEGEND_ERROR);
 
         
    
      /* if displaying product, set screen data.  */
      /* permits legend to only be read once      */
      if(frame_location != PREFS_FRAME) {
         
         /*  initialize arrays */
         for(i=0; i<=MAX_N_COLOR; i++) {
            sd_color_ptr->data.ui_dl[i] = 0;
            sd_color_ptr->dl_label[i][0] = '\0';
         }
         
         sd_decode_ptr->decode_flag = NO_DECODE;   /* for mouse click query */
         sd_decode_ptr->tot_n_levels = tot_num_lvls;  
         sd_decode_ptr->n_l_flags = num_lead_flags;
         sd_decode_ptr->n_t_flags = num_trail_flags;
         if(method == 1) {
            sd_decode_ptr->leg_min_val = leg_min_val;
            sd_decode_ptr->leg_incr = leg_inc;
            sd_decode_ptr->min_val_scale = leg_min_val_scale; 
            sd_decode_ptr->incr_scale = leg_inc_scale;
         } else { /* function also called with Methods 2 and 3 */
            sd_decode_ptr->leg_min_val = 0;
            sd_decode_ptr->leg_incr = 0;
            sd_decode_ptr->min_val_scale = 0; 
            sd_decode_ptr->incr_scale = 0;
            /* copy legend labels */
            for(i=0; i<=count; i++) {
                sd_color_ptr->data.ui_dl[i] = ui_dl[i];
                strcpy(sd_color_ptr->dl_label[i], leg_label[i]);
            }
         }

      } /* end if not PREFS_FRAME */
    
    
  return(GOOD_LEGEND);

} /* end read_digital_legend */








/*  HELPER FUNCTION     Method 1                                            */
/****************************************************************************/
void display_calculated_dig_labels(Drawable canvas, int h,
                                 int x, int y, int frame_loc)

{
  int i;
  float val;
  
  unsigned int first_ind=0;
  unsigned int last_ind=0;
  int num_flags;
  int y_trail_flags;
  unsigned long num_data_lvls; /*  calculated number of numerical data levels  */
                             /*  (not flags) - all Methods */
  int label_incr;

  char buf[60];
  
  /* local variables */
  unsigned long tot_n_lvls;
  int n_l_flags;  
  int n_t_flags; 
  int l_min_val; 
  int l_inc; 
  int l_min_val_scale; 
  int l_inc_scale; 
  
  decode_params_t *sd_decode_ptr=NULL;
  color_assign_t  *sd_color_ptr=NULL;

  if(frame_loc != PREFS_FRAME) {
     if(sd->last_image==DIGITAL_IMAGE) {
         sd_decode_ptr = &sd->dhr->decode;
         sd_color_ptr = &sd->dhr->color;
     } else if(sd->last_image==PRECIP_ARRAY_IMAGE) {
         sd_decode_ptr = &sd->digital_precip->decode;
         sd_color_ptr = &sd->digital_precip->color;
     }  
  } /* end not PREFS_FRAME */
  
 
/* TEST SCREEN DATA - MUST BE IN PRODUCT FRAME */
/*
 *if(frame_loc != PREFS_FRAME) {
 *fprintf(stderr,"Testing Screen Data for Display of Calculated Labels \n"
 *    "   tot_n_lvls = %u,  n_l_flags = %d, n_t_flags = %d \n"
 *    "   l_min_val is %d, l_min_val_scale is %d, l_inc is %d, l_inc_scale is %d\n"
 *    ,
 *       sd_decode_ptr->tot_n_levels, sd_decode_ptr->n_l_flags,
 *       sd_decode_ptr->n_t_flags, sd_decode_ptr->leg_min_val, 
 *       sd_decode_ptr->min_val_scale, sd_decode_ptr->leg_incr,
 *       sd_decode_ptr->incr_scale);
 *}
 */

     /* use screen data variables IN PROD_FRAME, file globals in PREF_FRAME */
     if(frame_loc != PREFS_FRAME) {
        tot_n_lvls      = sd_decode_ptr->tot_n_levels;
        n_l_flags       = sd_decode_ptr->n_l_flags;
        n_t_flags       = sd_decode_ptr->n_t_flags;
        l_min_val       = sd_decode_ptr->leg_min_val;
        l_inc           = sd_decode_ptr->leg_incr;
        l_min_val_scale = sd_decode_ptr->min_val_scale;
        l_inc_scale     = sd_decode_ptr->incr_scale;
        
     } else { /* use file globals in PREFS_FRAME */
        tot_n_lvls      = tot_num_lvls;
        n_l_flags       = num_lead_flags;
        n_t_flags       = num_trail_flags;
        l_min_val       = leg_min_val;
        l_inc           = leg_inc;
        l_min_val_scale = leg_min_val_scale;
        l_inc_scale     = leg_inc_scale;
       
     }

/* DEBUG */
/*fprintf(stderr,                                                                  */
/*     "DEBUG display_calculated_dig_labels - tot_n_lvls is %lu, \n"               */
/*     "      n_l_flags is %d, n_t_flags is %d, l_min_val is %d, l_inc is %d,\n"   */
/*     "      l_min_val_scale is %d, l_inc_scale is %d \n",                        */
/*     tot_n_lvls,n_l_flags,n_t_flags,l_min_val,l_inc,l_min_val_scale,l_inc_scale);*/
     
     num_flags = n_l_flags + n_t_flags;
/* DEBUG */
/* fprintf(stderr,"DEBUG - beginning to draw the leading flag labels\n"); */
/* fprintf(stderr,"DEBUG - x postion is %d, y position is %d\n", x, y);  */

     
     if (n_l_flags != 0) {
           for (i=1;i<=n_l_flags; i++) {
              sprintf(buf,"FL %d",i);
              XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
              if (h < MIN_FLAG_HT)
                 y += MIN_FLAG_HT;
              else
                 y += h;
           }
     } /* end if leading flags */     

/* DEBUG */
/* fprintf(stderr,"DEBUG - beginning to draw the numerical labels\n"); */
     
     /********************* DRAW LEGEND LABELS ********************/
     
     /* calculate the first & last index for numerical (non-flag) data */
     first_ind = 0 + n_l_flags; 
     num_data_lvls = tot_n_lvls - num_flags;
     last_ind = first_ind + (num_data_lvls - 1);
     /* used to correctly position the trailing flag labels (if any) */
     y_trail_flags = y + num_data_lvls*h;


     /* digital calculated values */
/* DEBUG */
/* fprintf(stderr,"DEBUG - palette_size is %d, num_flags is %d\n", */
/*                               palette_size , num_flags ); */

    /*  PROTECTION FOR PALETTE TOO SMALL */
     if(palette_size <= num_flags) {
         color_scale = 1.0;
         fprintf(stderr, 
             "* ERROR display_legend_blocks draw legend labels method 1*******\n");
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
         color_scale = ceil( (double)(tot_n_lvls - num_flags) / 
               (double)(palette_size - num_flags) );
     }
   
/* DEBUG */
/* fprintf(stderr,"DEBUG - color_scale is %f\n", color_scale); */

     /* should not have more colors than data levels */
     /* if so, colors are truncated */
     if (color_scale < 1.0) {
         color_scale = 1.0;
         fprintf(stderr, 
             "* WARNING ****************************************************\n");
         fprintf(stderr, 
             "* The color palette is larger than the number of data levels *\n");
         fprintf(stderr, 
             "* The excess colors (at top end) are not used                *\n");
         fprintf(stderr, 
             "**************************************************************\n");
     }
/* DEBUG */
/* fprintf(stderr,"DEBUG - color_scale is %f\n", color_scale); */

     /* this loop attempts to always align the numerical data level (non-flag) 
    * threshold labels with the displayed color bars by using the color_scale
    * factor.  The minimum increment between non-flag labels is MIN_LBL_INCR
    * which is 24 pixels.  This scheme works only under certain conditions:
    *     a. palette size = total number of levels & color values in 
    *        the palette are unique
    *     b. palette size is < total number of colors & color values in
    *        the palette are unique
    */
     label_incr = (int)color_scale;
     while (label_incr*h < MIN_LBL_INCR) {
         label_incr = label_incr*2;
     }
/* DEBUG */
/* fprintf(stderr,"DEBUG - x postion is %d, y position is %d\n", x, y);  */
     
     val = ((float)l_min_val / (float)l_min_val_scale);
     /* Currently NOT using contents of threshold fields for digital products */
     /* val = (float)((signed short)thresh[0]) / (float)(l_min_val_scale); */
     
/* TEST */
/* fprintf(stderr,"TEST - l_min_val is %d, l_min_val_scale is %d\n",  */
/*                                          l_min_val, l_min_val_scale); */
/* fprintf(stderr,"TEST - label_incr is %d, l_inc is %d, l_inc_scale is %d\n", */
/*                           label_incr, l_inc, l_inc_scale);  */
/* fprintf(stderr,"TEST - initial calculated value is %f\n", val); */
/*  */
     /* print numerical labels up to last index */
     for(i=first_ind; i<=last_ind; i+=label_incr) {      
         /* no sign in front of 0.0, assumes correct rounding by C library */
         /* avoid '+' before 0.0    since we force '+' to align labels better */
         if (val < 0.05 && val >= 0.0)
           sprintf(buf, " %3.1f", val);
         else
           sprintf(buf, "%+3.1f", val);
           
         XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
         y += (h*label_incr);
         val += label_incr*(l_inc / (float)l_inc_scale);

     } /* end for num_labels */

      
    /*C. IF THERE ARE TRAILING FLAGS, PRINT THEM OUT WITH MIN HEIGHT OF 12 */
     if (n_t_flags != 0) {
           y = y_trail_flags;
           for (i=1;i<=n_t_flags; i++) {
              sprintf(buf,"FT %d",i);
              XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
              if (h < MIN_FLAG_HT)
                 y += MIN_FLAG_HT;
              else
                 y += h;
           }
     } /* end if trailing flags */
 
    
} /*  end display_calculated_dig_labels() */





/*  HELPER FUNCTION     Methods 2 & 3                                       */
/****************************************************************************/
void display_explicit_dig_labels(Drawable canvas, int h,
                                 int x, int y, int frame_loc)
/* CVG 8.7 added frame_loc, removed globals */
{

     int ind;
     int num_flags;
     unsigned int first_ind=0;
     unsigned int last_ind=0; 
     unsigned long num_data_lvls;
     
     int count;
     
     /* local variables */
     unsigned long tot_n_lvls;
     int n_l_flags;  
     int n_t_flags; 
     unsigned int *d_level;
     label_string *l_label;
     
     decode_params_t *sd_decode_ptr=NULL;
     color_assign_t  *sd_color_ptr=NULL;

     if(frame_loc != PREFS_FRAME) {
        if(sd->last_image==DIGITAL_IMAGE) {
            sd_decode_ptr = &sd->dhr->decode;
            sd_color_ptr = &sd->dhr->color;
        } else if(sd->last_image==PRECIP_ARRAY_IMAGE) {
            sd_decode_ptr = &sd->digital_precip->decode;
            sd_color_ptr = &sd->digital_precip->color;
        }   
     } /* end not PREFS_FRAME */
     
     


     /* use screen data variables IN PROD_FRAME, file globals in PREF_FRAME */
     if(frame_loc != PREFS_FRAME) {
        tot_n_lvls      = sd_decode_ptr->tot_n_levels;
        n_l_flags       = sd_decode_ptr->n_l_flags;
        n_t_flags       = sd_decode_ptr->n_t_flags;
        d_level         = sd_color_ptr->data.ui_dl;
        l_label         = sd_color_ptr->dl_label;
        
     } else {
        tot_n_lvls      = tot_num_lvls;
        n_l_flags       = num_lead_flags;
        n_t_flags       = num_trail_flags;
        d_level         = ui_dl;
        l_label         = leg_label;
       
     }


      /*      The explicit legend labels are read in the              */
      /*      read_digital_legend function so this function uses two  */
      /*           data arrays contining the index and the text label */
     
     
     num_flags = n_l_flags + n_t_flags;
 
     /* calculate the first & last index for numerical (non-flag) data */
     first_ind = 0 + n_l_flags; 
     num_data_lvls = tot_n_lvls - num_flags;
     last_ind = first_ind + (num_data_lvls - 1);
    
    ind = 0;
    count = 0;
    
    /* display first label here */
    while (ind <= d_level[count]) {
       if(ind == d_level[count]) {
/* TEST */
/*fprintf(stderr,"TEST Explicit Labels - count is %d, level is %u, label '%s'\n" */
/*               "           y is %d, ind is %d, first_ind is %d\n",             */
/*             count, d_level[count], l_label[count], y, ind, first_ind);        */
             
          XDrawString(display, canvas, gc, x, y, l_label[count], 
                                                strlen(l_label[count]) );
       }
       if(h < MIN_FLAG_HT && (ind < first_ind || ind > last_ind))
          y += MIN_FLAG_HT;
       else
          y += h;
       ind++;
    } /*  end while */

    count = 1;
    while(d_level[count] != 0) {
        /* display next label */
        while (ind <= d_level[count]) {
           if(ind == d_level[count]) {  
/* TEST */
/*fprintf(stderr,"TEST Explicit Labels - count is %d, level is %u, label '%s'\n" */
/*               "           y is %d, ind is %d, first_ind is %d\n",             */
/*             count, d_level[count], l_label[count], y, ind, first_ind);        */
             
              XDrawString(display, canvas, gc, x, y, l_label[count], 
                                                    strlen(l_label[count]) );
           }
           if(h < MIN_FLAG_HT && (ind < first_ind || ind > last_ind))
              y += MIN_FLAG_HT;
           else
              y += h;
           ind++;
        } /*  end while index <= data level */

        count++;
        
    } /* end while data level != 0 */
    

     
} /*  end display_explicit_dig_labels() */






/*  HELPER FUNCTION     Methods 1, 2, & 3                                   */
/****************************************************************************/
void draw_digital_color_bars(Drawable canvas, int h,
                                 int x, int y, int frame_loc)
{

  int i;
  int bh, bw;
  int num_flags;
  unsigned int first_ind=0;
  unsigned int last_ind=0;  
  unsigned long num_data_lvls; /*  calculated number of numerical data levels  */
                             /*  (not flags) - all Methods   */
  unsigned int last_num_color; 
  unsigned int color;
  
  /* local variables */
  unsigned long tot_n_lvls;
  int n_l_flags;  
  int n_t_flags; 
  
  decode_params_t *sd_decode_ptr=NULL;
  color_assign_t  *sd_color_ptr=NULL;

     if(frame_loc != PREFS_FRAME) {
        if(sd->last_image==DIGITAL_IMAGE) {
            sd_decode_ptr = &sd->dhr->decode;
            sd_color_ptr = &sd->dhr->color;
        } else if(sd->last_image==PRECIP_ARRAY_IMAGE) {
            sd_decode_ptr = &sd->digital_precip->decode;
            sd_color_ptr = &sd->digital_precip->color;
        } 
     } /* end not PREFS_FRAME */
  


    /* digital products use number of data levels to */
    /* determine number of blocks                    */
    /* For digital products, if a palette has fewer colors than total 
     * number of levels, all flag values are assinged a unique color 
     * from the beginning OR the end of the palette, the remaining 
     * colors are spread out over the numerical data levels
     */
  
     /* for digital products, we display a product with all data values rather
      * than display the palette */


     /* CVG 8.7 use screen data IN PROD_FRAME, file globals in PREF_FRAME */
     if(frame_loc != PREFS_FRAME) {
        tot_n_lvls      = sd_decode_ptr->tot_n_levels;
        n_l_flags       = sd_decode_ptr->n_l_flags;
        n_t_flags       = sd_decode_ptr->n_t_flags;
        
     } else {
        tot_n_lvls      = tot_num_lvls;
        n_l_flags       = num_lead_flags;
        n_t_flags       = num_trail_flags;
       
     }


     num_flags = n_l_flags + n_t_flags;
     
     /* index of last color used for a numerical value */
     last_num_color = (palette_size - 1) - n_t_flags;

     /*  PROTECTION FOR PALETTE TOO SMALL */
     if(palette_size <= num_flags) {
         color_scale = tot_n_lvls;
         fprintf(stderr, 
            "* ERROR display_legend_blocks draw bars for digital legend******\n");
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
        color_scale = ceil( (double)(tot_n_lvls - num_flags) / 
                             (double)(palette_size - num_flags) );

     }
         
          /* should not have more colors than data levels */
     /* if so, colors are truncated */
     if (color_scale < 1.0) {
        color_scale = 1.0;
        fprintf(stderr, 
            "* WARNING ****************************************************\n");
        fprintf(stderr, 
            "* The color palette is larger than the number of data levels *\n");
        fprintf(stderr, 
            "* The excess colors (at top end) are not used                *\n");
        fprintf(stderr, 
            "**************************************************************\n");
     }
     
     /* bar width allways 50 since the legend bar area is now */
     /* copied into the legend frame rather than displayed    */
     bw = 50;

     /* calculate the first & last index for numerical (non-flag) data */
     first_ind = 0 + n_l_flags; 
     num_data_lvls = tot_n_lvls - num_flags;
     last_ind = first_ind + (num_data_lvls - 1);
     
     
     /* diagnostic info displayed for developer */
     fprintf(stderr,"\nDIGITAL PRODUCT CONFIGURATION INFORMATION\n");
     fprintf(stderr, 
             "The size of the color palette is %d\n", palette_size);
     fprintf(stderr, 
             "Total number of data levels is %lu\n", tot_n_lvls);
     fprintf(stderr, 
             "There are %d leading flag(s) and %d trailing flag(s)\n",
             n_l_flags, n_t_flags);
     fprintf(stderr, 
             "The first numerical data index is %d, the last is %d\n\n", 
             first_ind, last_ind );

     if(verbose_flag == TRUE)
         fprintf(stderr, "\nTHE FOLLOWING COLOR ASSIGNMENTS HAVE BEEN MADE\n");

     
         
     /*  TYPE MISSMATCH i is int, tot_n_lvls is unsigned int */
     /*  this does not cause a problem since methods 1, 2, and 3 work for a */
     /*  maximum of 512 data levels      */
     for (i=0; i < tot_n_lvls; i++) {
        
        /* determine the color (same as data_level_to_color() in packt_16.c) */
        if(i < first_ind) { /* leading flag */
           color = i;
        } else if (i > last_ind) { /* trailing flag */
           color = last_num_color + (i - last_ind);
        } else  { /* numerical data value */
           color = ( (i - n_l_flags) / (int)color_scale ) +
                    n_l_flags;
           if (color > last_num_color)
              color = last_num_color;
        }
        
         if (verbose_flag == TRUE) {
            fprintf(stderr,"Data level index %d is assigned color %d\n", i, color);
         }
         /* determine the bar height */
         if ( h < MIN_FLAG_HT && (i < first_ind || i > last_ind) )
            bh = MIN_FLAG_HT;
         else
            bh = h;
           
            /* outline flag bars if color is black (background) */
         if ((display_colors[color].red == 0 && display_colors[color].green == 0 
             && display_colors[color].blue == 0) 
             && (i < first_ind || i > last_ind) ) { 
            XSetForeground(display, gc, grey_color);
            XDrawRectangle(display, canvas, gc, x, y, bw, bh);
            
         } else {
            XSetForeground(display, gc, display_colors[color].pixel);
            XDrawRectangle(display, canvas, gc, x, y, bw, bh);
            XFillRectangle(display, canvas, gc, x, y, bw, bh);
        }
        
        if ( h < MIN_FLAG_HT && (i < first_ind || i > last_ind) )
             y += MIN_FLAG_HT;
        else
             y += h;
     } /* end for each data level */



} /*  end draw_digital_color_bars */





/****************************************************************************/
/* SECTION B - GENERIC LEGENDS                                              */
/****************************************************************************/



/*  HELPER FUNCTION     Methods 4, 5, & 6                                   */
/****************************************************************************/
int read_generic_legend(FILE *leg_file, char *generic_legend_filename, 
                                               int method, int frame_location)
{
    
    
   /* ----- Input Parameters ---------------
    *
    * leg_file       file pointer to the open legend file
    *
    * frame_location whether the legend is being drawn as part of the product
    *                    display or on the legend preview pane
    * --------------------------------------
    */    

   /* ----- File Globals used if in PREFS_FRAME -------------
    * sets the following variables used by display_generic_legend
    *                                and data_click
    *
    * num_lead_flags       type_leg_file
    * num_trail_flags      num_leg_colors
    * max_level                
    * leg_Scale            param_source
    * leg_Offset           
    *
    * ----Screen Data used if in PROD_FRAME --------------------
    */    

     int i, j, count, position;
     int num_read = 0;
     
     char buf[100], l_str[65];
     /* CVG 9.1 */
     char l_str_cpy[65], *tok1=NULL, *next_tok=NULL;
     unsigned int last_ui_level_read=0;
     int    last_si_level_read=0;
     double last_d_level_read=0.0;
     
     int color_read;
     unsigned int ui_level_read;  
     
     /*  FUTURE TO DO - TEST METHODS 4 AND 6 */
     int    si_level_read;
     double d_level_read;

     unsigned long max_num_levels; /*  based upon array type */
      
    char  source_of_params[85];/*excessively large to avoid crash on legend error*/
    char  num_disp_format[10]; 
    int   dec_places;
    int   field_width;
    int   dig_error = FALSE;
    int   gen_error = FALSE;
    int   line_read=0;
    double real_value=0; /* CVG 8.8 changed to double */
    
    /* CVG 8.8 */
    double product_min_val, product_max_val;
    char prod_min_str[12], prod_max_str[12];
    int no_decode_error;
     
    decode_params_t *sd_decode_ptr=NULL;
    color_assign_t  *sd_color_ptr=NULL;

    if(frame_location != PREFS_FRAME) {
        if(sd->last_image==DIGITAL_IMAGE) {
           sd_decode_ptr = &sd->dhr->decode;
           sd_color_ptr = &sd->dhr->color;
        } else if(sd->last_image==GENERIC_RADIAL) {
           sd_decode_ptr = &sd->gen_rad->decode;
           sd_color_ptr = &sd->gen_rad->color;
        }
    } /* end not PREFS_FRAME */

     
     
     /* This function reads the legend labels and color numbers    */
     /* associated with a data level specified in the legend file. */
     
     /* First, several parameters are read including the generic   */
     /* method the file is intended to be used with, the number    */
     /* number of colors specified in the data level lines, the    */
     /* minimum data level used, the max data level used, the      */
     /* number of leading and trailing flag values and the number  */
     /* of digits desired after the decimal point for dislplay.    */
    
     /* The source of SCALE OFFSET decoding parameters (this file, */
     /* the product, or do not decode), and the values for SCALE   */
     /* and OFFSET are additional parameters read.                 */
     
     /* The data type for the data value depends upon the type of  */
     /* generic product loaded.                                    */
     
     /* Each specified data level is a threshold value for the     */
     /* designated color number in the palette file (0 - black)    */
     /* and for selected data levels a threshold label can be      */
     /* can be designated.                                         */
     
     /* The labels can be simply the text provided or the          */
     /* the calculated result of the SCALE OFFSET formula if       */
     /* '$scale_offset' is specified, or the data level itself if  */
     /* '$data_level' is specified.                                */
   
     /* reading to end of line rather than a single value improves */
     /* robustness and permits comments after data value           */
     
     /* Comments are allowed after a line item entry only in the  */
     /* the first portion of the legend file prior to the         */
     /* threshold level color assignment section.                 */
     /* Entire line can be commented out in all parts of the file */
     
     if(frame_location != PREFS_FRAME) { /*  normal display function */
        sd_decode_ptr->n_leg_colors = 0;
        sd_decode_ptr->n_l_flags = 0;
        sd_decode_ptr->n_t_flags = 0;
       
        sd_decode_ptr->max_level = 0;
        sd_decode_ptr->leg_Scale = 0;
        sd_decode_ptr->leg_Offset = 0;
     }

/* CVG 8.8 BUG FIX */
/*    Modified test in while loop and before sscanf to better */
/*    handle missing parameters                               */

     /* 1. Skip comments and read type of legend file */
     buf[0] = '#';

     while( buf[0] == '#' && feof(leg_file) == 0 ) {
        read_to_eol(leg_file, buf);
        line_read ++;
     }

     if(feof(leg_file) == 0 && buf[0] != '\0')
        sscanf(buf, "%d", &type_leg_file);  /* determines data type */
           /*  4 - signed integer type, 5 - unsigned integer type, 6 - real type */
     else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR in Generic Legend File\n"
                       "     unable to read type of legend file on line %d\n",
                                                                     line_read);
     }
 
 
     /* 2. Skip comments and read number of colors used */
     buf[0] = '#';

     while( buf[0] == '#' && feof(leg_file) == 0 ) {
        read_to_eol(leg_file, buf);
        line_read ++;
     }

     if(feof(leg_file) == 0 && buf[0] != '\0')
        sscanf(buf, "%d", &num_leg_colors);   /* used in legend display */
     else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR in Generic Legend File\n"
                       "     unable to read number of legend colors on line %d\n",
                                                                        line_read);
     }
     
     

     /* 3. Skip comments and read minimum data level used in the product */
     buf[0] = '#'; 

     while( buf[0] == '#' && feof(leg_file) == 0 ) {
        read_to_eol(leg_file, buf);
        line_read ++;
     }

     if(feof(leg_file) == 0 && buf[0] != '\0')
        switch (method) {
               case 4:
                  sscanf(buf, "%d", &min_lvl.si); /* legend display & decoding */
                  break;
               case 5:
                  sscanf(buf, "%u", &min_lvl.ui); /* legend display & decoding */
                  break;
               case 6:
                  sscanf(buf, "%lf", &min_lvl.dd); /* legend display & decoding */
                  break;
               default:
                  break;
            } /*  end switch */
     else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR in Generic Legend File\n"
                       "     unable to read maximum data level used on line %d\n",
                                                                       line_read);
     }
     
     
     /* CVG 8.7 replaces total number of levels with max level */
     /* 4. Skip comments and read maximum data level used in the product */
     buf[0] = '#'; 

     while( buf[0] == '#' && feof(leg_file) == 0 ) {
        read_to_eol(leg_file, buf);
        line_read ++;
     }

     if(feof(leg_file) == 0 && buf[0] != '\0') {
        sscanf(buf, "%u", &max_level);    /*  legend display & decoding */
        /* TO DO - USE THE FOLLOWING IN THE FUTURE */
            switch (method) {
               case 4:
                  sscanf(buf, "%d", &max_lvl.si); /* legend display & decoding */
                  break;
               case 5:
                  sscanf(buf, "%u", &max_lvl.ui); /* legend display & decoding */
                  break;
               case 6:
                  sscanf(buf, "%lf", &max_lvl.dd); /* legend display & decoding */
                  break;
               default:
                  break;
            } /*  end switch */
        
     } else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR in Generic Legend File\n"
                       "     unable to read maximum data level used on line %d\n",
                                                                       line_read);
     }
     

     
     /* 5. Skip comments and read number of leading flags */
     buf[0] = '#';

     while( buf[0] == '#' && feof(leg_file) == 0 ) {
        read_to_eol(leg_file, buf);
        line_read ++;
     }

     if(feof(leg_file) == 0 && buf[0] != '\0')
        sscanf(buf, "%d", &num_lead_flags);   /* legend display & decoding */
     else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR in Generic Legend File\n"
                       "     unable to read number of leading flags on line %d\n",
                                                                        line_read);
     }
     
     /* 6. Skip comments and read number of trailing flags */
     buf[0] = '#';

     while( buf[0] == '#' && feof(leg_file) == 0 ) {
        read_to_eol(leg_file, buf);
        line_read ++;
     }

     if(feof(leg_file) == 0 && buf[0] != '\0')
        sscanf(buf, "%d", &num_trail_flags);  /*  legend display & decoding */
     else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR in Generic Legend File\n"
                       "     unable to read number of trailing flags on line %d\n",
                                                                        line_read);
     }
     

     /* CVG 8.7 */
     /* 7. Skip comments and read num digits after decimal for display */
     buf[0] = '#';

     while( buf[0] == '#' && feof(leg_file) == 0 ) {
        read_to_eol(leg_file, buf);
        line_read ++;
     }
/*     if(feof(leg_file) == 0) */
     if(feof(leg_file) == 0 && buf[0] != '\0')
        sscanf(buf, "%d", &dec_places);   /* legend display & decoding */
     else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR in Generic Legend File\n"
                       "     unable to read numeric label format on line %d\n",
                                                                        line_read);
     }

     /* CVG 8.7 */
     /* 8. Skip comments and read source of parameters */
     buf[0] = '#';
/*     while( buf[0] == '#' || feof(leg_file) != 0 ) { */
     while( buf[0] == '#' && feof(leg_file) == 0 ) {
        read_to_eol(leg_file, buf);
        line_read ++;
     }
/*     if(feof(leg_file) == 0) */
     if(feof(leg_file) == 0 && buf[0] != '\0')
        sscanf(buf, "%s", source_of_params);   /* legend display & decoding */
     else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR in Generic Legend File\n"
                       "     unable to read source of parameters on line %d\n",
                                                                     line_read);
     }
     
     

     /* 9. Skip comments and read Scale parameter for decoding */
          buf[0] = '#'; 

     while( buf[0] == '#' && feof(leg_file) == 0 ) {
        read_to_eol(leg_file, buf);
        line_read ++;
     }

     if(feof(leg_file) == 0 && buf[0] != '\0')
        sscanf(buf, "%f", &leg_Scale);      /*  legend display & decoding */
     else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR in Generic Legend File\n"
                       "     unable to read SCALE value on line %d\n",line_read);
     }
     

     /* 10. Skip comments and read Offset parameter for decoding */
     buf[0] = '#'; 

     while( buf[0] == '#' && feof(leg_file) == 0 ) {
        read_to_eol(leg_file, buf);
        line_read ++;
     }

     if(feof(leg_file) == 0 && buf[0] != '\0')
        sscanf(buf, "%f", &leg_Offset);     /*  legend display & decodingy */
     else {
        gen_error = TRUE;
        fprintf(stderr,"\ERROR in Generic Legend File\n"
                       "     unable to read OFFSET value on line %d\n",line_read);
     }
       
/* DEBUG */
/*fprintf(stderr,                                                       */
/*         "DEBUG - type legend file is %d, num legend colors is %d\n"  */
/*         "        source of decoding parameters is '%s'          \n"  */
/*         "        numerical label display format is '%s'         \n"  */
/*         "        num lead flags is %d,   num trail flags is %d  \n"  */
/*         "        maximum data level used is %u                  \n"  */
/*         "        SCALE parameter is %f,  OFFSET parameter is %f \n", */
/*   type_leg_file, num_leg_colors, source_of_params, num_disp_format,  */
/*   num_lead_flags, num_trail_flags, max_level, leg_Scale, leg_Offset);*/


      if(gen_error == TRUE) {
         return (LEGEND_ERROR);
      }



      /* --------------- ERROR CHECK SECTION --------------- */
      
      dig_error = FALSE;
      gen_error = FALSE;
      
      /* IMPORTANT NOTE:                                                */
      /*     The legend method specified in the product preferences     */
      /*     must agree with the type data structure used.              */
      if(frame_location != PREFS_FRAME) { /*  normal display function  */
        
         if(sd->last_image == GENERIC_RADIAL) {
            if(method==4) {
               if( (sd->gen_rad->type_data!=DATA_SHORT) &&
                   (sd->gen_rad->type_data!=DATA_INT)   &&
                   (sd->gen_rad->type_data!=DATA_BYTE) )
                  gen_error = TRUE; 
            } else if(method==5) {
               if( (sd->gen_rad->type_data!=DATA_USHORT) &&
                   (sd->gen_rad->type_data!=DATA_UINT)   &&
                   (sd->gen_rad->type_data!=DATA_UBYTE) )
                  gen_error = TRUE;
            } else if(method==6) {
               if( (sd->gen_rad->type_data!=DATA_FLOAT) &&
                   (sd->gen_rad->type_data!=DATA_DOUBLE) )
                  gen_error = TRUE;
            }
            
         /* FUTURE ENHANCEMENT: add support for PRECIP_ARRAY_IMAGE */   
         } else if(sd->last_image == DIGITAL_IMAGE) {
              if( (method==4) || (method==6) ) {
                 dig_error = TRUE;
              }
         }
         
         if(gen_error == TRUE) 
            fprintf(stderr, 
                "\n* ERROR ********************************************\n"
                "        Incorrect Generic Legend Method (%d) used   \n"
                "        for the data type present in the product: %d\n",
                                    method, sd->gen_rad->type_data);
            
         if(dig_error == TRUE) 
            fprintf(stderr, 
                "\n* ERROR ********************************************\n"
                "        Incorrect Digital Legend Method (%d) used   \n"
                "        for the data type present in the product.   \n",
                                                             method);
        
      } /*  end if not at prefs frame */
      
      
      /* Item 1 Tested */
      if(type_leg_file != method) {
         gen_error = TRUE;
        
         fprintf(stderr,"\n***** GENERIC PRODUCT CONFIGURATION ERROR ********\n");
         fprintf(stderr,"      The type of legend file field (%d) does     \n"
                        "      not match the method selected (%d) in the   \n"
                        "      in the product configuration dialog.        \n"
                        "**************************************************\n",
                                                type_leg_file, method);
      } 
      
      /* Item 2 Tested */
      if(num_leg_colors > MAX_N_COLOR) {
         gen_error = TRUE;
         
         fprintf(stderr,"\n***** GENERIC PRODUCT CONFIGURATION ERROR ********\n");
         fprintf(stderr,"      The number of colors (%d) exceeds the       \n"
                        "      maximum number of colors permitted (%d).    \n"
                        "**************************************************\n",
                                                 num_leg_colors, MAX_N_COLOR);
      }
      

     /* Item 3 Tested */
     /* The minimum data level not used for unsigned integers (Method 5) */
     /* TO DO Add support for signed integers(Method 4) and Real types(Method 6) */
     

      /* Item 4 Tested */
     /* CVG 8.7 - still used in validity checks */
     /* THE FOLLOWING TEST ONLY APPLIES TO UNSIGNED INTEGER TYPES (Method 5) */
     /* TO DO Add support for signed integers(Method 4) and Real types(Method 6) */
     tot_num_lvls = max_level + 1;
     
     /* CVG 8.8 Test the max level in the product description block in this case */
     if( (frame_location != PREFS_FRAME) && (param_source == PROD_PARAM) ) {
        tot_num_lvls = sd_decode_ptr->prod_max_level + 1;
     }

      /* IMPORTANT NOTE:                                                */
      /*     The maximum number of data levels that should be used with */
      /*     a generic legend flag depends upon the data type.          */
      /*     If out of range we abort display                           */
      /******************************************************************/
      
      /* FUTURE TO DO - ALL REFERENCES TO MAXIMUM NUMBER OF LEVELS SHOULD BE */
      /*                CHANGED TO MAXIMUM DATA LEVEL IN ORDER TO SUPPORT    */
      /*                UNSIGNED LONG DATA TYPES                             */
      max_num_levels = 256;  /*  works for DIGITAL_IMAGE and the byte/ubyte */
                             /*            GENERIC_RADIAL  */
      if(frame_location != PREFS_FRAME) { /* normal display function */
    
         if(sd->last_image == GENERIC_RADIAL) {
             if( (sd->gen_rad->type_data==DATA_SHORT) ||
                 (sd->gen_rad->type_data==DATA_USHORT) ) 
                 max_num_levels = 65536;  /*  2^16  */
             else if( (sd->gen_rad->type_data==DATA_INT) ||
                 (sd->gen_rad->type_data==DATA_UINT) ) 
/* CVG 8.8 */
                /* the following is (unsigned long) 4294967295; */ /*  2^32 - 1 */
                max_num_levels = ULONG_MAX; /* WARNING - MAX VALUE NOT NUM LEVELS */
             else if( (sd->gen_rad->type_data==DATA_FLOAT) ||
                 (sd->gen_rad->type_data==DATA_DOUBLE) )
                max_num_levels = 0;  /*  an artificial flag value for real types */
         }
         
         if ( (method==4) || (method==5) ) {
             if ( tot_num_lvls < 0 ||  tot_num_lvls > max_num_levels) {
                gen_error = TRUE;
                /* CVG 8.8 tested PDB max level */
                if((strcmp(source_of_params, "$product")==0)) 
                    fprintf(stderr, 
                         "\n* ERROR ********************************************\n"
                         "*       The max data level %lu specified in the product\n"
                         "*       description block is out of range (1-%lu) \n",
                                                tot_num_lvls-1, max_num_levels-1);
                else 
                   fprintf(stderr, 
                         "\n* ERROR ********************************************\n"
                         "*       The max data level %lu configured in the   \n"
                         "*       the legend file is out of range (1-%lu) \n",
                                                tot_num_lvls-1, max_num_levels-1);
                 
                fprintf(stderr,
                         "*        Aborting legend display                    \n" 
                         "****************************************************\n");
               
             }   /*  end test for too many integer data levels specified */
         
         } else if (method==6)  {
             ; /* total number of data levels not yet specified/used */
         }
         
      } else { /* in prefs frame */
         
         if ( (method==4) || (method==5) ) {
             fprintf(stderr,"\nNOTE: The maximum data level (%lu) should not exceed\n"
                            "       255 for digital products using packet 16 or \n"
                            "           generic products type BYTE / UBYTE.     \n"
                            "     65535 for generic products type SHORT / USHORT\n"
                            "4294967294 for generic products type INT / UINT   \n",
                                                                    tot_num_lvls-1);
             
         } else if (method==6)  {
             fprintf(stderr,"NOTE: Maximum data level is not yet used for \n"
                            "      generic method 6 "); 
         }
         
      } /* end in prefs frame */
      
      
      /* Items 5 % 6 Tested */
      if(num_lead_flags > 2)
         fprintf(stderr,"WARNING in Generic Legend File\n"
                       "      number of leading flags (%d) appears large.\n",
                                                               num_lead_flags);
      if(num_trail_flags > 2)
         fprintf(stderr,"WARNING in Generic Legend File\n"
                       "      number of trailing flags (%d) appears large.\n",
                                                               num_trail_flags);
      
      /* Items 4, 5, & 6 Compared */
      if(num_lead_flags + num_trail_flags > max_level) {
         gen_error = TRUE;
         fprintf(stderr,"\nERROR in Generic Legend File\n"
                        "     The number of leading flags %d, and trailing \n"
                        "     flags %d, is too large for the maximum data  \n"
                        "     level specified %d \n", 
                                     num_lead_flags, num_trail_flags, max_level);
      }



      /* Item 7 Tested */
      if( (dec_places < 0) || (dec_places > 6) ) {
         gen_error = TRUE;
         fprintf(stderr,"\nERROR in Generic Legend File\n"
                       "     invalid entry for number of digits after decimal point.\n"
                       "     Must bebetween 0 and 6.\n");
      }
     


      /* Item 8 Tested */
      /* we also set an integer variable to represent source of data */
      if((strcmp(source_of_params, "$product")==0)) {
         param_source = PROD_PARAM;
      } else if((strcmp(source_of_params, "$file")==0)) {
         param_source = FILE_PARAM;
      } else if((strcmp(source_of_params, "$no_decode")==0)) {
         param_source = NO_DECODE;
      } else {
         param_source = NO_DECODE;
         gen_error = TRUE;
         fprintf(stderr,"\nERROR in Generic Legend File\n"
                        "     invalid entry for source of decoding parameters.\n");
      }

      /* TO DO - THE FOLLOWING TESTS ARE ALLWAYS USING THE FILE VERSION OF THE */
      /*         PARAMETERS EVEN WHEN NOT IN PREFS_FRAME AND PROD_PARAM        */
      /* NEED TO CHANGE RIGHT AWAY!!!!!!!                                      */
      
      if(frame_location == PREFS_FRAME) {
         
         if( (param_source != NO_DECODE) && (method == 5) ) {
           
            /* Items 9 Tested - only used with Method 5 */
            /* protect against division by 0 */
            if(leg_Scale == 0.0) {
               fprintf(stderr,"\nERROR in Generic Legend File\n"
                              "     Scale value 0.0 is not a valid value\n");
              gen_error = TRUE;
            }
            /* scale must be positive value for increasing decoded value */
            if(leg_Scale < 0.0) {
               fprintf(stderr,"\nERROR in Generic Legend File\n"
                              "     Scale value must be positive because by \n"
                              "     convention the decoded value must increase\n"
                              "     with the raw data level\n");
              gen_error = TRUE;
            }
            
            /* TO DO - IN ADDITION TO PRINTING OUT THE FIRST AND LAST DECODED    */
            /*         VALUES ADD A COMPARISON OF SCALE, OFFSET, AND NUM DIGITS  */
            /*         AND PRINT WARNING IF IT LOOKS LIKE LIMITS ARE EXCEEDED    */
   
            /* viewing the minimum decoded value and maximum decoded value       */
            /* determines if the limits of the legend display have been exceeded.*/
            /*      delecting a different unit of measure and recalculating the  */
            /*      Scale and Offset parameters (and number of digits)           */
            
            /* limit based on number of characters allowed in legend label */
            sprintf(num_disp_format,"%c%d.%df", 37, 8, dec_places);
            
            product_min_val = ( ((double)num_lead_flags) - ((double)leg_Offset) ) 
                                                / ((double)leg_Scale);
            sprintf(prod_min_str, num_disp_format, product_min_val);
            
            /* TO DO - USE max_lvl.si, max_lvl.ui, or max_lvl.dd IN THE FUTURE */
            product_max_val = 
                 ( ((double)(max_level-num_trail_flags)) - ((double)leg_Offset) ) 
                                                           / ((double)leg_Scale);
            sprintf(prod_max_str, num_disp_format, product_max_val);
   
            fprintf(stderr,"\nUsing Parameters in the Legend File the minimum\n"
                           "and maximum numerical values would be displayed as:\n"
                           "     product min decode value %f  display '%s'\n"
                           "     product max decode value %f  display '%s'\n",
                   product_min_val, prod_min_str, product_max_val, prod_max_str);
   
         }  /* end if not  NO_DECODE and method 5 test & compare Items 9 and 10 */
         
      } else if( (frame_location != PREFS_FRAME) ) {
      
         if( (param_source != NO_DECODE) && (method == 5) ) {
                   
            if(param_source == FILE_PARAM) {
               /* protect against division by 0 */
               if(leg_Scale == 0.0) {
                  fprintf(stderr,"\nERROR in Generic Legend File\n"
                                 "     Scale value 0.0 is not a valid value\n");
                 gen_error = TRUE;
               }
               /* scale must be positive value for increasing decoded value */
               if(leg_Scale < 0.0) {
                  fprintf(stderr,"\nERROR in Generic Legend File\n"
                                 "     Scale value must be positive because by \n"
                                 "     convention the decoded value must increase\n"
                                 "     with the raw data level\n");
                 gen_error = TRUE;
               }
                
            } else if(param_source == PROD_PARAM) {
               /* protect against division by 0 */
               if(sd_decode_ptr->prod_Scale == 0.0) {
                  fprintf(stderr,"\nERROR in Product Description Block\n"
                                 "     Scale value 0.0 is not a valid value\n");
                 gen_error = TRUE;
               }
               /* scale must be positive value for increasing decoded value */
               if(sd_decode_ptr->prod_Scale < 0.0) {
                  fprintf(stderr,"\nERROR in Product Description Block\n"
                                 "     Scale value must be positive because by \n"
                                 "     convention the decoded value must increase\n"
                                 "     with the raw data level\n");
                 gen_error = TRUE;
               }
            
            } /* end PROD_PARAM */
            
         }  /* end if not  NO_DECODE and method 5 test & compare Items 9 and 10 */
      
      } /* end test product parameters */

     if( (gen_error==TRUE) || (dig_error==TRUE) )
        return (LEGEND_ERROR);



     
     /* ///////////////////////////////////////////////////////////////////// */
      
      
      /*  If displaying an image, then fill the screen data */
      if(frame_location != PREFS_FRAME) { /*  normal display function */

         if( (sd->last_image==GENERIC_RADIAL) ||
             (sd->last_image==DIGITAL_IMAGE) ) {
      
            sd_decode_ptr->n_leg_colors = num_leg_colors;
            sd_decode_ptr->n_l_flags = num_lead_flags;
            sd_decode_ptr->n_t_flags = num_trail_flags;
            /* CVG 8.7 */
            sd_decode_ptr->decode_flag = param_source;
            sd_decode_ptr->max_level = max_level;
            sd_decode_ptr->leg_Scale = leg_Scale;
            sd_decode_ptr->leg_Offset = leg_Offset;
            
         } else { /*  used with incorrect product type */
         
            fprintf(stderr,"\nERROR - Generic Legend Used with incorrect packet\n");
         
         } 
      
      } /* end not PREFS_FRAME */
      
 
     
     /* 1 -- fill data arrays for  data level, colors, and labels -- */
     
     /*  initialize arrays */
     /****************************************************************/
     for(i=0; i<=MAX_N_COLOR; i++) {
        /*  the last data level is not a threshold, it is the largest */
        /*  expected data level - TO BE REMOVED */
        /*  FUTURE TO DO - ADD SUPPORT FOR OTHER TYPES */
        ui_dl[i] = 0;
        si_dl[i] = 0;
        d_dl[i] = 0.0;

        bar_color[i] = 0;
        leg_label[i][0] = '\0';
     } /* end for MAX_N_COLOR */
     /****************************************************************/

     if(frame_location != PREFS_FRAME) { 
        for(i=0; i<=MAX_N_COLOR; i++) {
           sd_color_ptr->data.ui_dl[i]  = 0;
           sd_color_ptr->data.si_dl[i]  = 0;
           sd_color_ptr->data.d_dl[i]  = 0.0;
           
           sd_color_ptr->dl_clr[i] = 0;
           sd_color_ptr->dl_label[i][0]  = '\0';
        }
     } 

     
     /* read remaining lines */
     no_decode_error = FALSE;
     count = 0;
     /*---------------------------------------------------------------------------*/
     while(feof(leg_file) == 0) {  /* continue reading until we get to eof */
     
        /* read in one whole line */
        read_to_eol(leg_file, buf);  
        line_read ++;

        /*  ignore comment line */
        if(buf[0] == '#')
           continue;
           
        /* parse all but final string */  
        /*  FUTURE TO DO - TEST METHODS 4 AND 6 */
        /* CVG 9.1 - added test for increasing data levels in the legend file */
        switch (method) {
           case 4:
              num_read = sscanf(buf, "%d%d%n", &si_level_read, &color_read, 
                                                                    &position);
              if( (si_level_read<=last_si_level_read) && (count>0) && 
                 (count<(num_leg_colors-1)) ) {
                  fprintf(stderr,"ERROR in generic legend file, data levels not\n"
                                 "      increasing at data line %d\n", count+1);
              }
              last_si_level_read = si_level_read;
              break;
           case 5:
              num_read = sscanf(buf, "%u%d%n", &ui_level_read, &color_read, 
                                                                    &position);
              if( (ui_level_read<last_ui_level_read) && (count>0) && 
                 (count<(num_leg_colors-1)) ) {
                  fprintf(stderr,"ERROR in generic legend file, data levels not\n"
                                 "      increasing at data line %d\n", count+1);
              }
              last_ui_level_read = ui_level_read;
              break;
           case 6:
              num_read = sscanf(buf, "%lf%d%n", &d_level_read, &color_read, 
                                                                    &position);
              if( (d_level_read<last_d_level_read) && (count>0) && 
                 (count<(num_leg_colors-1)) ) {
                  fprintf(stderr,"ERROR in generic legend file, data levels not\n"
                                 "      increasing at data line %d\n", count+1);
              }
              last_d_level_read = d_level_read;
              break;
           default:
              break;
        } /*  end switch */

      

        if( (num_read == EOF) && 
            (count < (num_leg_colors-1)) ) { 
            fprintf(stderr,"\nERROR in Generic Legend File %s\n"
                       "    Encountered End of File while parsing color \n"
                       "assignments and legend text for designated data levels \n"
                       "for legend color entry number %d\n",
                                                   generic_legend_filename, count);
            
             break;
        }
             
        if( (num_read < 2) && 
            (count < (num_leg_colors-1)) ) { 
            /* we do NOT break out of loop */
            fprintf(stderr,"\nERROR in Generic Legend File %s\n"
                       "    This error occurred, while parsing the color \n"
                       "assignments and legend text for designated data levels \n"
                       "for legend color entry number %d\n",
                                                   generic_legend_filename, count);
        
        /* CVG 8.7 no longer read the max value after the last color assignment */
        } else if( (count > (num_leg_colors-1)) && 
                   (num_read >= 1)  ) { /* in case we have a non-comment*/
                                                 /* line at the end of the file  */
                                                 /* exit loop - DONE             */

            fprintf(stderr,"\nNOTE: there were more non-comment lines at the end \n"
                           "      of the legend file after processing the last \n"
                           "      data line according to the number of legend  \n"
                           "      colors specified in the legend file.\n");
            
            /*  exit loop */
            break;
            
        
        } else { /*  a good legend file entry */
            
            /*  FUTURE TO DO - TEST METHODS 4 AND 6 */
            switch (method) {
               case 4:
                  si_dl[count] = si_level_read;     /*  default is 0 */
                  break;
               case 5:
                  ui_dl[count] = ui_level_read;     /*  default is 0 */
                  break;
               case 6:
                  d_dl[count] = d_level_read;     /*  default is 0.0 */
                  break;
               default:
                  break;
            } /*  end switch */

                
            bar_color[count] = color_read; /*  default is 0 */
                
            /* skip initial white space */
            j=0;
            while(buf[position+j] == ' ') 
                j++;
            
            strcpy(l_str, &buf[position+j]);      /*  default is '\0' */
            

            /* Determine whether SCALE OFFSET decoding, data level pass through, */
            /* or explicit string is used for indicating legend threshold label  */
            
            /* first, set field width based upon the precision (dec_places) */
            /* this is only a minor cosmetic issue */
            if(dec_places == 0)        field_width = 7;
            else if(dec_places == 6)   field_width = 9;
            else                       field_width = 8;  /* the default */
            
            if( (strcmp(l_str, "$scale_offset")==0) ) {
                
                switch(method) {
                   case 4:
                      fprintf(stderr,"\nERROR in Generic Legend Configuration - \n"
                                     "   Scale Offset Decoding only compatible\n"
                                     "   Unsigned Integer - Method 5 \n");
                      break;
                   case 5:
                      sprintf(num_disp_format,"%c%d.%df", 
                              37, field_width, dec_places);
                      /* cvg 8.8 */
                      if(param_source == NO_DECODE && no_decode_error == FALSE) {
                         fprintf(stderr,
                                 "\n* ERROR in Generic Legend Configuration -   \n"
                                 "*   Used '$scale_offset' to decode numerical\n"
                                 "*   legend label with parameter source set  \n"
                                 "*   to '$no_decode'.  All labels set to '0'\n");
                         no_decode_error = TRUE;
                      }
                      real_value = decode_data_level(ui_level_read, param_source,
                                                                   frame_location);
                      sprintf(l_str, num_disp_format, real_value);
                      break;
                   case 6:
                      fprintf(stderr,"\nERROR in Generic Legend Configuration - \n"
                                     "   Scale Offset Decoding only compatible\n"
                                     "   Unsigned Integer - Method 5 \n");
                      break;
                   default:
                      break;
                } /* end switch */
                
                if(verbose_flag)
                   fprintf(stderr,"Legend Label for color %d using SCALE OFFSET\n",
                           count+1);
                
            } else if( (strcmp(l_str, "$data_level")==0) ) {
            
                switch(method) {
                   case 4:
                      sprintf(num_disp_format,"%c%d.%dd", 
                              37, field_width, dec_places);
                      sprintf(l_str, num_disp_format, si_level_read);
                      break;
                   case 5:
                      sprintf(num_disp_format,"%c%d.%du", 
                              37, field_width, dec_places);
                      sprintf(l_str, num_disp_format, ui_level_read);
                      break;
                   case 6:
                      sprintf(num_disp_format,"%c%d.%dlf", 
                              37, field_width, dec_places);
                      sprintf(l_str, num_disp_format, d_level_read);
                      break;
                   default:
                      break;
                } /* end switch */
                if(verbose_flag)
                   fprintf(stderr,"Legend Label for color %d is actual Data Level\n",
                           count+1);
                
            } else { /* using the text string */
             
                /* We do nothing with empty strings or strings consisting entirely of ' ' */
                if( (strcspn(l_str, " ")) != 0  ) {
                      if(verbose_flag)
                         fprintf(stderr,"Legend Label for color %d uses string in legend file\n",
                                 count+1);
                /* only text interpreted as numerical values ar right justified */
                      /* find the first numerical token */
                   if( (strspn(l_str, "0123456789.-")) !=0 ) {
                       /* get the first token, it should be numerical */
                       strcpy(l_str_cpy, l_str); /* DO NOT ALTER ORIGINAL l_str */
                       tok1 = strtok(l_str_cpy, " ");

/* DEBUG */
/*fprintf(stderr,"DEBUG - l_str is '%s', l_str_cpy is '%s'\n", l_str, l_str_cpy);*/
/* DEBUG */
/*fprintf(stderr,"DEBUG tok1 is '%s'\n", tok1);       */

                       next_tok = strtok(NULL, " ");
                       /* if numerical, the first token should be the only token */
                       /* for the following conversion to right justified        */
                       if( (next_tok == NULL) ) {
                          switch(method) {
                             case 4:
                                sprintf(num_disp_format,"%c%d.%df", 
                                        37, field_width, dec_places);
                                real_value = atof(tok1);
                                sprintf(l_str, num_disp_format, real_value);
                                break;
                             case 5:
                                sprintf(num_disp_format,"%c%d.%df", 
                                        37, field_width, dec_places);
                                real_value = atof(tok1);
                                sprintf(l_str, num_disp_format, real_value);  

/* DEBUG */
/*fprintf(stderr,"DEBUG real_value is %f\n", real_value);*/
/* DEBUG */
/*fprintf(stderr,"DEBUG l_str is '%s'\n", l_str);*/

                                break;
                             case 6:
                                sprintf(num_disp_format,"%c%d.%dlf", 
                                        37, field_width, dec_places);
                                real_value = atof(tok1);
                                sprintf(l_str, num_disp_format, real_value);
                                break;
                             default:
                                break;
                          } /* end switch */
                     } /* end if next_tok is NULL */
                       
                   } /* end if there is a numerical token */
                     
                } /* end if NOT empty or blank string */
                
            } /* end else using the text string */

            /* CVG 8.7 - increased from 8 to 9 */
            if(strlen(l_str)>9) { /*  truncate to string of 9 characters max */
                l_str[9] = '\0';
                fprintf(stderr,"NOTE: Threshold label truncated to 9 characters\n");
            }

            strcpy(leg_label[count], l_str);


        } /*  end if not EOF and num_read >=2 */
        
          count++;
          l_str[0] = '\0'; /*  reset label string after processing entry */
          
     } /*  end while not end of file */
     /*---------------------------------------------------------------------------*/

     if(num_trail_flags == 0) {
     /* set the maximum data level after the last numerical color assignment. */
        switch (method) {
           case 4:
              si_dl[num_leg_colors] = max_lvl.si;    /*  default is 0 */
              break;
           case 5:
              ui_dl[num_leg_colors] = max_level;     /*  default is 0 */
              break;
           case 6:
              d_dl[num_leg_colors] = max_lvl.dd;     /*  default is 0.0 */
              break;
           default:
              break;
        } /*  end switch */
     } /*  end if num_trail_flags is 0   */
            
/*  TEST */
/* fprintf(stderr,"DEBUG - index %d:: data level %d, is maximum expected value\n",*/ 
/*                                   num_leg_colors, max_level);                  */
/***********************************************************************/


      /*  If displaying an image, then fill the screen data */
      if(frame_location != PREFS_FRAME) { /*  normal display function */

         if( (sd->last_image==GENERIC_RADIAL) ||
             (sd->last_image==DIGITAL_IMAGE) ) {
         
            for(i=0; i<=num_leg_colors; i++) { 
            /*  the last data level is not a threshold, it is the largest */
            /*  expected data level */

               /*  FUTURE TO DO - TEST METHODS 4 AND 6 */
               switch (method) {
                  case 4:   /* only generic radial */
                     sd->gen_rad->color.data.si_dl[i] = si_dl[i];
                     break;
                  case 5:   /* both generic radial and digital image */
                     sd_color_ptr->data.ui_dl[i] = ui_dl[i];
                     break;
                  case 6:   /* only generic radial */
                     sd->gen_rad->color.data.d_dl[i] = d_dl[i];
                     break;
                  default:
                     break;
               } /*  end switch */
   
               sd_color_ptr->dl_clr[i] = bar_color[i];

               strcpy(sd_color_ptr->dl_label[i], leg_label[i]);

        
            } /*  end for */


         } else  {  
       
            fprintf(stderr,"\nERROR - Generic Legend Used with incorrect packet\n");
            
         } /*  not a DIGITAL IMAGE or GENERIC RADIAL */


      } /*  end if not PREFS_FRAME    */
    
    
    
     return(GOOD_LEGEND);
    
} /*  END read_generic_legend */







/*  HELPER FUNCTION     Methods 4, 5, & 6                                   */
/****************************************************************************/
/* CVG 8.9 - added prod_id param */
void display_generic_legend(Drawable canvas, int n_val_hgt, int x_in, int y_in,  
                            int method, int prod_id, int frame_location)
{
   /* ----- Input Parameters ---------------
    * n_val_hgt      the calculated height in pixels that can be used for
    *                    the numerical data levels (does not include flags)
    *
    * x_in           the x pixel postion of the left side of the legend 
    *                    (starting point of the labels
    * y_in           the y pixel postion used for writing the unit of measure
    *                    description which is just above the color bars
    *
    * frame_location whether the legend is being drawn as part of the product
    *                    display or on the legend preview pane
    * --------------------------------------
    */

   /* ----- File Globals used when in PREFS_FRAME -----------
    * uses the following variables set by read_generic_legend
    *
    * num_lead_flags
    * num_trail_flags
    * 
    * type_leg_file
    * num_leg_colors
    *
    * ----Screen Data used when in PROD_FRAME --------------
    */
    
     
   /* --------------------------------------------------------- */
   /* The unit of measure is written with y = legend_start      */
   /*                                                           */
   /* The first threshold label is with  y = legend_start +     */
   /*                                         2*font_height + 5 */
   /*                                                           */
   /* The first color bar is drawn with y = legend_start +      */
   /*                                         font_height + 8   */
   /* --------------------------------------------------------- */
     int i;

     int num_flags;

     float pix_per_val = 0.0;
     
     /* these are used to calculate the height of the numerical 
      * (non_flag) bar which is pix_per_val */
     unsigned int first_ui=0;  /*  first non-flag numerical data value */
     unsigned int last_ui=0;   /*  with no trailing flags this is the   */
                               /*      last numerical data value        */
                               /*  with trailing flags this is the data */
                               /*      level of the first trailing flag */
     /*  FUTURE TO DO - TEST SUPPORT FOR OTHER TYPES (Methods 4 and 6) */
     int first_si = 0;
     int last_si = 0;
     double first_d = 0.0;
     double last_d = 0.0;
     
     int first_num_color;      /*  first non-flag color number */
     int last_num_color;       /*  last non-flag color number */

     int lx, ly, cx, cy;       /*  x and x pixel locations */
     int bh, bw;               /*  height/width of color bar rectangle  */
     
     int start_y[MAX_N_COLOR+1];      /*  where to draw this threshold */
     int delta_y;
     
     int retval;

     /* local variables */
     int           n_leg_colors;
     unsigned int  max_lvl;
     int           n_l_flags;  
     int           n_t_flags; 
     int          *sd_lvl;
     unsigned int *ud_lvl;
     double       *dd_lvl;
     label_string *l_label;
     int          *b_color;

    decode_params_t *sd_decode_ptr=NULL;
    color_assign_t  *sd_color_ptr=NULL;




    if(frame_location != PREFS_FRAME) {
       if(sd->last_image==DIGITAL_IMAGE) {
          sd_decode_ptr = &sd->dhr->decode;
          sd_color_ptr = &sd->dhr->color;
       } else if(sd->last_image==GENERIC_RADIAL) {
          sd_decode_ptr = &sd->gen_rad->decode;
          sd_color_ptr = &sd->gen_rad->color;
       }  
    } /* end not PREFS_FRAME */
    
     
     /* This function reads the legend labels and color numbers    */
     /* associated with a data level specified in the legend file. */
     
     /* The data type for the data value depends upon the type of  */
     /* generic product loaded. This function could be modified    */
     /* to work with packet 16 as well.                            */
     
     /* Each specified data level is a threshold value for the     */
     /* designated color number in the palette file (0 - black)    */
     /* and for selected data levels a threshold label can be      */
     /* can be designated.                                         */
     
     
      fprintf(stderr,"\nDisplay Generic Legend\n"); 
      
      
     /* use screen data IN PROD_FRAME, file globals in PREF_FRAME */
     if(frame_location != PREFS_FRAME) {
        n_leg_colors    = sd_decode_ptr->n_leg_colors;
        
        if(sd_decode_ptr->decode_flag == PROD_PARAM) {
           max_lvl   = sd_decode_ptr->prod_max_level;
           n_l_flags = sd_decode_ptr->prod_n_l_flags;
           n_t_flags = sd_decode_ptr->prod_n_t_flags;
        } else {
           max_lvl   = sd_decode_ptr->max_level;
           n_l_flags = sd_decode_ptr->n_l_flags;
           n_t_flags = sd_decode_ptr->n_t_flags;
        }
        
        sd_lvl     = sd_color_ptr->data.si_dl;
        ud_lvl     = sd_color_ptr->data.ui_dl;
        dd_lvl     = sd_color_ptr->data.d_dl;
        
        l_label      = sd_color_ptr->dl_label;
        b_color      = sd_color_ptr->dl_clr;
        
     } else { /* use the file globals set by read generic legend */
        n_leg_colors = num_leg_colors;
        max_lvl      = max_level;
        n_l_flags    = num_lead_flags;
        n_t_flags    = num_trail_flags;
        
        sd_lvl     = si_dl;
        ud_lvl     = ui_dl;
        dd_lvl     = d_dl;
        
        l_label      = leg_label;
        b_color      = bar_color;
       
     }
      
      /* CVG 8.8 */
    if(frame_location != PREFS_FRAME) {
        
       if(sd_decode_ptr->decode_flag == PROD_PARAM) {
          fprintf(stderr,"\nProduct configured to decode using parameters in product\n");
          fprintf(stderr,"    Scale: %f   Offset: %f \n"
                         "    max level: %u    lead flags: %d    trail flags: %d\n",
                      sd_decode_ptr->prod_Scale, sd_decode_ptr->prod_Offset,
                      sd_decode_ptr->prod_max_level, 
                      sd_decode_ptr->prod_n_l_flags, 
                      sd_decode_ptr->prod_n_t_flags);
          
       }
       
       /* CVG 8.8 */
       if(sd_decode_ptr->decode_flag == FILE_PARAM) {
          fprintf(stderr,"\nProduct configured to decode using parameters in legend file\n");
          fprintf(stderr,"    Scale: %f   Offset: %f \n"
                         "    max level: %u    lead flags: %d    trail flags: %d\n",
                      sd_decode_ptr->leg_Scale, sd_decode_ptr->leg_Offset,
                      sd_decode_ptr->max_level, 
                      sd_decode_ptr->n_l_flags, 
                      sd_decode_ptr->n_t_flags);
       }
       /* CVG 8.8 */
       if(sd_decode_ptr->decode_flag == NO_DECODE) {
          fprintf(stderr,"No decoding via Scale Offset\n");
       }
       
    } else { /* we are in PREFS_FRAME */
       /* CVG 8.8 */
       if(param_source == FILE_PARAM || param_source == PROD_PARAM) {
          fprintf(stderr,"\nDecoding via Scale Offset parameters in Legend File\n");
          fprintf(stderr,"    Scale: %f   Offset: %f \n"
                         "    max level: %u    lead flags: %d    trail flags: %d\n",
                      leg_Scale, leg_Offset,
                      max_level, 
                      num_lead_flags, 
                      num_trail_flags);
       }
       /* CVG 8.8 */
       if(param_source == NO_DECODE) {
          fprintf(stderr,"\nNo Decoding via Scale Offset - parameters in Legend File\n");
          fprintf(stderr,"Decoding via Scale Offset parameters in Legend File\n");
          fprintf(stderr,"    Scale: %f   Offset: %f \n"
                         "    max level: %u    lead flags: %d    trail flags: %d\n",
                      leg_Scale, leg_Offset,
                      max_level, 
                      num_lead_flags, 
                      num_trail_flags);
       }
    } /* end in PREFS_FRAME */
     
     lx = x_in;          /*  x value for writing threshold labels */
     cx = x_in + 54;     /*  x value for drawing color bars */
     
     ly = 2*font_height + 5; /*  offset for start_y[n] for threshold labels */
     cy = font_height + 8;   /*  offset for start_y[n] for color bars */
     
     /* bar width allways 50 since the legend bar area is now */
     /* copied into the legend frame rather than displayed    */
        bw = 50;

      
     /* 2.-- pre-calculate the y cursor positions for all color assignments -- */

     for(i=0; i<n_leg_colors; i++)
        start_y[i] = 0;
        
     /* calculate the first & last index for numerical (non-flag) data */
     num_flags = n_l_flags + n_t_flags;
     first_num_color = 0 + n_l_flags; 
     
     /*  FUTURE TO DO - TEST METHODS 4 AND 6 */
     switch (method) {
        case 4:
           first_si = sd_lvl[first_num_color];
           break;
        case 5:
           first_ui = ud_lvl[first_num_color];
           break;
        case 6:
           first_d = dd_lvl[first_num_color];
           break;
        default:
           break;
     } /*  end switch */


     last_num_color = n_leg_colors-1-n_t_flags;
     
         
     /*  FUTURE TO DO - MODIFY FOR METHODS 4 AND 6  */
     switch (method) {
        case 4:
           if(n_t_flags == 0)
              last_si = sd_lvl[n_leg_colors];
           else 
              last_si = sd_lvl[last_num_color+1];
           break;
        case 5:
           if(n_t_flags == 0)
              last_ui = ud_lvl[n_leg_colors];
              /* last_ui = max_lvl; */
           else 
              last_ui = ud_lvl[last_num_color+1];
              /* last_ui = ud_lvl = max_lvl - n_t_flags; */
           break;
        case 6:
           if(n_t_flags == 0)
              last_d = dd_lvl[n_leg_colors];
           else 
              last_d = dd_lvl[last_num_color+1];
           break;
        default:
           break;
     } /*  end switch */

/* TEST */
/* fprintf(stderr,                                                              */
/*         "TEST - num_flags is %d, n_l_flags is %d, n_t_flags is %d\n",        */
/*                                       num_flags, n_l_flags, n_t_flags);      */
/* fprintf(stderr,"TEST - n_leg_colors is %d\n", n_leg_colors);                 */
/* fprintf(stderr,"TEST - ud_lvl[n_leg_colors] is %d\n", ud_lvl[n_leg_colors]); */
/* fprintf(stderr,"TEST - ud_lvl[last_num_color+1] is %d\n",                    */
/*         ud_lvl[last_num_color+1]);                                           */
     
/*  TEST MUST RESET TYPE WHEN TESTING */
/*fprintf(stderr,"TEST-first numerical value is %u, last numerical value is %u\n"*/
/*               "  first numerical color is %d, last numerical color is %d\n",  */
/*                 first_ui, last_ui, first_num_color, last_num_color);          */
    
    /*  FUTURE TO DO -  TEST METHODS 4 AND 6 */
     switch (method) {
    
        case 4:
           if(last_si < 0 && first_si < 0) /*  both negative */
               pix_per_val = (float)n_val_hgt / (float)-(last_si - first_si);
           else if(last_si >= 0 && first_si < 0) 
               pix_per_val = (float)n_val_hgt / (float)(last_si + first_si);
           else /*  both positive */
               pix_per_val = (float)n_val_hgt / (float)(last_si - first_si);
           break;
           
        case 5:
           pix_per_val = (float)n_val_hgt / (float)(last_ui - first_ui);
           break;
           
        case 6:
           if(last_d < 0 && first_d < 0) /*  both negative */
               pix_per_val = (float)n_val_hgt / (float)-(last_d - first_d);
           else if(last_d >= 0 && first_d < 0) 
               pix_per_val = (float)n_val_hgt / (float)(last_d + first_d);
           else /*  both positive */
               pix_per_val = (float)n_val_hgt / (float)(last_d - first_d);
           break;
           
        default:
           break;
           
     } /*  end switch */


     /*  precalculated y cursor positions for leading flags */
     if(n_l_flags != 0)
        for (i=0; i < n_l_flags; i++) {
            start_y[i] = y_in + ( MIN_FLAG_HT * (i) );  
        }

     /*  precalculated y cursor positions for numerical values */
     start_y[first_num_color] = y_in + (n_l_flags * MIN_FLAG_HT);

/* TEST */  
/* fprintf(stderr,"TEST - input n_val_hgt is %d, calculated pix_per_val is %f\n",*/
/*                n_val_hgt, pix_per_val);                                       */
/* fprintf(stderr,"TEST - start_y[first_num_color] is %d\n"                      */
/*                "        y_in is %d, n_l_flags * MIN_FLAG_HT IS %d\n",         */
/*               start_y[first_num_color], y_in, n_l_flags * MIN_FLAG_HT);       */

    for(i=first_num_color+1; i <= last_num_color+1; i++) {
     
        /*  FUTURE TO DO -  TEST METHODS 4 AND 6 */
        switch (method) {
    
            case 4:
               if(sd_lvl[i] < 0 && sd_lvl[first_num_color] < 0) /* both negative */
                   delta_y = ( -(sd_lvl[i] - sd_lvl[first_num_color]) * pix_per_val);
               else if(sd_lvl[i] >= 0 && sd_lvl[first_num_color] < 0) 
                   delta_y = ( (sd_lvl[i] + sd_lvl[first_num_color]) * pix_per_val);
               else /*  both positive */
                   delta_y = ( (sd_lvl[i] - sd_lvl[first_num_color]) * pix_per_val);
               start_y[i] = start_y[first_num_color] + delta_y;  
               break;
               
            case 5:
               start_y[i] = start_y[first_num_color] + 
                      ( (ud_lvl[i] - ud_lvl[first_num_color]) * pix_per_val);
               break;
               
            case 6:
               if(dd_lvl[i] < 0 && dd_lvl[first_num_color] < 0) /*  both negative */
                   delta_y = ( -(dd_lvl[i] - dd_lvl[first_num_color]) * pix_per_val);
               else if(dd_lvl[i] >= 0 && dd_lvl[first_num_color] < 0) 
                   delta_y = ( (dd_lvl[i] + dd_lvl[first_num_color]) * pix_per_val);
               else /*  both positive */
                   delta_y = ( (dd_lvl[i] - dd_lvl[first_num_color]) * pix_per_val);
               start_y[i] = start_y[first_num_color] + delta_y;
               break;
               
            default:
               break;
               
         } /*  end switch */

    }  /*  end for */
    
     /*  NOTICE: the previous loop calculated the height of the last numerical color */
     /*          bar and the beginning of the first trailing flag (if it exists) */
     /*          the first trailing flag begins at start_y[last_num_color+1] */
     
     /*  precalculated y cursor positions for trailing flags */
     if(n_t_flags != 0) {
        /*  requires calculating the bar height of the last numerical value */
        bh = start_y[last_num_color+1] - start_y[last_num_color];

/* TEST  MUST RESET TYPE WHEN TESTING */
/* fprintf(stderr,                                                              */
/*       "TEST last num bar hgt is %d, trail_fl_dl is %d, last_num_dl is %d\n", */
/*       bh, ud_lvl[last_num_color+1], ud_lvl[last_num_color]);                 */

        /*  we already know where the first trailing flag starts, so we begin  */
        /*  with the second trailing flag (if it exists). */
          for (i=2; i <= n_t_flags; i++)
             start_y[last_num_color+i] = start_y[last_num_color] + bh 
                                                +  (MIN_FLAG_HT * (i-1));

     }


     /* diagnostic info displayed for developer */
     fprintf(stderr,"\nGENERIC CONFIGURATION INFORMATION\n");
     fprintf(stderr, 
             "The number of colors configured is %d\n", n_leg_colors);
     fprintf(stderr, 
             "There are %d leading flag(s) and %d trailing flag(s)\n",
             n_l_flags, n_t_flags);

     switch (method) {
  
          case 4:
              fprintf(stderr, 
                      "The first numerical data threshold is %d at index %d\n"
                      "the last numerical data threshold is %d at index %d\n\n", 
                                 sd_lvl[first_num_color], first_num_color, 
                                 sd_lvl[last_num_color], last_num_color );
              break;
              
          case 5:
              fprintf(stderr, 
                      "The first numerical data threshold is %d at index %d\n"
                      "the last numerical data threshold is %d at index %d\n\n", 
                                 ud_lvl[first_num_color], first_num_color, 
                                 ud_lvl[last_num_color], last_num_color );
              break;
              
          case 6:
              fprintf(stderr, 
                      "The first numerical data threshold is %f at index %d\n"
                      "the last numerical data threshold is %f at index %d\n\n", 
                                 dd_lvl[first_num_color], first_num_color, 
                                 dd_lvl[last_num_color], last_num_color );
              break;
              
          default:
              break;
              
      } /*  end switch */




     /* 3.-- draw the legned threshold labels */   
     for(i = 0; i < n_leg_colors; i++) {

        /* print the threshold labels */
        if(strlen(l_label[i]) > 0)
           XDrawString(display, canvas, gc, lx, start_y[i] + ly, l_label[i],
                                                    strlen(l_label[i]));
     } /*  end for */
     


     /* 4.-- draw the legned color bars */ 
/* TEST */
/* fprintf(stderr,"TEST - PRINT THE CONTENTS OF THE START Y ARRAY\n"  */
/*                 "       beginning y y_in is %d, n_val_hgt is %d\n",*/ 
/*                 y_in, n_val_hgt);                                  */

     /** Reading The Palette File And Setting Up Bar Colors **/
     /* CVG 8.9  */
     retval = open_legend_block_palette(prod_id, frame_location);
    
     if(retval == FALSE) /* palette not opened */
         return;
        
     for(i = 0; i < n_leg_colors; i++) {
                                                     
        /* determine the bar height */
        if ( (i < first_num_color) || (i > last_num_color) )
           bh = MIN_FLAG_HT;

        /*  CURRENTLY WE RELY ON THE CONTENTS OF THE LEGEND FILE TO INCLUDE */
        /*  a. the maximum expected data level for the final numerical threshold */
        /*     color bar, if there are no trailing flags */
        /*  b. a trailing flag data level that is immediately after the maximum */
        /*     expected numerical data level    */
        
        else
           bh = start_y[i+1] - start_y[i];


        /* outline flag bars if color is black (background) */
        if ( (display_colors[b_color[i]].red == 0 &&
              display_colors[b_color[i]].green == 0 &&
              display_colors[b_color[i]].blue == 0) 
            && (i < first_num_color || i > last_num_color) ) { 
           XSetForeground(display, gc, grey_color);
           XDrawRectangle(display, canvas, gc, cx, start_y[i] + cy, bw, bh);
           
        } else {
           XSetForeground(display, gc, display_colors[b_color[i]].pixel);
           XDrawRectangle(display, canvas, gc, cx, start_y[i] + cy, bw, bh);
           XFillRectangle(display, canvas, gc, cx, start_y[i] + cy, bw, bh);
       }

/*  TEST  MUST RESET TYPE WHEN TESTING */
/* fprintf(stderr,"TEST - DL %3u, color %3d, pixel_y %3d, text '%s', height %d\n",*/
/*              ud_lvl[i] , b_color[i], start_y[i], l_label[i], bh);              */
           
     } /*  end for */
      
     
} /*  end display_generic_legend() */







/* Helper Function for Method 5 */
/* This function should NOT be called if decode_flag is NO_DECODE  */
/* which means Method 5 was not used to configure the product      */
/* protects against a bad configuration of Scale == 0.0 which      */
/* would result in division by 0                                   */
/* CVG 8.8 changed return value to double */
double decode_data_level(unsigned int data_level, int parameter_source, 
                                                         int frame_loc)
{
    int decode_err = FALSE;
    double decode_val=0; /* CVG 8.8 changed to double */
    decode_params_t *sd_decode_ptr=NULL;
    
    if(sd->last_image==DIGITAL_IMAGE)
        sd_decode_ptr = &sd->dhr->decode;
    else if(sd->last_image==GENERIC_RADIAL)
        sd_decode_ptr = &sd->gen_rad->decode;
    
    if(frame_loc == PREFS_FRAME) {
        if(leg_Scale != 0.0) {
            decode_val = ( ((double)data_level) - ((double)leg_Offset) ) 
                                   / (double)leg_Scale;
        } else {
            decode_err = TRUE;
        }
        
    } else if(parameter_source == FILE_PARAM) {
        if(sd_decode_ptr->leg_Scale != 0.0) {
            decode_val = 
                 ( ((double)data_level) - ((double)sd_decode_ptr->leg_Offset) ) 
                                   / (double)sd_decode_ptr->leg_Scale;
        } else {
            decode_err = TRUE;
        }
        
    } else if(parameter_source == PROD_PARAM) {
        if(sd_decode_ptr->prod_Scale != 0.0) {
            decode_val = 
                 ( ((double)data_level) - ((double)sd_decode_ptr->prod_Offset) ) 
                                   / (double)sd_decode_ptr->prod_Scale;
        } else {
            decode_err = TRUE;
        }
        
    }
    
    if(decode_err == TRUE) {
        decode_val = 0.0;
        fprintf(stderr,"CONFIGURATION ERROR - Attempting to use Scale Offset \n"
                       "     for legend label calculation with Scale value 0.0\n");
    } 
    
    return decode_val;
    
    
} /* end decode_data_level */







/****************************************************************************/
/* SECTION C - RLE LEGENDS                                                  */
/****************************************************************************/



/*  HELPER FUNCTION     Method 0                                            */
/****************************************************************************/
void display_rle_labels(Drawable canvas, int h_in, int x_in, int y_in,  
                             int frame_location)
{

int i;

char  buf[60], buf2[20];

      /* File Global used:  unsigned short thresh[16] */

      if(frame_location != PREFS_FRAME) { /*  normal display function */
          /* rle products have threshold labels encoded in the product */
        
          unsigned short coded_thresh, msb_flag, lsbyte;
      
          for(i=0; i<16; i++) {
              /* the native format of these threshold value is a short */
              coded_thresh = (unsigned short)thresh[i];
              msb_flag = coded_thresh >> 15;
              lsbyte = coded_thresh & 0x00ff;
              /* if the most significant bit is one, then the least
               * significant byte specifies some special message */
              if(msb_flag == 1) {
      
                  /*  Legacy Legend Text Definitions */
                  if(lsbyte == 1)
                    sprintf(buf, "TH");  /*  Below Threshold */
                  else if(lsbyte == 2)
                    sprintf(buf, "ND");  /*  No Data */
                  else if(lsbyte == 3)
                    sprintf(buf, "RF");  /*  Range Folded */
                  
                  /*  HC product Legend Text Definitions */
                  else if(lsbyte == 4)
                    sprintf(buf, "BI");  /*  Biological */
                  else if(lsbyte == 5)
                    sprintf(buf, "GC");  /*  AP/Ground Clutter */
                  else if(lsbyte == 6)
                    sprintf(buf, "IC");  /*  Ice Crystals */
                  else if(lsbyte == 7)
                    sprintf(buf, "GR");  /*  Graupel */
                  else if(lsbyte == 8)
                    sprintf(buf, "WS");  /*  Wet Snow */
                  else if(lsbyte == 9)
                    sprintf(buf, "DS");  /*  Dry Snow  */
                  else if(lsbyte == 10)
                    sprintf(buf, "RA");  /*  Light and Mod Rain */
                  else if(lsbyte == 11)
                    sprintf(buf, "HR");  /*  Heavy Rain */
                  else if(lsbyte == 12)
                    sprintf(buf, "BD");  /*  Big Drops */
                  else if(lsbyte == 13)
                    sprintf(buf, "HA");  /*  Hail and Rain Mixed */
                  else if(lsbyte == 14)
                    sprintf(buf, "UK");  /*  Unknown */
      
                  else if(lsbyte == 15)
                    sprintf(buf, "  ");
                    
                  else
                    sprintf(buf, " ");
      
              } else { /* otherwise the least sig. byte is a number */
                  /* but the most sig byte can have a bunch of flags */
                  strcpy(buf2, "");
                  if( (coded_thresh & 0x0100) >> 8 )  /* bit 7 (of msb) */
                    strcat(buf2, "-");
                  if( (coded_thresh & 0x0200) >> 9 )  /* bit 6 */
                    strcat(buf2, "+");
                  if( (coded_thresh & 0x0400) >> 10)  /* bit 5 */
                    strcat(buf2, "<");
                  if( (coded_thresh & 0x0800) >> 11)  /* bit 4 */
                    strcat(buf2, ">");
            
                  if( (coded_thresh & 0x1000) >> 12)  /* bit 3 */
                      sprintf(buf, "%s%5.1f", buf2, ((float)lsbyte)/10.0f );
                  else if( (coded_thresh & 0x2000) >> 13)  /* bit 2 */
                      sprintf(buf, "%s%5.2f", buf2, ((float)lsbyte)/20.0f );
                  else if( (coded_thresh & 0x4000) >> 14)  /* bit 1 */
                      sprintf(buf, "%s%5.2f", buf2, ((float)lsbyte)/100.0f );
      
                  else
                    sprintf(buf, "%s%d", buf2, lsbyte);
      
              }  /*  end else */
               
              /* now that we've figured out what we're printing out, show it! */
              XDrawString(display, canvas, gc, x_in, y_in, buf, strlen(buf));
              y_in += h_in;  
      
          }  /*  end for */
           
      }  /* end if != PREFS_FRAME draw labels */
      
} /* end display_rle_labels()  */




/*  HELPER FUNCTION     Method 0                                            */
/****************************************************************************/
void draw_rle_color_bars(Drawable canvas, int h_in, int x_in, int y_in,  
                             int frame_location)
{

/* NOTE: CVG 8.7 - frame_location not currently used */

int i, w;

     /* bar width allways 50 since the legend bar area is now */
     /* copied into the legend frame rather than displayed    */
     w = 50;

        
     for(i=0; i<palette_size; ++i) {
         
         XSetForeground(display, gc, display_colors[i].pixel);
         XDrawRectangle(display, canvas, gc, x_in, y_in, w, h_in);
         XFillRectangle(display, canvas, gc, x_in, y_in, w, h_in);
         if (i == 0) { /* assumes background and color 0 are the same */
            XSetForeground(display, gc, grey_color);
            XDrawRectangle(display, canvas, gc, x_in, y_in, w, h_in);
         }
         
         y_in += h_in;
         
     } /*  end for */


} /* end draw_rle_color_bars() */

