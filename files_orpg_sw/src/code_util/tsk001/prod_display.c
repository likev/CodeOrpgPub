/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/07 21:49:41 $
 * $Id: prod_display.c,v 1.10 2014/11/07 21:49:41 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */
/* prod_display.c */


#include "prod_display.h"


/* CVG 9.0 used by plot image and display_legend */
int use_cvg_colors;
/* CVG 9.0 - Used in plot_image() and add_or_page_gab() */
int *msg_type;

 /******************************************************************************
  ******************************************************************************
  ******************************************************************************
  ******************************************************************************
  */
/* first we make sure that the active screen data points to the correct screen,
 * then we plot the current selection.  this  sets a lot of side info as well
 *
 * for this function to actually do something, an ICD product must have been 
 * loaded, parsed to figure out what layers/packets it has, and had some of these
 * layers/packets selected for display
 *
 * The only time add_history is FALSE is when plot_image is called from
 * replay_history.  When FALSE the image being displayed is not added to history 
 * and existing history is not cleared. This also results in dispatch_packet_type
 * being called with the replay flag set TRUE.
 */
/* this function is called from the following in packetselect.c
 *   the select_layer_or_packet function
 *   the select_all_packets function
 * called from the following in history.c
 *   the replay_history function
 * called from the followning in anim.c
 *   the display_animation_product function
 */
 
void plot_image(int screen_num, int add_history)
{
    int i, j, index;
    Prod_header *hdr;
/*    int *msg_type; CVG 9.0 to file SCOPE */
    int gabj=0,gabi=0,gabindex=-1;
    int r;

    int replay;

    XRectangle  clip_rectangle;

    /*     XPoint    X[5];  // used for diamond symbol */
    XSegment seg[4];   /*  used for plus symbol */
    int numseg=4;

    /* CVG 9.0 */
    int retval;
    int no_blocks = FALSE;

    if(verbose_flag) {
        fprintf(stderr,"Enter plot_image() overlay_flag %d - 1 (TRUE) \n"
                       "  add_history %d - 1 (TRUE) means replay FALSE\n", 
                       overlay_flag, add_history);
    }
    
    /* if nothing exists to be selected, nothing _can_ be selected or plotted */
    /* A SHORTCUT */
    if(sd->layers == NULL) {
        fprintf(stderr,"\nPRODUCT NOT DISPLAYED (Plot Image)\n");
        fprintf(stderr,"The product contained no displayable layers\n\n");
        return;
    }
    
    if(sd->icd_product == NULL) {
        fprintf(stderr,"\nPRODUCT NOT DISPLAYED (Plot Image)\n");
        fprintf(stderr,"The product was not loaded\n\n");
        return;
    }
    
    /* If product not configured, do not display */
    hdr = (Prod_header *)(sd->icd_product);
    msg_type = assoc_access_i(msg_type_list, hdr->g.prod_id);
    if (msg_type == NULL) {
        fprintf(stderr,"\nCONFIGURATION ERROR (Product Display)\n");
        fprintf(stderr,"This product has not been configured.\n");
        fprintf(stderr,"Enter preferences for product id %d\n", hdr->g.prod_id);
        fprintf(stderr,"using the CVG Site Specific Preferences Menu.\n\n");
        /* CVG 9.0 */
        return;
    
    }

    /* set up globals */
    if(screen_num == SCREEN_1) {
        display = XtDisplay(screen_1);
        window = XtWindow(screen_1);
        sd = sd1;

    } else if(screen_num == SCREEN_2) {
        display = XtDisplay(screen_2);
        window = XtWindow(screen_2);
        sd = sd2;

    } else if(screen_num == SCREEN_3) {
        display = XtDisplay(screen_3);
        window = XtWindow(screen_3);
        sd = sd3;
    }

    /* if we're not overlaying, clear both the off screen pixmap we're writing
     * to and the info about the previous products we've plotted
     * ALSO zero out the center offset regardless of message type
     * and disable zoom if not geographic
     */
    /* clearing the entire global_packet_list array and */
    /* the global_display_colors array fixed a problem */
    /* with the export colors of the new background maps. */
    if(overlay_flag == FALSE) {
       
       if(sd==sd1) {
           global_palette_size_1=0;

           for(r=0;r<100;r++)
              global_packet_list[r]=0;
           
           for(r=0;r<256;r++) {
              global_display_colors_1[r].flags = DoRed | DoGreen | DoBlue;
              global_display_colors_1[r].red = 0;
              global_display_colors_1[r].green = 0;
              global_display_colors_1[r].blue = 0;
           }
             
           if(*msg_type != GEOGRAPHIC_PRODUCT) {
              XtVaSetValues(zoom_opt1, 
                    XmNmenuHistory,      zoombutnormal1, 
                    NULL);
              XtSetSensitive(zoom_opt1, False);
              sd->scale_factor = 1.0;
           } else {
              XtSetSensitive(zoom_opt1, True);
           }
        
       } else if(sd==sd2) {
           global_palette_size_2=0;

           for(r=0;r<100;r++)
              global_packet_list_2[r]=0;
           
           for(r=0;r<256;r++) {
              global_display_colors_2[r].flags = DoRed | DoGreen | DoBlue;
              global_display_colors_2[r].red = 0;
              global_display_colors_2[r].green = 0;
              global_display_colors_2[r].blue = 0;
           }
                        
           if(*msg_type != GEOGRAPHIC_PRODUCT) {
              XtVaSetValues(zoom_opt2, 
                    XmNmenuHistory,      zoombutnormal2, 
                    NULL);
              XtSetSensitive(zoom_opt2, False);
              sd->scale_factor = 1.0;
           } else {
              XtSetSensitive(zoom_opt2, True);
           }
             
       } else if(sd==sd3) {
           global_palette_size_3=0;

           for(r=0;r<100;r++)          
              global_packet_list_3[r]=0;

           for(r=0;r<256;r++) {
              global_display_colors_3[r].flags = DoRed | DoGreen | DoBlue;
              global_display_colors_3[r].red = 0;
              global_display_colors_3[r].green = 0;
              global_display_colors_3[r].blue = 0;
           }
                        
           if(*msg_type != GEOGRAPHIC_PRODUCT) {
              XtVaSetValues(zoom_opt3, 
                    XmNmenuHistory,      zoombutnormal3, 
                    NULL);
              XtSetSensitive(zoom_opt3, False);
              sd->scale_factor = 1.0;
           } else {
              XtSetSensitive(zoom_opt3, True);
           }
       }
              
       clear_pixmap(screen_num);
       
       
       /* if we are replaying from history, add_history is FALSE so we keep it, */
       /* otherwise whenever overlay_flag is FALSE we delete history            */
       if(add_history == TRUE)
           clear_history(&(sd->history), &(sd->history_size));
           

    } /*  end if overlay_flag FALSE */
    
    




    
    /* if add_history is TRUE we are not replaying from history so we save this */
    if(add_history == TRUE) {
        save_current_state_to_history(&(sd->history), &(sd->history_size));
    }

    /* flag sent to dispatch_packet_type */
    if(add_history == TRUE) {
        replay = FALSE;
    } else {
        replay = TRUE;
    }
    
       /* CVG 9.0 - added here to not display previous legend bar information */
       /* when replay and overlay_flag is FALSE, was in dispatch_packets */

       /* Be Careful Here - deleting the packet info too early will
        * cause problems because the function also resets sd->last_image.
        * sd->last_image is used to determine whether and how to display
        * the legend and is used by the mouse data click function to 
        * determine what information to return.
        */
       /* CVG 9.0 - logic is now the same for GEOGRAPHIC_PRODUCT */
       /*           and NON_GEOGRAPHIC_PRODUCT                   */
       if(*msg_type!=GEOGRAPHIC_PRODUCT) {
           
           if( (replay==FALSE) && (overlay_flag==FALSE) ) {
            
               if(sd->last_image == RASTER_IMAGE)
                   delete_raster();
               else if(sd->last_image == RLE_IMAGE)
                   delete_radial_rle();
               else if(sd->last_image == DIGITAL_IMAGE)
                   delete_packet_16();
               else if(sd->last_image == PRECIP_ARRAY_IMAGE)
                   delete_packet_17();
               else if(sd->last_image == GENERIC_RADIAL)
                   delete_generic_radial();
               else if(sd->last_image == GENERIC_GRID)
                    ;/* delete_generic_grid(); not yet implemented */
           }
        
       /* CVG 9.0 - moved from dispatch_packet_type() */
       } else { /* a GEOGRAPHIC_PRODUCT */
        
           if( (replay==FALSE) && (overlay_flag==FALSE) ) {
            
               if(sd->last_image == RASTER_IMAGE)
                   delete_raster();
               else if(sd->last_image == RLE_IMAGE)
                   delete_radial_rle();
               else if(sd->last_image == DIGITAL_IMAGE)
                   delete_packet_16();
               else if(sd->last_image == PRECIP_ARRAY_IMAGE)
                   delete_packet_17();
               else if(sd->last_image == GENERIC_RADIAL)
                   delete_generic_radial();
               else if(sd->last_image == GENERIC_GRID)
                    ;/* delete_generic_grid(); not yet implemented */
            
           }
        
      } /* end else a GEOGRAPHIC_PRODUCT */
           
    /* CVG 9.1 - this will be set to TRUE in open_config_or_default_palette() */
    /*           if a 2-d array packet has a configured palette */ 
    display_color_bars=FALSE;


    /* clip drawing to the main part of the digital canvas
     * (the non-sidebar part)
     */
    clip_rectangle.x       = 0;
    clip_rectangle.y       = 0;
    clip_rectangle.width   = pwidth;
    clip_rectangle.height  = pheight;
    XSetClipRectangles(display, gc, 0, 0, &clip_rectangle, 1, Unsorted);



    /* plot the selected packets */

/*  DEBUG */
/* fprintf(stderr,"DEBUG plot_image - beginning plotting data packets\n"); */
 
    /* note we delay plotting the GAB so it is on top of product graphics */

    /* the TAB (code 56) is no longer displayed with SELECT_ALL */
    
    if(sd->packet_select_type == SELECT_NONE) {
        fprintf(stderr,"\nPRODUCT NOT DISPLAYED\n");
        fprintf(stderr,"No packets were selected for display.\n\n");
        return;

    } else if(sd->packet_select_type == SELECT_ALL) { /* plot every packet */
        for(i=0;i<sd->num_layers;i++) {
            for(j=0;j<sd->layers[i].num_packets;j++) {
                index = transfer_packet_code(sd->layers[i].codes[j]);
            
                if( (index!=GRAPHIC_ALPHA_BLOCK) && 
                    (index!=TABULAR_ALPHA_BLOCK) ) {
                    dispatch_packet_type(index, sd->layers[i].offsets[j], replay);
                }
                else if(index==GRAPHIC_ALPHA_BLOCK) {
                            gabindex=GRAPHIC_ALPHA_BLOCK;
                            gabi=i;
                            gabj=j;
                }
            } /*  end for j<num_packets */
        } /*  end for i<num_layers */

    } else if(sd->packet_select_type == SELECT_LAYER) {  
        /* plot every packet in the layer */
        for(j=0;j<sd->layers[sd->layer_select].num_packets;j++) {
            index = transfer_packet_code(sd->layers[sd->layer_select].codes[j]);
            if(index!=GRAPHIC_ALPHA_BLOCK) {
                dispatch_packet_type(index, 
                                     sd->layers[sd->layer_select].offsets[j], replay);
            } else {
                gabindex=GRAPHIC_ALPHA_BLOCK;
                gabi=sd->layer_select;
                gabj=j;
            }
        } /*  end for */

    } else if(sd->packet_select_type == SELECT_PACKET) {  /* plot one packet */
        index = transfer_packet_code( 
                         sd->layers[sd->layer_select].codes[sd->packet_select] );

        if(index!=GRAPHIC_ALPHA_BLOCK) {
            dispatch_packet_type(index, 
                         sd->layers[sd->layer_select].offsets[sd->packet_select], replay);
        } else {
            gabindex=GRAPHIC_ALPHA_BLOCK;
            gabi=sd->layer_select;
            gabj=sd->packet_select;
        }

    } else {
        if(verbose_flag)
            fprintf(stderr,">>CVG ERROR: bad packet select type = %d\n", 
                                            sd->packet_select_type);
        return;
    }

    
/*  DEBUG */
/* fprintf(stderr,"DEBUG plot_image - beginning plotting selected attributes\n"); */
 
    /* do any overlays that have been selected but only */
    /* display for products configured as GEOGRAPHIC_PRODUCTS */
    
    if( (*msg_type == 0 && overlay_flag == FALSE) ) {    

        if(screen_num == SCREEN_1) {
            if(map_flag1 == TRUE)
                display_map(SCREEN_1);
            if(az_line_flag1 == TRUE)
                draw_az_lines(SCREEN_1);
            if(range_ring_flag1 == TRUE)
                draw_range_rings(SCREEN_1);

        } else if(screen_num == SCREEN_2) {
            if(map_flag2 == TRUE)
                display_map(SCREEN_2);
            if(az_line_flag2 == TRUE)
                draw_az_lines(SCREEN_2);
            if(range_ring_flag2 == TRUE)
                draw_range_rings(SCREEN_2);

        } else if(screen_num == SCREEN_3) {
            if(map_flag3 == TRUE)
                display_map(SCREEN_3);
            if(az_line_flag3 == TRUE)
                draw_az_lines(SCREEN_3);
            if(range_ring_flag3 == TRUE)
                draw_range_rings(SCREEN_3);
        }
  
        /* CVG 9.0 - ADDED HERE */
        /*  setup legend text palette file here */
        /*  text_1.plt     */
        /* 0-black 1-white */
        retval = open_default_palette(LEGEND_TEXT_DATA);
        
        if(retval==FALSE)
             use_cvg_colors = TRUE;
           
         if(use_cvg_colors == TRUE)  {
             XSetForeground(display, gc, white_color);
             fprintf(stderr,"In plot_image(), using internal colors\n");
         } else {
             XSetForeground(display, gc, display_colors[1].pixel);
         }

        XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);

        if(ctr_loc_icon_visible_flag == TRUE) {
            
/* /        */ /* draw diamond symbol at canvas center */
/* /        */ /* center is (-1, -1), width is 2 */
/* /    //        X[0].x = pwidth/2 - 4; */
/* /            X[0].x = pwidth/2 - 6; */
/* /    //        X[0].y = pheight/2; */
/* /            X[0].y = pheight/2 - 1; */
/* /    //        X[1].x = pwidth/2; */
/* /            X[1].x = pwidth/2 - 1; */
/* /    //        X[1].y = pheight/2 - 4; */
/* /            X[1].y = pheight/2 - 6; */
/* / */
/* /            X[2].x = pwidth/2 + 4; */
/* /    //        X[2].y = pheight/2; */
/* /            X[2].y = pheight/2 - 1; */
/* /    //        X[3].x = pwidth/2; */
/* /            X[3].x = pwidth/2 - 1; */
/* /            X[3].y = pheight/2 + 4; */
/* /    //        X[4].x = pwidth/2 - 4; */
/* /            X[4].x = pwidth/2 - 6; */
/* /            X[4].y = pheight/2 - 1; */
/* /            XDrawLines(display, sd->pixmap, gc, X, 5, CoordModeOrigin); */
            
         /* draw a plus symbol at the canvas center */
         /* center is (-1, -1), width is 2 */
            seg[0].x1 = pwidth/2 - 6;
            seg[0].y1 = pheight/2 - 1;
            seg[0].x2 = pwidth/2 - 16;
            seg[0].y2 = pheight/2 - 1;        
            
            seg[1].x1 = pwidth/2 - 1;
            seg[1].y1 = pheight/2 - 6;
            seg[1].x2 = pwidth/2 - 1;
            seg[1].y2 = pheight/2 - 16;         
            
            seg[2].x1 = pwidth/2 + 4;
            seg[2].y1 = pheight/2 - 1;
            seg[2].x2 = pwidth/2 + 14;
            seg[2].y2 = pheight/2 - 1;         
            
            seg[3].x1 = pwidth/2 - 1;
            seg[3].y1 = pheight/2 + 4;
            seg[3].x2 = pwidth/2 - 1;
            seg[3].y2 = pheight/2 + 14;         
            XDrawSegments(display, sd->pixmap, gc, seg, numseg);
            
            
            /* finish */
            XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);
        
        } /*  end if ctr_loc_icon_visible */

    } /*  end if geographic or overlay product */



    /* restore the full area of the digital canvas for drawing */
    
    clip_rectangle.x       = 0;
    clip_rectangle.y       = 0;
    clip_rectangle.width   = pwidth + barwidth;
    clip_rectangle.height  = pheight;
    XSetClipRectangles(display, gc, 0, 0, &clip_rectangle, 1, Unsorted);



/*  FUTURE WORK - The most robust way of displaying the legend and color bars */
/*                would be to display them from dispatch packets just after */
/*                displaying the 2D array data packets (radial / raster). */
/*                Could eliminate logic in open_config_or_default_palette */

    if(overlay_flag == FALSE) {
/* eliminate pdp display when using small pixmap */
/*  DEBUG */
/* fprintf(stderr,"DEBUG plot_image - calling display_legend \n"); */

        /* cvg 9.0 - prevent block display from previous product when displaying */
        /*             a NON_GEOGRAPHIC_PRODUCT with no 2-d array (i.e, VAD & VWP) */
        if( (*msg_type != GEOGRAPHIC_PRODUCT) && (sd->last_image==NO_IMAGE)  ) {
            no_blocks = TRUE;
        } else {
            no_blocks = FALSE;
        }
        
        /* CVG 9.0 - CHANGED 760 TO 768 */
        /* CVG 9.1 */
        if(pheight==SMALL_IMG)
            display_legend(sd->pixmap, pwidth, no_blocks, FALSE);
        else
            display_legend(sd->pixmap, pwidth, no_blocks, TRUE);
    }


    if( (overlay_flag == FALSE) ) {
        legend_clear_pixmap(screen_num);
        /* added for outside legend display */
        if( sd->last_image==DIGITAL_IMAGE || sd->last_image==RLE_IMAGE || 
            sd->last_image==RASTER_IMAGE || sd->last_image==PRECIP_ARRAY_IMAGE ||
                                                  sd->last_image==GENERIC_RADIAL )
            legend_copy_pixmap(screen_num);

        legend_show_pixmap(screen_num);

    } /*  end if overlay_flag */
   
    /* CVG 9.0 - moved to just after LEGEND DISPLAY */    
    if(gabindex==GRAPHIC_ALPHA_BLOCK) {
/*  DEBUG */
/* fprintf(stderr,"DEBUG plot_image - displaying GAB packet \n"); */
        dispatch_packet_type(gabindex,sd->layers[gabi].offsets[gabj], replay);
    }

    /* now, copy what's been plotted to the screen */
    if(screen_num == SCREEN_1)
        XCopyArea(XtDisplay(screen_1),sd1->pixmap,XtWindow(screen_1),gc,0,0,
                            pwidth+barwidth,pheight,0,0);

    else if(screen_num == SCREEN_2)
        XCopyArea(XtDisplay(screen_2),sd2->pixmap,XtWindow(screen_2),gc,0,0,
                            pwidth+barwidth,pheight,0,0);

    else if(screen_num == SCREEN_3)
        XCopyArea(XtDisplay(screen_3),sd3->pixmap,XtWindow(screen_3),gc,0,0,
                            pwidth+barwidth,pheight,0,0);
      
      
    if(compare_shell) {
       if((screen_num == SCREEN_1) && (compare_num == 1))
           display_compare_product();
       else if((screen_num == SCREEN_2) && (compare_num == 2))
           display_compare_product();       
    }


/*  DEBUG */
/* fprintf(stderr,"DEBUG plot_image - calling display_product_info \n"); */

   /* display information in the product information pane of the display screen */
   /* moved to after displaying the product to eliminate errors due */
   /*          to sd->last_image not yet being updated */
   if(overlay_flag == FALSE)
       display_product_info();

    
    
} /* end plot_image() */




 /******************************************************************************
  ******************************************************************************
  ******************************************************************************
  ******************************************************************************
  */
/* we use this function to display the legend and associated info
 * for the plotted packets.  for simplicity's sake, we use the
 * global variables;
 *
 *function is only called when overlay_flag == FALSE
 *
 * product dependent parameters are plotted for long display
 * Called with long_display_flag FALSE when using a
 *           small pixmap (768x768)  
 */
/* FUTURE ENHANCEMENT: provide a custom display of product dependent  */
/*                     parameters using the different parameter types */
/*                     unsigned short, short, etc.                    */
/* CVG 9.0 - added no_blocks_flag */
void display_legend(Drawable canvas, int legend_offset, int no_blocks_flag, 
                                                              int long_display_flag)
{
  char buf[200], *p_desc, *icao_ptr, icao[10], res_text[20];
  char *rtext_ptr, radar_text[50];
  Prod_header *hdr;
  Graphic_product *gp;
  char *prod_pointer=NULL;
  int x, y; 

  float res;

  int *radar_type_ptr, source_id, offset;
  int icd_format, pre_icd;

  /* for creating Vol time & date from pre-ICD header */
  time_t tval;
  struct tm *t1=NULL;
  int jul_date;
  
  /* CVG 9.3 - added elevation angle to display */
  int *elevation_flag;

char *mnemonic;

char prod_mnemon[4]; /*  hold 3 character mnemonic padded with blanks  */
int p_code;

/* CVG 9.0 */
int retval;

/* DEBUG */
/* fprintf(stderr,"DEBUG - Entering display_legend() \n"); */

  /* since we clip, we only display the legend when the */
  /*          overlay flag is FALSE */
  /* when displaying an overlay product, we use info from the base product */
  if(overlay_flag==TRUE && sd->history[0].icd_product != NULL) {
    hdr = (Prod_header *)(sd->history[0].icd_product);
    gp  = (Graphic_product *)(sd->history[0].icd_product+96);
    prod_pointer = sd->history[0].icd_product;
  } else {
    hdr = (Prod_header *)(sd->icd_product);
    gp  = (Graphic_product *)(sd->icd_product + 96);
    prod_pointer = sd->icd_product;
  }


  /* CVG 9.0 THIS MAY NO LONGER BE REQUIRED SINCE WE SET UP IN plot_image()*/
  /*  setup legend text palette file here */
  /*  text_1.plt     */
  /* 0-black 1-white */
  retval = open_default_palette(LEGEND_TEXT_DATA);
    
  if(retval==FALSE)
      use_cvg_colors = TRUE;
    
  if(use_cvg_colors == TRUE) {
      XSetForeground(display, gc, black_color);
      fprintf(stderr,"In display_legend(), using internal black color\n");
  } else {
      XSetForeground(display, gc, display_colors[0].pixel);
    }

  XFillRectangle(display,sd->pixmap,gc,pwidth,0,barwidth,pheight);
  
   
  /* the product name overlaps the packet image part somewhat,
   *  because it is so long */
  /* if the product has a description, use it, otherwise use a default */
  p_desc = assoc_access_s(product_names, hdr->g.prod_id);
  if(p_desc == NULL)
    strcpy(buf, "Product Description Not Configured");
  else
    strcpy(buf, p_desc);


  /*  beginning x-position based upon the length of the description  */
  XFillRectangle(display,sd->pixmap,gc,legend_offset-(strlen(buf)*6)+115, 0, 
                   (legend_offset-(strlen(buf)*6)+115) - (pwidth+barwidth), 30);
    
  if(use_cvg_colors == TRUE) {
      XSetForeground(display, gc, white_color);
      fprintf(stderr,"In display_legend(), using internal white color\n");
  } else {
      XSetForeground(display, gc, display_colors[1].pixel);
  }

  /*  beginning x-position based upon the length of the description */
  XDrawString(display, canvas, gc, legend_offset-(strlen(buf)*6)+120, 20, 
                                                           buf, strlen(buf));


  sprintf(buf, "%s",CVG_SHORT_VER);
  XDrawString(display, canvas, gc, legend_offset+140, 20, buf, strlen(buf));

  /* get the resolution of the product */
  /* get resolution from gen_radial */
  /*  TO DO - DO I NEED TO LIMIT TO SIG DIGITS??? */
  if(sd->last_image == GENERIC_RADIAL) {
     res = sd->gen_rad->range_interval / 1000 * CVG_KM_TO_NM;
     if(res < 1.0)
        sprintf(res_text,"%4.2fnm (%dm)", res, (int) sd->gen_rad->range_interval);
     else
        sprintf(res_text,"%4.1fnm (%dm)", res, (int) sd->gen_rad->range_interval);
  } else  {
     res = res_index_to_res(sd->resolution);
     sprintf(res_text, resolution_name_list[sd->resolution]);
  }


  /* get the station identifier */
  offset = HEADER_ID_OFFSET;
  
  source_id = read_half(prod_pointer, &offset);


  icao_ptr = assoc_access_s(icao_list, source_id);

  if(icao_ptr == NULL) {
      strcpy(icao, "Unknown");
      strcpy(sd->rad_id, "    ");
  } else {
      strcpy(icao, icao_ptr);
      strcpy(sd->rad_id, icao);
  }

  /* figure out whatever the radar type string should be
   * if we can't find one, we default to a blank
   */
  radar_type_ptr = assoc_access_i(radar_type_list, source_id);
  if(radar_type_ptr == NULL) {
      strcpy(radar_text, "");
  } else {
      rtext_ptr = assoc_access_s(radar_names, *radar_type_ptr);
      if(rtext_ptr == NULL)
          strcpy(radar_text, "");
      else
          strcpy(radar_text, rtext_ptr);
  }


  /* get the short product name */
  mnemonic = assoc_access_s(product_mnemonics, hdr->g.prod_id);

  if(mnemonic == NULL)
      strcpy(prod_mnemon, "---");
  else
      strcpy(prod_mnemon, mnemonic);


  /* we need to output the string line by line
   * so we first set things up, and then do it , one line at a time */
  x = legend_offset + 5; /* initial positions */
  y = 38;  


  /* TEST FOR ICD STRUCTURE */
  /* here we just check for the pdb divider and symb divider */
  if(test_for_icd(gp->divider, gp->elev_ind, gp->vol_num, TRUE)==FALSE) 
         icd_format = FALSE;
  else
         icd_format = TRUE;

  
  /* TEST FOR PRESENCE OF PRE-ICD HEADER */
  /* test works because we loaded the first 96 bytes with 0 if no header */

  if(hdr->g.vol_num==0 && get_elev_ind(sd->icd_product,orpg_build_i)==0)
         pre_icd = FALSE;
  else
         pre_icd = TRUE;

  if(icd_format == FALSE)
      p_code = 0;
  else
      p_code = gp->prod_code;



/***********************************************************************/
  /* NOTE: we have product ID with a pre-ICD header (hdr->g.prod_id) */
  /* Without a pre-ICD header, the product code is used to look up ID's below 130 */
  sprintf(buf, "ID:%d   Name(PCode): %s(%d)", hdr->g.prod_id,
                prod_mnemon, p_code);
  XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
  y += font_height;
  sprintf(buf, "Resolution: %s", res_text);
  XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
  y += font_height;



   /* vol time and date from pre-icd header, if it exists */
   if(pre_icd == TRUE) {       
       tval=(time_t)hdr->g.vol_t;
       t1 = gmtime(&tval);
       /* YYYY is t1->tm_year+1900, MM is t1->tm_mon+1, DD is t1->tm_mday, 
        * HH is tm_hour, MM is t1->tm_min, SS is t1->tm_sec 
        */
        julian_date( t1->tm_year+1900, t1->tm_mon+1, t1->tm_mday, &jul_date );
           
   }
   /* end create short date */



/***********************************************************************/
    /*  the priority for the source of the date-time.                  */
    /*           1. use the date in the ICD product if it is one, else */
    /*           2. use the pre-ICD header if it exists                */
    /*  this priority is needed to handle TDWR products from SPG       */
    
  /* determine how to display the volume and gen dates and times */
    if(pre_icd == FALSE) { /* no pre-ICD header */
       if(icd_format == FALSE) { /* not icd format */
           sprintf(buf, "GEN: Time and Date Unknown");
           XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
           y += font_height;
           sprintf(buf, "VOL: Time and Date Unknown");
           XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
           y += font_height;
           
        } else { /* icd format */
           /* when cvg raw data is stuffed into icd format, */
           /* generation data and time are zero */
           /* can still get volume date from pre-ICD header */
           if(gp->vol_date == 0) {
                 sprintf(buf, "GEN: Time and Date Unknown");
                 XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
                 y += font_height;
                 sprintf(buf, "VOL: Time and Date Unknown");
                 XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
                 y += font_height;
           } else { /* valid vol and gen dates */
               sprintf(buf, "GEN: %s %s", _88D_secs_to_string(gp->gen_time),
                      _88D_date_to_string(gp->gen_date));
               XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
               y += font_height;
               sprintf(buf, "VOL: %s %s",
                      _88D_secs_to_string(((int)(gp->vol_time_ms) << 16)
                                        | ((int)(gp->vol_time_ls) & 0xffff)),
                      _88D_date_to_string(gp->vol_date));
               XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
               y += font_height;
           }
         
        } /*  end else ICD format */
    
    } else { /* there is a pre-ICD header */
        if(icd_format == FALSE) { /* not icd format */
           sprintf(buf, "GEN: Time and Date Unknown");
           XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
           y += font_height;
           /* we get volume date/time from pre_icd  */
           sprintf( buf, "VOL: %02d:%02d:%2d %s",
                  t1->tm_hour, t1->tm_min, t1->tm_sec,
                  _88D_date_to_string(jul_date) );
           XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
           y += font_height;
        } else { /* icd format */
           /* when cvg raw data is stuffed into icd format, */
           /* generation data and time are zero */
           /* can still get volume date from pre-ICD header */
           if(gp->vol_date == 0) {
                 sprintf(buf, "GEN: Time and Date Unknown");
                 XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
                 y += font_height;
                 /* we get volume date/time from pre_icd  */
                 sprintf( buf, "VOL: %02d:%02d:%2d %s",
                      t1->tm_hour, t1->tm_min, t1->tm_sec,
                      _88D_date_to_string(jul_date) );
                 XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
                 y += font_height;
           } else { /* valid vol and gen dates */
               sprintf(buf, "GEN: %s %s", _88D_secs_to_string(gp->gen_time),
                      _88D_date_to_string(gp->gen_date));
               XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
               y += font_height;
           /*  the priority for the source of the date-time.             */
           /*      1. use the date in the ICD product if it is one, else */
           /*      2. use the pre-ICD header if it exists                */
           /*  this priority is needed to handle TDWR products from SPG  */
               sprintf(buf, "VOL: %s %s",
                      _88D_secs_to_string(((int)(gp->vol_time_ms) << 16)
                                        | ((int)(gp->vol_time_ls) & 0xffff)),
                      _88D_date_to_string(gp->vol_date));
               XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
               y += font_height;
           } /*  end valid vol and gen times */
        } /*  end else ICD format */
        
   } /*  end else a pre-ICD header */


/***********************************************************************/
  /* determine where to read the elevation index and vcp number */
   elevation_flag = assoc_access_i(elev_flag, hdr->g.prod_id);
   if(pre_icd == FALSE) { /* no pre-ICD header */
       if(icd_format == FALSE) { /* not icd format and no header */
          sprintf(buf, "Elevation UNK   VCP UNK");
       } else { /* icd format and no header */
         if (*elevation_flag) { /* an elevation-based product */
            float elev;
            elev = gp->param_3/10.;
            /* cvg raw may have invalid vcp number */
            if(gp->vcp_num == 0)
               sprintf(buf, "Elevation %3.1f #%d VCP UNK", elev, gp->elev_ind);
            else
               sprintf(buf, "Elevation %3.1f #%d VCP %d", elev, gp->elev_ind, gp->vcp_num);
         } else { /* not an elevation-based product */
            /* cvg raw may have invalid vcp number */
            if(gp->vcp_num == 0)
               sprintf(buf, "Elevation %d   VCP UNK", gp->elev_ind);
            else
               sprintf(buf, "Elevation %d   VCP %d", gp->elev_ind, gp->vcp_num);
         }
       }
   } else { /* there is a pre-ICD header */
       if(icd_format == FALSE) { /* not icd format with header */
         if (*elevation_flag) { /* an elevation-based product */
            float elev;
            elev = gp->param_3/10.;
            sprintf(buf, "Elevation %3.1f #%d VCP UNK", 
                                  elev, get_elev_ind(sd->icd_product,orpg_build_i));
         } else { /* not an elevation-based product */
            sprintf(buf, "Elevation %d   VCP UNK", 
                                  get_elev_ind(sd->icd_product,orpg_build_i));
         }
       } else { /* icd format with header */
         if (*elevation_flag) { /* an elevation-based product */
            float elev;
            elev = gp->param_3/10.;
             /* cvg raw may have invalid vcp number */
             if(gp->vcp_num == 0)
                sprintf(buf, "Elevation %3.1f #%d VCP UNK", elev, gp->elev_ind);
             else
                sprintf(buf, "Elevation %3.1f #%d VCP %d", elev, gp->elev_ind, gp->vcp_num);
         } else { /* not an elevation-based product */
             /* cvg raw may have invalid vcp number */
             if(gp->vcp_num == 0)
                sprintf(buf, "Elevation %d   VCP UNK", gp->elev_ind);
             else
                sprintf(buf, "Elevation %d   VCP %d", gp->elev_ind, gp->vcp_num);
         }
       }
   }
  XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
  y += font_height;
  
/***********************************************************************/  

  sprintf(buf, "Station: [%s] (%s)", icao, radar_text);
  XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
  y += font_height;



  if(long_display_flag == TRUE) {
      y += (font_height/2);   
      sprintf(buf, "Product Dependant Parameters");
      XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
      y += font_height;
      sprintf(buf, " 1: % 6d         6: % 6d", gp->param_1, gp->param_6);
      XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
      y += font_height;
      sprintf(buf, " 2: % 6d         7: % 6d", gp->param_2, gp->param_7);
      XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
      y += font_height;
      sprintf(buf, " 3: % 6d         8: % 6d", gp->param_3, gp->param_8);
      XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
      y += font_height;
      sprintf(buf, " 4: % 6d         9: % 6d", gp->param_4, gp->param_9);
      XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
      y += font_height;
      sprintf(buf, " 5: % 6d        10: % 6d", gp->param_5, gp->param_10);
      XDrawString(display, canvas, gc, x, y, buf, strlen(buf));

  
  } else { /* omitting PDPs avoids loss of bottom of digital & generic color bars */
      y+= (4*font_height);

  }
  
  
  /***********************************************************************/
    /* move to where we want to start the actual legend */
  
  y += 2*font_height;
    
  x += 24;


/* CVG 8.7 - used to copy legend bars to the legend frame */
  x_legend_start = x;  /* currently 1869 large pixmap and 789 small pixmap*/
  y_legend_start = y;  /* currently 200  large pixmap and 182 small pixmap*/
  
  /*  since we clip display of all packets we only display         */
  /*            the legend once, when displaying the first product */
  /*  cvg 9.0 - also not displaying NON_GEOGRAPHIC_PRODUCT without 2-d data */
  /*              which sets no_blocks_flag FALSE                             */
  if( (overlay_flag == FALSE) || (no_blocks_flag == FALSE) )
      /* changed to avoid weird colors if displaying a non-radial / raster */
      /* product at the root level (e.g. MESO and TVS .  However when      */
      /* displaying certain non-geographic products like VAD that contain  */
      /* colors but do not contain radial raster data, no color bars are   */
      /* displayed                                                         */
      if( sd->last_image==DIGITAL_IMAGE || sd->last_image==RLE_IMAGE || 
           sd->last_image==RASTER_IMAGE || sd->last_image==PRECIP_ARRAY_IMAGE ||
                                                 sd->last_image==GENERIC_RADIAL )
        /* CVG 9.1 - only display color bars if a 2-d array packet has a configured palette */
        if(display_color_bars==TRUE)
            /* CVG 9.0 - added use_cvg_colors */
            display_legend_blocks(canvas, x_legend_start, y_legend_start, 
                                        use_cvg_colors, PRODUCT_FRAME);
        
  
} /* end display_legend() */





 /******************************************************************************
  ******************************************************************************
  ******************************************************************************
  ******************************************************************************
  */
/* prints the colored blocks that one finds in the legend
 * LEGEND ONLY DISPLAYED FOR RADIAL / RASTER PRODUCTS
 * diagnostics for PRODUCT_FRAME display
 */
 
 /*        called with frame_location PRODUCT_FRAME by display_legend() (which */
 /*        is called with TRUE by plot_image) for the canvas legend. */

 /*        ALSO called with frame_location PREFS_FRAME by several preference */
 /*        functions. */
/* CVG 9.0 - added "local_color" parameter */
void display_legend_blocks(Drawable canvas, int x, int in_y, int local_color, 
                                                               int frame_location)
{

  int h=0, *msg_type; 
 
  float res;
  
  int i, y, legend_start, *flag_ptr, thresh_fields_blank, offset;
  int digital_flag;

  /* variables for display of digital legend */
  int bar_height;
  int numerical_val_hgt=0; /*  calculating bar height for Methods 4. 5. 6 */


  char *legend_filename_ptr=NULL, legend_filename[200], buf[60], *l_units;
  
/* cvg 9.0 */
/*  char *palette_filename_ptr, palette_filename[200]; */
  int retval;
  
  char unit_line1[30], *unit_line2;
  int length1;

  Prod_header *hdr=NULL;
  Graphic_product *gp=NULL;
  char *prod_pointer=NULL;
  
  int num_flags;

  FILE *legend_file=NULL;
  
  
  int product_id;
  
  XmString xmstr;
  char *str;

  int dig_leg_error = FALSE;
  int gen_leg_error = FALSE;

  /* local variables used to calculate har height input parameter for digital    */ 
  /* display functions and numerical height input parameter for generic function */
  unsigned long bh_tot_n_lvls;
  int           nh_n_l_flags;
  int           nh_n_t_flags;


  decode_params_t *pd_decode_ptr=NULL;
  
  
 
/* DEBUG */
/* fprintf(stderr,"DEBUG - Entering display_legend_blocks() \n");                    */
/* fprintf(stderr,"DEBUG - local_color is %d \n", local_color);                      */
/* fprintf(stderr,"DEBUG - frame_location is %d, 1-PRODUCT_FRAME, 2-PREFS_FRAME \n", */
/*                 frame_location);                                                  */

/***********************************************************************
 * SECTION A - PRELIMINARIES
 *    Here we get pointers to product data and headers
 *            shortcut out if legend not appropriate for type of product
 *            draw the unit of measure (common to all methods )
 ***********************************************************************/

  /* CVG 9.0 */
  if(local_color == TRUE) {
      XSetForeground(display, gc, white_color);
      fprintf(stderr,"In display_legend_blocks(), using internal white color\n");
  } else {
      XSetForeground(display, gc, display_colors[1].pixel);
  }

  /* get the product id */

  if(frame_location != PREFS_FRAME) { /*  normal display function */
      /* when displaying an overlay product, we use info from the base product */
      if(overlay_flag==TRUE && sd->history[0].icd_product != NULL) {
          hdr = (Prod_header *)(sd->history[0].icd_product);
          gp  = (Graphic_product *)(sd->history[0].icd_product + 96);
          prod_pointer = sd->history[0].icd_product;
      } else {
          hdr = (Prod_header *)(sd->icd_product);
          gp  = (Graphic_product *)(sd->icd_product + 96);
          prod_pointer = sd->icd_product;
      }
      
      product_id = hdr->g.prod_id;

  } else { /*  called from preferences */
      XtVaGetValues(id_label, XmNlabelString, &xmstr, NULL);
      XmStringGetLtoR(xmstr, XmFONTLIST_DEFAULT_TAG, &str);
      product_id = atoi(str);
      free(str);
      XmStringFree(xmstr);
  } /* end else called from preferences */

  /* do we have a geographical product? */
  msg_type = assoc_access_i(msg_type_list, product_id);

  /* to see if this is an overlay product NOT being overlaid */
  res = res_index_to_res(sd->resolution);

  /* see if the product's legend info is encoded like the digital products' */
  flag_ptr = assoc_access_i(digital_legend_flag, product_id);
  digital_flag = *flag_ptr;


/* TEST */
/* fprintf(stderr,"TEST display_legend_blocks() prod id is %d, dig flag is %d\n", */
/*                                               product_id, digital_flag);       */ 
                                              
  /******** STOP HERE IF LEGEND NOT APPROPRIATE ********/
  /* only geographic and non-geographic product require legend bars */
  if( (*msg_type != GEOGRAPHIC_PRODUCT) && 
      (*msg_type != NON_GEOGRAPHIC_PRODUCT)) {
    fprintf(stderr,"No legend bar information required.\n");
    return;
  }
  /* if geographic overlay type and displaying at base, not required */
  if( (res == 999) && (overlay_flag == FALSE) ) {
    fprintf(stderr,"No legend bar information required.\n");
    return;
  }

  
   /* adjust for height of font */
   legend_start = in_y + font_height;
   y = legend_start;
   
   /* --------------------------------------------------------- */
   /* The unit of measure is written with y = legend_start      */
   /*                                                           */
   /* The first threshold label is with  y = legend_start +     */
   /*                                         2*font_height + 5 */
   /*                                                           */
   /* The first color bar is drawn with y = legend_start +      */
   /*                                         font_height + 8   */
   /* --------------------------------------------------------- */


  /******** DRAW UNIT OF MEASURE (IF APPLICABLE) ********/
  
  /* first show the units unless non-digital on pref edit window */
  if ( (frame_location != PREFS_FRAME)  ||
        ((frame_location == PREFS_FRAME) && (digital_flag != 0)) ) {
  
      l_units = assoc_access_s(legend_units, product_id);
      if(l_units == NULL)
        strcpy(buf, "");
      else
        sprintf(buf, "%s", l_units);
        
      /* parse the two lines (if present) separated by '\' */    
      unit_line2 =  strrchr(buf, '\\');
      if(unit_line2 == NULL) {  /* a single line unit descriptor */
          XDrawString(display, canvas, gc, x, y, buf, strlen(buf));
          y = legend_start + 2*font_height + 5; /* space below the "units" label */
      } else { /* a two line descriptor */
          length1 = strcspn(buf, "\\\0" );
          strncpy(unit_line1, buf, (size_t)length1);
          XDrawString(display, canvas, gc, x, y, unit_line1, length1);
          y = legend_start + font_height ; /* down one line */
          XDrawString(display, canvas, gc, x, y, (unit_line2+1), 
                                                strlen(unit_line2)-1);
          y = legend_start + 2*font_height + 5; /* space below the "units" label */
      }
    
  } /* end if not prefs frame or prefs frame and dig flag not 0 */
  

  /** Reading The Palette File And Setting Up Bar Colors **/
  /* CVG 9.0 - Moved To Display Of Legend Blocks! */





/***********************************************************************
 * SECTION B - READ LEGEND INFORMATION FOR DIGITAL & GENERIC PRODUCTS 
 *             READ THRESHOLD FIELDS FOR RLE PRODUCTS
 ***********************************************************************/

     /*  ALL LEGEND FILES ARE READ DURING INITIAL DISPLAY OF THE DATA PACKET */
     /*  AND THE INFO REQUIRED FOR LEGEND DISPLAY STORED IN SCREEN DATA      */
     /*  FOR PACKETS 16 AND GENERIC RADIAL */
     
     /*  Reading Legend Filss is accomplished here for the preview     */
     /*  frame and data stored in file global variables .              */


  /*** READ THRESHOLD FIELDS FOR RLE PRODUCTS ***/
 
  if(digital_flag == 0) {
         
      /* now read all of the thresholds in -- for non-digital products we also check to 
       * see that all the fields are not zero.  if they are, we assume there is nothing
       * interesting/useful in the legend info, and therefore we don't display it */
      if(frame_location != PREFS_FRAME) { /*  normal display function */
          thresh_fields_blank = FALSE;
          offset = THRESHOLD_OFFSET;
          for(i=0; i<16; ++i) {
              if( (thresh[i] = read_half(prod_pointer, &offset)) != 0 )
                  thresh_fields_blank = TRUE;
          }
          if(thresh_fields_blank==FALSE)
              fprintf(stderr,"NOTE: All threshold fields in the product are '0'\n");
      }
      
  } /* end digital_flag == 0 */
  
  
  /* The digital and generic legend configuration files are read when */
  /* when the 2D packet is displayed (packet 16, packet 17, and the   */
  /* generic radial.  They are only read here if in the PREFS_FRAME.  */
  /* Screen Data is used during product display (PRODUCT_FRAME) and   */
  /* File Globals are used if read here.                              */
  
  if(frame_location != PREFS_FRAME) {
      ; /* DO NOTHING - required legend configuration info in Screen Data */
  
  
  } else { /* PREFS_FRAME open and read digital/generic legend configuration files */
        
      if(digital_flag != 3) {  /*  only one legend file */
        
          legend_filename_ptr = assoc_access_s(digital_legend_file, product_id);
          /*  NO LEGEND FILE ERROR IS TESTED DURING PRODUCT POST LOAD */
        
      }  else if(digital_flag == 3) { /*  two legend files to choose from */
        
          if(frame_location != PREFS_FRAME) { /*  normal display function  */
              if(gp->level_1 == -635 ) {
                legend_filename_ptr = assoc_access_s(digital_legend_file, 
                                                                  product_id);
              } else if(gp->level_1 == -1270 ) {
                legend_filename_ptr = assoc_access_s(dig_legend_file_2, 
                                                                  product_id);
              } else {  /*  unexpected value - use first legend file */
                legend_filename_ptr = assoc_access_s(digital_legend_file, 
                                                                  product_id); 
              }
          } else { /*  called from preferences */
            /*  NEED TO ADD LOGIC FOR OPTION OF USING SECOND LEGEND WITH METHOD 3 */
              legend_filename_ptr = assoc_access_s(digital_legend_file, 
                                                                   product_id);
          }
        
      } /*  end if digital_flag == 3 */
    
      if(digital_flag !=0) {
    
          sprintf(legend_filename, "%s/legends/%s", config_dir, 
                                                            legend_filename_ptr);
        
          if((legend_file = fopen(legend_filename, "r")) == NULL) {
              fprintf(stderr,"\nCONFIGURATION ERROR\n");
              fprintf(stderr, "Could not open legend file: %s", legend_filename);
              return;
          }
    
      } /* end if digital_flag !=0 */
      
      
      /*** READ THE LEGEND FILES FOR NON-RLE PRODUCTS ***/
      
      
      /******** METHOD 1, 2, 3 - DIGITAL LEGEND ********/
      
      if( (digital_flag==1) || (digital_flag==2) || (digital_flag==3) ) {   
/* DEBUG */
/* fprintf(stderr,"DEBUG - reading digital legend (flag is %d)\n",digital_flag); */
    
          
          dig_leg_error = read_digital_legend(legend_file, legend_filename, 
                                                   digital_flag, frame_location);
    
    
          /* NOTE: error checking in read_generic_legend */
          /*       egend file closed here                */
          fclose(legend_file);
    
          if(dig_leg_error==LEGEND_ERROR) {
              fprintf(stderr,"ERROR - Display of Legend Bars Aborted Due to "
                              " errors in the digital legend file.\n");
              return;
          }
          
    
      /********  METHOD 4, 5, 6 - GENERIC LEGEND  ********/
      
      } else if( (digital_flag==4) || (digital_flag==5) || (digital_flag==6) ) {
/* DEBUG */
/* fprintf(stderr,"DEBUG - reading generic legend (flag is %d)\n",digital_flag); */
          
          
          gen_leg_error = read_generic_legend(legend_file, legend_filename, 
                                            digital_flag, frame_location);
    
          
          /* NOTE: error checking in read_generic_legend */
          /*       egend file closed here                */
          fclose(legend_file);
          
          if(gen_leg_error==LEGEND_ERROR) {
              fprintf(stderr,"ERROR - Display of Legend Bars Aborted Due to "
                              " errors in the generic legend file.\n");
              return;
          }
              
      } /*  end reading initial data items in Generic Legend (Method 4, 5. 6) */
  
  
  } /* end PREFS_FRAME open and read digital/generic legend configuration files */





/***********************************************************************
 * SECTION C - DRAWING THRESHOLD VALUES AND COLOR BARS
 ***********************************************************************/


  if(frame_location != PREFS_FRAME) {
      if(sd->last_image == GENERIC_RADIAL) {
          pd_decode_ptr = &sd->gen_rad->decode;
      } else if(sd->last_image == DIGITAL_IMAGE) {
          pd_decode_ptr = &sd->dhr->decode;
      }
  }
      
 
  if( (frame_location != PREFS_FRAME) &&
      ( (sd->last_image == GENERIC_RADIAL) || (sd->last_image == DIGITAL_IMAGE) ) 
                                                                               ) {
      bh_tot_n_lvls = pd_decode_ptr->tot_n_levels;
      
      if(pd_decode_ptr->decode_flag == PROD_PARAM) {
          nh_n_l_flags  = pd_decode_ptr->prod_n_l_flags;
          nh_n_t_flags  = pd_decode_ptr->prod_n_t_flags;
      } else {
          nh_n_l_flags  = pd_decode_ptr->n_l_flags;
          nh_n_t_flags  = pd_decode_ptr->n_t_flags;
      }
      
  } else {
      bh_tot_n_lvls = tot_num_lvls;
      nh_n_l_flags  = num_lead_flags;
      nh_n_t_flags  = num_trail_flags;
      
  }

/* TEST */
/* fprintf(stderr,"TEST - beginning to draw threshold labels\n");          */
/* fprintf(stderr,"TEST - num_lead_flags is %d, num_trail_flags is %d, "   */
/*                "tot_num_lvls is %lu\n",                                 */ 
/*                         num_lead_flags, num_trail_flags, tot_num_lvls );*/ 
/* fprintf(stderr,"TEST - digital flag is %d\n", digital_flag);            */

/* TEST */
/* fprintf(stderr,"TEST - beginning to draw the leading flag labels\n"); */
/* fprintf(stderr,"TEST - x position is %d, y position is %d\n", x, y);  */



  /** Reading The Palette File And Setting Up Bar Colors **/
  /* CVG 9.0 the palette file must be opened before drawing color bars, for  */
  /*           Method 1, palette file must be opened before calculating labels */
  retval = open_legend_block_palette(product_id, frame_location);
    
  if(retval == FALSE) /* palette not opened */
      return;




  /******** METHOD 1 - CALCULATED DIGITAL LEGEND ********/
    
  if(digital_flag == 1) {  /* digital threshold labels must be calculated */
/* DEBUG */
/* fprintf(stderr,"DEBUG - beginning to draw the numerical labels - METHOD 1\n"); */

     /* bar height based upon number of data levels, not number of colors */
     /* bar height never greater than 28 pixels */
     if (bh_tot_n_lvls <= 18) {
       bar_height = MAX_BAR_HT;  /* 28 */
     } else {
       bar_height = TOT_BAR_HT / tot_num_lvls;
     }
     h = bar_height;

     /* this function calculates evenly spaced threshold labels  */
     /* based upon the number of data levels and the other       */
     /* values contained in the digital legend file method 1     */
     display_calculated_dig_labels(canvas, h, 
                                   x, y, frame_location);


     /* NOTE: The palette file is read and colors set in just before */
     /*       printing the color bars                                */
         
     /* now we can actually print the colored blocks */
     y = legend_start + font_height + 8;
     x += 54;


     /* this function draws all of the data levels using the */
     /* assigned color.  This is the reason for the limit of */
     /* 512 data levels using digital legend methods 1 2 & 3 */


/* DEBUG */
/*fprintf(stderr,"DEBUG - beginning to draw the digital color bars, method %d\n", */
/*        digital_flag);                                                          */
/* fprintf(stderr,"DEBUG - x position is %d, y position is %d\n", x, y);   */

     draw_digital_color_bars(canvas, h,
                             x,  y, frame_location);

     
     

  /******** METHOD 2 AND 3 - EXPLICIT DIGITAL LEGEND ********/

  } else if ( (digital_flag == 2) || (digital_flag == 3) ) { 
    /* digital threshold labels are stated explicitly */
/* DEBUG */
/* fprintf(stderr,"DEBUG - beginning to draw the numerical labels - METHOD 2/3\n"); */

     /* bar height based upon number of data levels, not number of colors */
     /* bar height never greater than 28 pixels */
     if (bh_tot_n_lvls <= 18) {
       bar_height = MAX_BAR_HT;  /* 28 */
     } else {
       bar_height = TOT_BAR_HT / tot_num_lvls;
     }
     h = bar_height;
     
     /* this function displays the legend labels at the data     */
     /* level specified in the digital legend file method 2 or 3 */

     display_explicit_dig_labels(canvas, h, 
                                 x, y, frame_location);
                                                      

     /* NOTE: The palette file is read and colors set in just before */
     /*       printing the color bars                                */
        
     /* now we can actually print the colored blocks */
     y = legend_start + font_height + 8;
     x += 54;



     /* this function draws all of the data levels using the */
     /* assigned color.  This is the reason for the limit of */
     /* 512 data levels using digital legend methods 1 2 & 3 */
     
     
/* DEBUG */
/* fprintf(stderr,"DEBUG - beginning to draw the digital color bars, method %d\n", */
/*         digital_flag);                                                          */
/* fprintf(stderr,"DEBUG - x position is %d, y position is %d\n", x, y);      */
     
     draw_digital_color_bars(canvas, h,
                                 x, y, frame_location);




  /******** METHOD 4, 5, and 6 - GENERIC LEGEND ********/
  
  } else if( (digital_flag==4) || (digital_flag==5) || (digital_flag==6) ) {  

      /* generic products use number of legend colors to */
                          /* determine number of blocks  */
      
      /* bar height is based upon min and max values, */
      /* not number of data levels */
      num_flags = nh_n_l_flags + nh_n_t_flags;

      /*  number of vertical pixel lines available for the numerical values */
      numerical_val_hgt = TOT_BAR_HT - (num_flags * MIN_FLAG_HT);
     
      /* the height of the numerical values rather than the bar */
      /* height is passed to the function drawing the legend    */
                          
      /* This function draws the colors specified in the legend */
      /* using method 4 5 or 6.  The height of the drawn color  */
      /* bars is proportional to the relative data level value. */
      /* The threshold labels are drawn as specified at the     */
      /* selected data value.                                   */

/* DEBUG */
/* fprintf(stderr,"DEBUG - beginning to draw the digital color bars, method %d\n", */
/*         digital_flag);                                                          */
/* fprintf(stderr,"DEBUG - x position is %d, y position is %d\n", x, y);      */

      display_generic_legend(canvas, numerical_val_hgt, x, legend_start, 
                                    digital_flag,  product_id, frame_location);





  /******** METHOD 0 - NON-DIGITAL LEGEND ********/
  
  } else { /* is method 0, RLE product with encoded legend labels */
      
      /* bar height based upon number of colors, not number data levels */
      /* since rle products have a maximum of 16 colors (& data levels) */
      /* the bar height is always 32 pixels   (512/16)                  */
      h = 28;
     
      display_rle_labels(canvas, h, x, y, frame_location);
      
      /* NOTE: The palette file is read and colors set in just before */
      /*       printing the color bars                                */
         
      /* now we can actually print the colored blocks */
      y = legend_start + font_height + 8;
      x += 54;
      

/* DEBUG */
/* fprintf(stderr,"DEBUG - beginning to draw the color bars\n");              */
/* fprintf(stderr,"DEBUG - x position is %d, y position is %d\n", x, y);      */

      draw_rle_color_bars(canvas, h, x, y, frame_location);
       
  } /*  end METHOD 0 LEGEND */

/* DEBUG */
/* fprintf(stderr,"DEBUG - Finishing display_legend_blocks() \n"); */

} /* end display_legend_blocks() */





 /******************************************************************************
  ******************************************************************************
  */

int open_legend_block_palette(int prod_id, int frame_loc)
{


  char *palette_filename_ptr, palette_filename[200]; 
  FILE *the_palette_file=NULL;


  /******** READING THE PALETTE FILE AND SETTING UP INTERNAL COLORS ********/
  /* CVG 9.0 - MOVED TO JUST BEFORE DISPLAY OF LEGEND BLOCKS! */
  /* CVG 9.0 - changed name from setup_colors to open_config_or_default_palette */

  if(frame_loc != PREFS_FRAME) { /*  normal display function      */
      if(sd->last_image==DIGITAL_IMAGE) 
         open_config_or_default_palette(DIGITAL_RADIAL_DATA_ARRAY, TRUE); /* 16 */
      else if(sd->last_image==RLE_IMAGE) 
         open_config_or_default_palette(RADIAL_DATA_16_LEVELS, TRUE); /* 53 */
      else if(sd->last_image==RASTER_IMAGE) 
         open_config_or_default_palette(RASTER_DATA_7, TRUE); /* 54 */
      else if(sd->last_image==PRECIP_ARRAY_IMAGE) 
         open_config_or_default_palette(DIGITAL_PRECIP_DATA_ARRAY, TRUE); /* 17 */
      else if(sd->last_image==GENERIC_RADIAL) 
         open_config_or_default_palette(GENERIC_RADIAL_DATA, TRUE); /* 41 */
      else fprintf(stderr,"ERROR drawing legend without setting up palette\n");

   } else { /*  called from preferences */
      /*  NEED TO ADD LOGIC FOR OPTION OF USING SECOND PALETTE WITH METHOD 3 */
      palette_filename_ptr = assoc_access_s(configured_palette, prod_id);
      sprintf(palette_filename, "%s/colors/%s", config_dir, palette_filename_ptr);
      
      /* CVG 9.0 - when palette file is not configured report different message */
      if( (strcmp(palette_filename_ptr, ".plt") == 0) ||      
          (strcmp(palette_filename_ptr, ".") == 0) ||         
          (strcmp(palette_filename_ptr, "") == 0) ||          
          (strcmp(palette_filename_ptr, " ") == 0)  ) {       
          fprintf(stderr, "No palette file is configured\n"); 
          return FALSE;                                             
      }                                                       
      
      if((the_palette_file = fopen(palette_filename, "r")) == NULL) {
          fprintf(stderr, "ERROR: Could not open palette file: %s\n", 
                                                         palette_filename);
          return FALSE;
      }
 
      setup_palette(the_palette_file, 0, TRUE);

      fclose(the_palette_file);

   } /*  end else called from preferences */


  /* if the palette size is, for whatever reason, 0,
   * we don't want to be printing out blocks */
  if ( palette_size <= 0 ||  palette_size > 256) {
     fprintf(stderr, "* ERROR ********************************************\n");
     fprintf(stderr, "*        Number of colors out of range (1-256)     *\n");
     fprintf(stderr, "*        Aborting product display                  *\n");
     fprintf(stderr, "****************************************************\n");
     return FALSE;
  }
 
  return TRUE;
 

} /* end open_legend_block_palette() */







 /******************************************************************************
  ******************************************************************************
  ******************************************************************************
  ******************************************************************************
  */
/* here we display per-product info (much like that displayed on the main 
 * window) on each display window.  it requires that the needed globals be 
 * set up beforehand
 */
void display_product_info()
{
   /* CVG 8.5.2 */
   #define BUF_LEN 120
   #define RES_LEN 20
   #define NAME_LEN 32
   #define RADAR_LEN 10
   #define DESC_LEN 80
   #define TIME_LEN 25
   #define ICAO_LEN 5
        
    Prod_header *hdr;
    Graphic_product *gp;
    XmString xmstr;

    

    char buf[BUF_LEN], res_text[RES_LEN], *name, name_text[NAME_LEN];

    int n_char;
    
    float res;
    int icd_format, pre_icd;

    char *icao_ptr, icao[ICAO_LEN];
    char *rtext_ptr, radar_text[RADAR_LEN];
    int *radar_type_ptr, source_id, offset;
    int *elevation_flag;  /* CVG 9.3 - added elevation flag 1=elevation-based product) */
    
    char *p_desc;
    char *mnemonic;
    char prod_desc[DESC_LEN]; /*  hold product description */
    char prod_mnemon[4]; /*  hold 3 character mnemonic padded with blanks  */

    /* for creating short vol time */
    time_t tval;
    struct tm *t1;
    char time_string[TIME_LEN]; /*  volume date-time */
    int  dd, dm, dy;



    /* get the short name of the product */
    hdr = (Prod_header *)(sd->icd_product);
    gp  = (Graphic_product *)(sd->icd_product + 96);
    name = assoc_access_s(short_prod_names, hdr->g.prod_id);
    
    if(name == NULL) {
        strcpy(name_text, "");
        
    } else {
       
        if(strlen(name) > (NAME_LEN - 1) ) {
            strncpy(name_text, name, NAME_LEN - 1);
            name_text[NAME_LEN-1] = '\0';
            fprintf(stderr,"display_product_info - "
                           "truncated product name to %d characters\n",
                           NAME_LEN - 1);
        } else 
            strcpy(name_text, name);

    } /* end else not NULL */

    /* get the resolution of the product */

    /*  get resolution from gen_radial */
    /*  TO DO - DO I NEED TO LIMIT TO SIG DIGITS??? */
  
    if(sd->last_image == GENERIC_RADIAL) {
       res = sd->gen_rad->range_interval / 1000 * CVG_KM_TO_NM;
       sd->resolution = temporary_set_screen_res_index(sd->gen_rad->range_interval);
       if(res < 1.0)
          sprintf(res_text,"%4.2fnm (%dm)", res, (int) sd->gen_rad->range_interval);
       else
          sprintf(res_text,"%4.1fnm (%dm)", res, (int) sd->gen_rad->range_interval);

    } else  { /* not generic radial */
       res = res_index_to_res(sd->resolution);
       
        if(strlen(resolution_name_list[sd->resolution]) > (RES_LEN - 1) ) {
            strncpy(res_text, resolution_name_list[sd->resolution], RES_LEN - 1);
            res_text[RES_LEN-1] = '\0';
            fprintf(stderr,"display_product_info - "
                           "truncated resolution text to %d characters\n",
                           RES_LEN - 1);
        } else 
            strcpy(res_text, resolution_name_list[sd->resolution]);

    } /* end else not generic radial */   


    /* if the product has a description use it, otherwise use a default */
    p_desc = assoc_access_s(product_names, hdr->g.prod_id);
    mnemonic = assoc_access_s(product_mnemonics, hdr->g.prod_id);

    if(p_desc == NULL) {
        strcpy(prod_desc, "[--- Product Description Not Available ---]");
        strcpy(prod_mnemon, "---");
        
    } else { /* desc not NULL */
        
        if(strlen(p_desc) > (DESC_LEN - 1) ) {
            strncpy(prod_desc, p_desc, DESC_LEN - 1);
            prod_desc[DESC_LEN-1] = '\0';
            fprintf(stderr,"display_product_info - "
                           "truncated product description to %d characters\n",
                           DESC_LEN - 1);
        } else 
            strcpy(prod_desc, p_desc);
      
        if(mnemonic==NULL)
            strcpy(prod_mnemon, "---");

        else { /* mnemonic not NULL */
          
          if(strlen(mnemonic) > (4 - 1) ) {
              strncpy(prod_mnemon, mnemonic, 4 - 1);
              prod_mnemon[3] = '\0';
              fprintf(stderr,"display_product_info - "
                             "truncated product mnemonic to %d characters\n",
                             4 - 1);
          } else 
            strcpy(prod_mnemon, mnemonic); 
            
        } /*  end else mnemonic not NULL */
        
    } /* end else desc not NULL */

        
    /* Clear any screen "data click info" from previous product */
    xmstr = XmStringCreateLtoR("", XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(sd->data_display, XmNlabelString, xmstr, NULL);
    XtVaSetValues(sd->data_display_2, XmNlabelString, xmstr, NULL);
    /* clear product information */
    XtVaSetValues(sd->base_info, XmNlabelString, xmstr, NULL);
    XtVaSetValues(sd->base_info_2, XmNlabelString, xmstr, NULL);
    /*   clear new prod description label here        */
    XtVaSetValues(sd->prod_descript, XmNlabelString, xmstr, NULL);
    XmStringFree(xmstr);

    /* TEST FOR ICD STRUCTURE */
    /* here we just check for the pdb divider and symb divider */
    if(test_for_icd(gp->divider, gp->elev_ind, gp->vol_num, TRUE)==FALSE) 
       icd_format = FALSE;
    else
       icd_format = TRUE;


    /* TEST FOR PRESENCE OF PRE-ICD HEADER */
    /* test works because we loaded the first 96 bytes with 0 if no header */

    if(hdr->g.vol_num==0 && get_elev_ind(sd->icd_product,orpg_build_i)==0)
       pre_icd = FALSE;
    else
       pre_icd = TRUE;
           
/***********************************************************************/
    /*DON'T TRY TO GET SITE ID / RADAR TYPE IF NOT ICD FORMAT 
    or IF VOL DATE IS 0 (CVG Raw data structure) */
    if( (icd_format == TRUE) && (gp->vol_date != 0) ) {
    
       /* get the station identifier */
       offset = HEADER_ID_OFFSET;
       source_id = read_half(sd->icd_product, &offset);
   
       icao_ptr = assoc_access_s(icao_list, source_id);
       if(icao_ptr == NULL) {
           strcpy(icao, "UNK");
           
       } else { /* icao_ptr not NULL */
          
           if(strlen(icao_ptr) > (ICAO_LEN - 1) ) {
               strncpy(icao, icao_ptr, ICAO_LEN - 1);
               icao[ICAO_LEN-1] = '\0';
               fprintf(stderr,"display_product_info - "
                          "truncated icao id text to %d characters\n",
                          ICAO_LEN - 1);
           } else 
               strcpy(icao, icao_ptr);
          
       } /* end else icao_ptr not NULL */
 
      /* figure out whatever the radar type string should be
       * if we can't find one, we default to a blank
       */
       radar_type_ptr = assoc_access_i(radar_type_list, source_id);
       if(radar_type_ptr == NULL) {
          strcpy(radar_text, "UNK");
          
       } else { /*  type_ptr not NULL */
           rtext_ptr = assoc_access_s(radar_names, *radar_type_ptr);
           if(rtext_ptr == NULL) {
               strcpy(radar_text, "UNK");
               
           } else { /* rrtext not NULL */
               
               if(strlen(rtext_ptr) > (RADAR_LEN - 1) ) {
                   strncpy(radar_text, rtext_ptr, RADAR_LEN - 1);
                   radar_text[RADAR_LEN-1] = '\0';
                   fprintf(stderr,"display_product_info - "
                              "truncated radar type to %d characters\n",
                              RADAR_LEN - 1);
               } else 
                   strcpy(radar_text, rtext_ptr);
               
           } /* end else rrtext not NULL */
           
       } /*  end else type_ptr not NULL */

    } else { /*  not an ICD format or vol_date is 0 */
        strcpy(icao, "UNK");
        strcpy(radar_text, "UNK");
    }

       
       
/***********************************************************************/
    /* create short date */
    /* use pre-icd header time if it exits, if not then */
    /* we can only get volume date if an ICD product  */
    /*  the priority for the source of the date-time.             */
    /*      1. use the date in the ICD product if it is one, else */
    /*      2. use the pre-ICD header if it exists                */
    /*  this priority is needed to handle TDWR products from SPG  */

    if(icd_format == TRUE) {
            calendar_date(gp->vol_date, &dd, &dm, &dy);
            sprintf(time_string,"%4d/%02d/%02d-%s",1900+dy, 
                    dm, dd ,_88D_secs_to_string( ((int)(gp->vol_time_ms) << 16)
                                         | ((int)(gp->vol_time_ls) & 0xffff) ) );
 
        /*  should we set the date string to UNK if not an ICD format? */
        
    } else { /*  not an ICD product */
        if(pre_icd == TRUE) { /*  pre_icd exists */
            tval=(time_t)hdr->g.vol_t;
            t1 = gmtime(&tval);
            /* YYYY is t1->tm_year+1900, MM is t1->tm_mon+1, DD is t1->tm_mday, 
             * HH is tm_hour, MM is t1->tm_min, SS is t1->tm_sec 
             */
            sprintf(time_string,"%4d/%02d/%02d-%02d:%02d:%2d", t1->tm_year+1900, 
                t1->tm_mon+1, t1->tm_mday, t1->tm_hour, t1->tm_min, t1->tm_sec);
        } /*  end if pre_ICD header */
        
    } /*  end else not an ICD product  */
    
    /* end create short date */
    if(strlen(time_string) > TIME_LEN)
      fprintf(stderr,"ERROR Buffer Overrun in display_product_info(), "
                     "time_string is %d characters\n", TIME_LEN);



/***********************************************************************/
  /* set the screen header string ...            */
  /* vol sequence, elev index or angle, resolution & zoom */
  /* CVG 9.3 - added elevation flag to prod_config file.  0 = not an   */
  /*           elevation-based product, 1 = an elevation-based product */
  elevation_flag = assoc_access_i(elev_flag, hdr->g.prod_id);

  if(pre_icd == FALSE) { /* no pre-ICD header */
       /* the product ID (hdr->g.prod_id) exists because it is loaded */ 
       /* interactively by the CVG user                               */
       if(icd_format == FALSE) { /* not icd format (and no header) */
          n_char = sprintf(buf, "%04d: %s\n"
                                "Not An ICD Product\n"
                                "Vol UNK  El N/A  VCP UNK\n"
                                "Res %s",
                  hdr->g.prod_id, name_text,       res_text);

       } else { /* icd format (and no header) */
          if (*elevation_flag) { /* an elevation-based product */
             float elev;
             elev = gp->param_3/10.;
             n_char = sprintf(buf, "%04d: %s\n"
                                   "%s  (%d)\n"
                                   "Vol #%2d  El %3.1f #%2d VCP %3d\n"
                                   "Res %s",
                     hdr->g.prod_id, name_text, prod_mnemon, gp->prod_code, 
                     gp->vol_num, elev, gp->elev_ind, gp->vcp_num, res_text);
          } else {
             n_char = sprintf(buf, "%04d: %s\n"
                                   "%s  (%d)\n"
                                   "Vol #%2d  El #%2d  VCP %3d\n"
                                   "Res %s",
                     hdr->g.prod_id, name_text, prod_mnemon, gp->prod_code, 
                     gp->vol_num, gp->elev_ind, gp->vcp_num, res_text);
          } /* end else an elevation product */
       } /*  end else an icd product */

   } else { /* there is a pre-ICD header */

       if(icd_format == FALSE) {   /*  not an ICD product (with header) */

          n_char = sprintf(buf, "%04d: %s\n"
                                "Not An ICD Product\n"
                                "Vol UNK  El #%2d  VCP UNK\n"
                                "Res %s",
                  hdr->g.prod_id, name_text, 
                  get_elev_ind(sd->icd_product,orpg_build_i),
                  res_text);

        } else {  /*  is an ICD product (with header) */
          if (*elevation_flag) { /* an elevation-based product */
             float elev;
             elev = gp->param_3/10.;
             n_char = sprintf(buf, "%04d: %s\n"
                                   "%s  (%d)\n"
                                   "Vol #%2d  El %3.1f #%2d VCP %3d\n"
                                   "Res %s",
                     hdr->g.prod_id, name_text, prod_mnemon, gp->prod_code, 
                     gp->vol_num, elev, gp->elev_ind, gp->vcp_num, res_text);
          } else {
             n_char = sprintf(buf, "%04d: %s\n"
                                   "%s  (%d)\n"
                                   "Vol #%2d  El #%2d  VCP %3d\n"
                                   "Res %s",
                      hdr->g.prod_id, name_text, prod_mnemon, gp->prod_code, 
                      gp->vol_num, get_elev_ind(sd->icd_product,orpg_build_i),
                      gp->vcp_num, res_text); 
          } /* end else an elevation product */
        } /*  end else an icd product */
        
   } /*  end else there is a pre_ICD header */

    if(n_char > BUF_LEN) {
        buf[BUF_LEN - 1] = '\0';
        fprintf(stderr,"ERROR Buffer Overrun in display_product_info(), "
                       "base_info_2 is %d characters\n", n_char);
    }

    xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(sd->base_info_2, XmNlabelString, xmstr, NULL);
    XmStringFree(xmstr);



/***********************************************************************/
   /* set the screen header string ... */
   /* volume date/time, VCP number, and Site ID (and type)*/ 
      
   if(pre_icd == FALSE) { /* no pre-ICD header */
   
       if(icd_format == FALSE) { /* not icd format and no header */
           sprintf(buf, "Product:            ID : Buffer\n"
                        "                   Name (PCode)\n"
                        "Not ICD - Date/Time UNK\n"
                        "Site: UNKNOWN");

       } else { /* icd format (and no header) */
          /* when cvg raw data is stuffed into icd format, */
          /* gp volume date/time and VCP are zero; Site UNK */
          if(gp->vol_date == 0) {
              n_char = sprintf(buf, "Product:          ID : Buffer\n"
                                    "                 Name (PCode)\n"
                                    "Not ICD - Date/Time UNK\n"
                                    "Site: UNKNOWN"); 
          } else {
              n_char = sprintf(buf, "Product:          ID : Buffer\n"
                                    "                 Name (PCode)\n"
                                    "%s\n"
                                    "Site: [%s] (%s)", 
                                       time_string, icao, radar_text);
          }

      } /*  end else icd format (and no header) */
      
   } else { /* there is a pre-ICD header */
   
       if(icd_format == FALSE) { /* not icd format (with header) */
          n_char = sprintf(buf, "Product:          ID : Buffer\n"
                                "                 Name (PCode)\n"
                                "%s\n"
                                "Site: UNKNOWN",
                                                        time_string);

       } else { /* icd format (with header) */
          /* when cvg raw data is stuffed into icd format,   */
          /* gp volume date/time and VCP are zero; Site UNK; */
          /*  can get volume time from Pre-ICD               */

          if(gp->vol_date == 0) {  /* a cvg raw product */
             n_char = sprintf(buf, "Product:          ID : Buffer\n"
                                   "                 Name (PCode)\n"
                                   "%s\n"
                                   "Site: UNKNOWN",
                                                        time_string); 
 
          } else { /* not a cvg raw product */
             n_char = sprintf(buf, "Product:          ID : Buffer\n"
                                   "                 Name (PCode)\n"
                                   "%s\n"
                                   "Site: [%s] (%s)",
                                       time_string, icao, radar_text);
          } /* end else not a cvg raw product */

       } /*  end else icd format (with header) */

   } /*  end else there is a pre-ICD header */

    if(n_char > BUF_LEN) {
        buf[BUF_LEN - 1] = '\0';
        fprintf(stderr,"ERROR Buffer Overrun in display_product_info(), "
                       "base_info is %d characters\n", n_char);
    }

    xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(sd->base_info, XmNlabelString, xmstr, NULL);
    XmStringFree(xmstr);



    /*   set new prod description label here   */

    xmstr = XmStringCreateLtoR(prod_desc, "largefont");
    XtVaSetValues(sd->prod_descript, XmNlabelString, xmstr, NULL);
    XmStringFree(xmstr);


} /*  end display_product_info */





 /******************************************************************************
  ******************************************************************************
  ******************************************************************************
  ******************************************************************************
  */
void add_or_page_gab(int screen)
{
    int i, j, index;
    Prod_header *hdr;
    int *msg_type;    

   if(screen == SCREEN_1)
       sd=sd1;

   else if(screen == SCREEN_2)
       sd=sd2;

   else if(screen == SCREEN_3)
       sd=sd3;       
       

    /* if nothing exists to be selected, nothing _can_ be selected or plotted */
    if(sd->layers == NULL) {
        fprintf(stderr,"\nGAB NOT DISPLAYED\n");
        fprintf(stderr,"The product contained no displayable layers\n\n");
        return;
    }

    sd->gab_page += 1; 

   
    /* If product not configured, do not display */
    hdr = (Prod_header *)(sd->icd_product);
    msg_type = assoc_access_i(msg_type_list, hdr->g.prod_id);
    if (msg_type == NULL) {
        fprintf(stderr,"\nCONFIGURATION ERROR (GAB Display)\n");
        fprintf(stderr,"This product has not been configured.\n");
        fprintf(stderr,"Enter preferences for product id %d\n", hdr->g.prod_id);
        fprintf(stderr,"using the CVG Site Specific Preferences Menu.\n\n");    
    }

    for(i=0;i<sd->num_layers;i++)
        for(j=0;j<sd->layers[i].num_packets;j++) {
            index = transfer_packet_code(sd->layers[i].codes[j]);
            if( index==GRAPHIC_ALPHA_BLOCK ) 
                dispatch_packet_type(index, sd->layers[i].offsets[j], TRUE);
        }      

    /* now, copy what's been plotted to the screen */
    if(screen == SCREEN_1)
        XCopyArea(XtDisplay(screen_1),sd1->pixmap,XtWindow(screen_1),gc,0,0,
                            427,102,0,0);
    else if(screen == SCREEN_2)
        XCopyArea(XtDisplay(screen_2),sd2->pixmap,XtWindow(screen_2),gc,0,0,
                            427,102,0,0);
    else if(screen == SCREEN_3)
        XCopyArea(XtDisplay(screen_3),sd3->pixmap,XtWindow(screen_3),gc,0,0,
                            427,102,0,0);
      
    if(compare_shell) {
       if((screen == SCREEN_1) && (compare_num == 1))
           display_compare_product();
       else if((screen == SCREEN_2) && (compare_num == 2))
           display_compare_product();       
    }   


    
} /*  end add_or_page_gab */





 /******************************************************************************
  ******************************************************************************
  ******************************************************************************
  ******************************************************************************
  */
void add_tab(int screen)
{

    int i, j, index;
    Prod_header *hdr;
    int *msg_type;

   if(screen == SCREEN_1)
       sd=sd1;

    else if(screen == SCREEN_2)
       sd=sd2;

    else if(screen == SCREEN_3)
       sd=sd3;
       
       /* if nothing exists to be selected, nothing can be selected or plotted */
    if(sd->layers == NULL) {
        fprintf(stderr,"\nTAB NOT DISPLAYED\n");
        fprintf(stderr,"The product contained no displayable layers\n\n");
        return;
    }
   
    /* If product not configured, do not display */
    hdr = (Prod_header *)(sd->icd_product);
    msg_type = assoc_access_i(msg_type_list, hdr->g.prod_id);

    if (msg_type == NULL) {
        fprintf(stderr,"\nCONFIGURATION ERROR (TAB Display)\n");
        fprintf(stderr,"This product has not been configured.\n");
        fprintf(stderr,"Enter preferences for product id %d\n", hdr->g.prod_id);
        fprintf(stderr,"using the CVG Site Specific Preferences Menu.\n\n"); 
        /* CVG 9.0 */ 
        return;  
    }

    for(i=0;i<sd->num_layers;i++)
        for(j=0;j<sd->layers[i].num_packets;j++) {
            index = transfer_packet_code(sd->layers[i].codes[j]);
            if( index==TABULAR_ALPHA_BLOCK )   
                index = transfer_packet_code(sd->layers[i].codes[j]);
            if( index==TABULAR_ALPHA_BLOCK ) 
                /* CVG 8.7 - added replay flag */
                dispatch_packet_type(index, sd->layers[i].offsets[j], TRUE);
        }      
    
    
} /*  end add_tab */


