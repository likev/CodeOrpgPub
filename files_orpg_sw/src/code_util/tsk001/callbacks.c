/* callbacks.c */

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/13 19:47:45 $
 * $Id: callbacks.c,v 1.18 2012/09/13 19:47:45 steves Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */

#include "callbacks.h"


int mouse_x, mouse_y = 0;



/* pops up an about box */
void about_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  char buf[400];
  Widget dialog = XmCreateMessageDialog(mainwin, "aboutd", NULL, 0);
  XmString xmstr;
  Widget rc;
  Pixmap mask;
/* Solaris 8 is weird here, for Solaris 8 the struct XpmAttributes_21 */
/* is the same as struct XpmAttributes in Solaris 7 and Linux         */
/* changed for LINUX */
/* also required change for cvg makefile */
#ifdef SUNOS
  XpmAttributes_21 attributes;
#endif
#ifdef LINUX
  XpmAttributes attributes;
#endif

  /* a little informational note */
  /* this provides the BUILD data and time */

  /*  Noblis */
  sprintf(buf, "%s\nBuild %s\n\n"

                "Provided to the NWS Office of Science & Technology by\n"
                "Noblis, Inc. of Falls Church Virginia\n\n"

                "NPI Development Manager (NWS/OS&T):\n"
                "Michael Istok, Michael.Istok@noaa.gov\n\n"

                "CVG Development Lead: Brian Klein\n\n"

                "Send bug reports / suggestions to: brian.klein@noaa.gov\n",
                               CVG_VERSION_STRING, VERSION_DATE_STRING);
  
  
  
  xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
  XtVaSetValues(dialog, XmNmessageString,      xmstr,
                XmNmessageAlignment,   XmALIGNMENT_CENTER,
                XmNbackground,         white_color,
                NULL);
  XmStringFree(xmstr);
  XtVaSetValues(XtParent(dialog), XmNtitle, "About CVG", NULL);
  XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
  XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));


  /* we want the Noblis logo flanked by the NWS and FAA logo */

  /* add logos in */
  rc = XmCreateRowColumn(dialog, "aboutrc", NULL, 0);
  XtVaSetValues(rc, XmNorientation,   XmHORIZONTAL,
            XmNspacing,       25,
            XmNbackground,    white_color,
            NULL);

  /* get basic attribute info */
  XtVaGetValues(dialog,
        XmNdepth,    &attributes.depth,
                XmNcolormap, &attributes.colormap,
        NULL);
  attributes.visual = DefaultVisual(XtDisplay(dialog), 
                                    DefaultScreen(XtDisplay(dialog)));
  attributes.valuemask = XpmDepth | XpmColormap | XpmVisual;

  /* set up label */
  faa_logo_label = XtVaCreateManagedWidget("faa_logo_draw",
     xmLabelWidgetClass, rc,
     XmNbackground, white_color,
     NULL);
  XpmCreatePixmapFromData(XtDisplay(faa_logo_label),
              DefaultRootWindow(XtDisplay(faa_logo_label)),
              faa_logo, &faa_logo_pix, &mask, &attributes);
  /* the mask isn't used, so if one is made, we can kill it */
  if(mask)
      XFreePixmap(XtDisplay(dialog), mask);
  /* set the pixmap for display on the label */
  XtVaSetValues(faa_logo_label,
        XmNlabelType,   XmPIXMAP,
        XmNlabelPixmap, faa_logo_pix,
        NULL);


  /* set up label */
  mts_logo_label = XtVaCreateManagedWidget("mts_logo_draw",
     xmLabelWidgetClass, rc,
     XmNbackground, white_color,
     NULL);
  XpmCreatePixmapFromData(XtDisplay(mts_logo_label),
              DefaultRootWindow(XtDisplay(mts_logo_label)),
              noblis_jpg_logo, &mts_logo_pix, &mask, &attributes);
  /* the mask isn't used, so if one is made, we can kill it */
  if(mask)
      XFreePixmap(XtDisplay(dialog), mask);
  /* set the pixmap for display on the label */
  XtVaSetValues(mts_logo_label,
        XmNlabelType,   XmPIXMAP,
        XmNlabelPixmap, mts_logo_pix,
        NULL);


  /* set up label */
  nws_logo_label = XtVaCreateManagedWidget("nws_logo_draw",
     xmLabelWidgetClass, rc,
     XmNbackground, white_color,
     NULL);
  XpmCreatePixmapFromData(XtDisplay(nws_logo_label),
              DefaultRootWindow(XtDisplay(nws_logo_label)),
              nws_logo, &nws_logo_pix, &mask, &attributes);
  /* the mask isn't used, so if one is made, we can kill it */
  if(mask)
      XFreePixmap(XtDisplay(dialog), mask);
  /* set the pixmap for display on the label */
  XtVaSetValues(nws_logo_label,
        XmNlabelType,   XmPIXMAP,
        XmNlabelPixmap, nws_logo_pix,
        NULL);

  XtManageChild(rc);
  XtManageChild(dialog);

} /* end about_callback() */



/* we need to clean up after all that pixmap making */
void about_delete_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

    XFreePixmap(XtDisplay(faa_logo_label),faa_logo_pix);
    XFreePixmap(XtDisplay(mts_logo_label),mts_logo_pix);
    XFreePixmap(XtDisplay(nws_logo_label),nws_logo_pix);
}


void orpg_build_callback(Widget w,XtPointer client_data, XtPointer call_data)
{
    
int *orpg_build = (int *)client_data;  

    orpg_build_i =  *orpg_build;
    write_orpg_build(orpg_build_i);
    /*  send signal to read_db child to get new orpg build */
    kill(new_pid, SIGUSR1); 
    
}





/* main expose callback used to show the pixmap to the draw widget */
void expose_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  int screen_num;
  if(w == screen_1)
    screen_num = 1;
  else if(w == screen_2)
    screen_num = 2;
  else if(w == screen_3)
    screen_num = 3;
  else
    return;

  show_pixmap(screen_num);
}



/* copies the offscreen copy of the "digital canvas" onto the canvas itself */
void show_pixmap(int screen_num)
{
  if(screen_num == SCREEN_1) {
      XCopyArea(XtDisplay(screen_1),sd1->pixmap,XtWindow(screen_1),gc,0,0,
        pwidth+barwidth,pheight,0,0);
  } else if(screen_num == 2) {
      XCopyArea(XtDisplay(screen_2),sd2->pixmap,XtWindow(screen_2),gc,0,0,
        pwidth+barwidth,pheight,0,0);
  } else if(screen_num == 3) {
      XCopyArea(XtDisplay(screen_3),sd3->pixmap,XtWindow(screen_3),gc,0,0,
        pwidth+barwidth,pheight,0,0);
  }


}




void clear_pixmap(int screen_num)
{
  /* clear the pixmap to black */
  if(screen_num == SCREEN_1) {
    if(screen_1 != NULL) {
      XSetForeground(XtDisplay(screen_1),gc,black_color);
      XFillRectangle(XtDisplay(screen_1),sd1->pixmap,gc,0,0,
                     pwidth+barwidth,pheight);
    }

  } else if(screen_num == SCREEN_2) {
    if(screen_2 != NULL) {
      XSetForeground(XtDisplay(screen_2),gc,black_color);
      XFillRectangle(XtDisplay(screen_2),sd2->pixmap,gc,0,0,
                     pwidth+barwidth,pheight);
    }

  } else if(screen_num == SCREEN_3) {
    if(screen_3 != NULL) {
      XSetForeground(XtDisplay(screen_3),gc,black_color);
      XFillRectangle(XtDisplay(screen_3),sd3->pixmap,gc,0,0,
                     pwidth+barwidth,pheight);
    }

  }


}





/* similar expose callback for the legend window */
void legend_expose_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  int screen_num;
  if(w == legend_screen_1)
    screen_num = 1;
  else if(w == legend_screen_2)
    screen_num = 2;
  else if(w == legend_screen_3)
    screen_num = 3;
  else
    return;

  legend_show_pixmap(screen_num);


}





/* copies the offscreen copy of the legend into the viewable area */
void legend_show_pixmap(int screen_num)
{
  if(screen_num == SCREEN_1) {
      XCopyArea(XtDisplay(legend_screen_1),sd1->legend_pixmap,
        XtWindow(legend_screen_1),gc,0,0,sidebarwidth,sidebarheight,0,0);
  } else if(screen_num == 2) {
      XCopyArea(XtDisplay(legend_screen_2),sd2->legend_pixmap,
        XtWindow(legend_screen_2),gc,0,0,sidebarwidth,sidebarheight,0,0);
  }else if(screen_num == 3) {
      XCopyArea(XtDisplay(legend_screen_3),sd3->legend_pixmap,
        XtWindow(legend_screen_3),gc,0,0,sidebarwidth,sidebarheight,0,0);
  }


}


/* copies area containing legend bars and labels */
void legend_copy_pixmap(int screen_num)
{
    
  if(screen_num == SCREEN_1) {
      XCopyArea(XtDisplay(screen_1), sd1->pixmap, sd1->legend_pixmap, gc,
        x_legend_start, y_legend_start, sidebarwidth-6, sidebarheight-6, 5, 5);
  } else if(screen_num == 2) {
      XCopyArea(XtDisplay(screen_2), sd2->pixmap, sd2->legend_pixmap, gc,
        x_legend_start, y_legend_start, sidebarwidth-6, sidebarheight-6, 5, 5);
  }else if(screen_num == 3) {
      XCopyArea(XtDisplay(screen_3), sd3->pixmap, sd3->legend_pixmap, gc,
        x_legend_start, y_legend_start, sidebarwidth-6, sidebarheight-6, 5, 5);
  }

    
}






void legend_clear_pixmap(int screen_num)
{
  /* clear the pixmap to black */
  if(screen_num == SCREEN_1) {
    if(screen_1 != NULL) {
      XSetForeground(XtDisplay(legend_screen_1),gc,black_color);
      XFillRectangle(XtDisplay(legend_screen_1),sd1->legend_pixmap,gc,0,0,
             sidebarwidth,sidebarheight);
    }
  } else if(screen_num == SCREEN_2) {
    if(screen_2 != NULL) {
      XSetForeground(XtDisplay(legend_screen_2),gc,black_color);
      XFillRectangle(XtDisplay(legend_screen_2),sd2->legend_pixmap,gc,0,0,
             sidebarwidth,sidebarheight);
    }
  } else if(screen_num == SCREEN_3) {
    if(screen_3 != NULL) {
      XSetForeground(XtDisplay(legend_screen_3),gc,black_color);
      XFillRectangle(XtDisplay(legend_screen_3),sd3->legend_pixmap,gc,0,0,
             sidebarwidth,sidebarheight);
    }
  }


} /* end legend_clear_pixmap */




/* clears the pixmap for a screen and wipes out all the data displayed there */


void clear_screen_callback(Widget w,XtPointer client_data, XtPointer call_data)
{
int screen=0;
    

    if(XtParent(XtParent(XtParent(XtParent(XtParent(w))))) == dshell1) {
        screen=SCREEN_1;           

    } else if(XtParent(XtParent(XtParent(XtParent(XtParent(w))))) == dshell2) {
        screen=SCREEN_2;        

    } else if(XtParent(XtParent(XtParent(XtParent(XtParent(w))))) == dshell3) {
        screen=SCREEN_3;        
    }

    clear_screen_data(screen, FALSE);
    
    
} /*  end clear_screen_callback */



/*----------------------------------------------------------------------------*/
void clear_screen_data(int screen, int plot_image) {
    
    XmString blank_xmstr, other_xmstr;
    
    int rv;

    XRectangle  clip_rectangle;


    blank_xmstr = XmStringCreateLtoR("", XmFONTLIST_DEFAULT_TAG);
    other_xmstr = XmStringCreateLtoR(" \n\n\n\n", XmFONTLIST_DEFAULT_TAG);





/**********************************************************/
    if( screen == SCREEN_1 ) {
        
      if(dshell1 != NULL) {
        /* first clean the screen */
        clear_pixmap(SCREEN_1);
        legend_clear_pixmap(SCREEN_1);
        XCopyArea(XtDisplay(screen_1),sd1->pixmap,XtWindow(screen_1),gc,0,0,
        pwidth+barwidth,pheight,0,0);
        XCopyArea(XtDisplay(legend_screen_1),sd1->legend_pixmap,
        XtWindow(legend_screen_1),gc,0,0,sidebarwidth,
        sidebarheight,0,0);

/* DEBUG */
/* fprintf(stderr,"DEBUG clear_screen_data - pixmaps freed\n"); */

        /* reset animation information */
        reset_time_series(SCREEN_1, ANIM_FULL_INIT);
        reset_elev_series(SCREEN_1, ANIM_FULL_INIT);
        reset_auto_update(SCREEN_1, ANIM_FULL_INIT);

        /*  NEED A DESTROY WIDGET HERE */
        /* close TAB window if open */
        if(sd1->tab_window != NULL) {
            XtDestroyWidget(sd1->tab_window);
        /*     close_tab_window(1); */
        }

/* DEBUG */
/* fprintf(stderr,"DEBUG clear_screen_data - tab window destroyed\n"); */

        /* get rid of any history info lying around */
        clear_history(&(sd1->history), &(sd1->history_size));
        /* now, clear both informational displays on the screen window */
        XtVaSetValues(sd1->data_display, XmNlabelString, blank_xmstr, NULL);
        XtVaSetValues(sd1->data_display_2, XmNlabelString, blank_xmstr, NULL);
        XtVaSetValues(sd1->base_info, XmNlabelString, blank_xmstr, NULL);

        XtVaSetValues(sd1->base_info_2, XmNlabelString, blank_xmstr, NULL);
        XtVaSetValues(sd1->prod_descript, XmNlabelString, blank_xmstr, NULL);
        
        if(sd1->icd_product != NULL) {
            free(sd1->icd_product);
            sd1->icd_product = NULL;
        }
    /*  GENERIC_PROD */
        if((sd1->generic_prod_data != NULL)) {
            rv = cvg_RPGP_product_free((void *)sd1->generic_prod_data);
            sd1->generic_prod_data = NULL;
        }   
        sd1->packet_28_offset = 0;
    
        if(sd1->layers != NULL) {
            delete_layer_info(sd1->layers, sd1->num_layers);
            sd1->layers = NULL;
        }

/* DEBUG */
/* fprintf(stderr,"DEBUG clear_screen_data - layers freed\n"); */

        /* recenter the image */
        sd1->x_center_offset = 0;
        sd1->y_center_offset = 0;
    
    
    /*  why is this line here?, because the delete functions need it! */
        sd = sd1;
        
        /* delete any extra info lying around */
        if(sd1->last_image == RASTER_IMAGE)
            delete_raster();
        else if(sd1->last_image == RLE_IMAGE)
            delete_radial_rle();
        else if(sd1->last_image == DIGITAL_IMAGE)
            delete_packet_16();
        else if(sd1->last_image == PRECIP_ARRAY_IMAGE)
            delete_packet_17();
        else if(sd1->last_image == GENERIC_RADIAL)
            delete_generic_radial();
            
        sd1->last_image = NO_IMAGE;
        sd1->this_image = NO_IMAGE;

/* DEBUG */
/* fprintf(stderr,"DEBUG clear_screen_data - image data freed\n"); */

        if(plot_image == FALSE) {
           clip_rectangle.x       = 0;
           clip_rectangle.y       = 0;
           clip_rectangle.width   = pwidth;
           clip_rectangle.height  = pheight;
           XSetClipRectangles(display, gc, 0, 0, &clip_rectangle, 1, Unsorted);
           if(map_flag1 == TRUE)
                display_map(SCREEN_1);
           if(az_line_flag1 == TRUE)
                draw_az_lines(SCREEN_1);
           if(range_ring_flag1 == TRUE)
                draw_range_rings(SCREEN_1);
           XCopyArea(XtDisplay(screen_1),sd1->pixmap,XtWindow(screen_1),gc,0,0,
                               pwidth+barwidth,pheight,0,0);
         } /*  end plot_image FALSE */
                 
      } /*  end if dshell1 not null */

      if(compare_shell)  /* test fails if both screens are not open */     
        if(compare_num == 1)
           display_compare_product();
      
    } /*  end if screen1 */


    /**********************************************************/
    if( screen == SCREEN_2 ) {
      if(dshell2 != NULL) {
        /* first clean the screen */
        clear_pixmap(SCREEN_2);
        legend_clear_pixmap(SCREEN_2);
        XCopyArea(XtDisplay(screen_2),sd2->pixmap,XtWindow(screen_2),gc,0,0,
              pwidth+barwidth,pheight,0,0);
        XCopyArea(XtDisplay(legend_screen_2),sd2->legend_pixmap,
              XtWindow(legend_screen_2),gc,0,0,sidebarwidth,
              sidebarheight,0,0);

        /* reset animation information */
        reset_time_series(SCREEN_2, ANIM_FULL_INIT);
        reset_elev_series(SCREEN_2, ANIM_FULL_INIT);
        reset_auto_update(SCREEN_2, ANIM_FULL_INIT);

        /*  NEED A DESTROY WIDGET HERE */
        /* close TAB window if open */
        if(sd2->tab_window != NULL) {
            XtDestroyWidget(sd2->tab_window);
        }
              
        /* get rid of any history info lying around */
        clear_history(&(sd2->history), &(sd2->history_size));
        /* now, clear both informational displays on the screen window */
        XtVaSetValues(sd2->data_display, XmNlabelString, blank_xmstr, NULL);
        XtVaSetValues(sd2->data_display_2, XmNlabelString, blank_xmstr, NULL);
        XtVaSetValues(sd2->base_info, XmNlabelString, blank_xmstr, NULL);

        XtVaSetValues(sd2->base_info_2, XmNlabelString, blank_xmstr, NULL);
        XtVaSetValues(sd2->prod_descript, XmNlabelString, blank_xmstr, NULL);
        
        if(sd2->icd_product != NULL) {
            free(sd2->icd_product);
            sd2->icd_product = NULL;
        }
/*  GENERIC_PROD */
        if((sd2->generic_prod_data != NULL)) {
            rv = cvg_RPGP_product_free((void *)sd2->generic_prod_data);
            sd2->generic_prod_data = NULL;
        }   
        sd2->packet_28_offset = 0;
        
            if(sd2->layers != NULL) {
                delete_layer_info(sd2->layers, sd2->num_layers);
                sd2->layers = NULL;
        }

        /* recenter the image */
        sd2->x_center_offset = 0;
        sd2->y_center_offset = 0;
        
    /*  why is this line here? because the delete functions need it! */
        sd = sd2;
        
        /* delete any extra info lying around */
        if(sd2->last_image == RASTER_IMAGE)
            delete_raster();
        else if(sd2->last_image == RLE_IMAGE)
            delete_radial_rle();
        else if(sd2->last_image == DIGITAL_IMAGE)
            delete_packet_16();
        else if(sd2->last_image == PRECIP_ARRAY_IMAGE)
            delete_packet_17();
        else if(sd2->last_image == GENERIC_RADIAL)
            delete_generic_radial();
            
        sd2->last_image = NO_IMAGE;
        sd2->this_image = NO_IMAGE;

        if(plot_image == FALSE) {        
            clip_rectangle.x       = 0;
            clip_rectangle.y       = 0;
            clip_rectangle.width   = pwidth;
            clip_rectangle.height  = pheight;
            XSetClipRectangles(display, gc, 0, 0, &clip_rectangle, 1, Unsorted);
            if(map_flag2 == TRUE)
                 display_map(SCREEN_2);
            if(az_line_flag2 == TRUE)
                 draw_az_lines(SCREEN_2);
            if(range_ring_flag2 == TRUE)
                 draw_range_rings(SCREEN_2);
            XCopyArea(XtDisplay(screen_2),sd2->pixmap,XtWindow(screen_2),gc,0,0,
                                pwidth+barwidth,pheight,0,0);
        } /*  end plot_image FALSE */
        
        
      }/*  end dshell not null */

      if(compare_shell)  /* test fails if both screens are not open */     
        if(compare_num == 2)
           display_compare_product();
      
    }  /*  end if screen2    */



    /**********************************************************/
    if( screen == SCREEN_3 ) {
      if(dshell3 != NULL) {
        /* first clean the screen */
        clear_pixmap(SCREEN_3);
        legend_clear_pixmap(SCREEN_3);
        XCopyArea(XtDisplay(screen_3),sd3->pixmap,XtWindow(screen_3),gc,0,0,
              pwidth+barwidth,pheight,0,0);
        XCopyArea(XtDisplay(legend_screen_3),sd3->legend_pixmap,
              XtWindow(legend_screen_3),gc,0,0,sidebarwidth,
              sidebarheight,0,0);

        /* reset animation information */
        reset_time_series(SCREEN_3, ANIM_FULL_INIT);
        reset_elev_series(SCREEN_3, ANIM_FULL_INIT);
        reset_auto_update(SCREEN_3, ANIM_FULL_INIT);

        /*  NEED A DESTROY WIDGET HERE */
        /* close TAB window if open */
        if(sd3->tab_window != NULL) {
            XtDestroyWidget(sd3->tab_window);
        /*     close_tab_window(2); */
        }
              
        /* get rid of any history info lying around */
        clear_history(&(sd3->history), &(sd3->history_size));
        /* now, clear both informational displays on the screen window */
        XtVaSetValues(sd3->data_display, XmNlabelString, blank_xmstr, NULL);
        XtVaSetValues(sd3->data_display_2, XmNlabelString, blank_xmstr, NULL);
        XtVaSetValues(sd3->base_info, XmNlabelString, blank_xmstr, NULL);

        XtVaSetValues(sd3->base_info_2, XmNlabelString, blank_xmstr, NULL);
        XtVaSetValues(sd3->prod_descript, XmNlabelString, blank_xmstr, NULL);
        
        if(sd3->icd_product != NULL) {
            free(sd3->icd_product);
            sd3->icd_product = NULL;
        }
/*  GENERIC_PROD */
        if((sd3->generic_prod_data != NULL)) {
            rv = cvg_RPGP_product_free((void *)sd3->generic_prod_data);
            sd3->generic_prod_data = NULL;
        }   
        sd3->packet_28_offset = 0;
        
            if(sd3->layers != NULL) {
                delete_layer_info(sd3->layers, sd3->num_layers);
                sd3->layers = NULL;
        }

        /* recenter the image */
        sd3->x_center_offset = 0;
        sd3->y_center_offset = 0;
        
    /*  why is this line here? because the delete functions need it! */
        sd = sd3;
        
        /* delete any extra info lying around */
        if(sd3->last_image == RASTER_IMAGE)
            delete_raster();
        else if(sd3->last_image == RLE_IMAGE)
            delete_radial_rle();
        else if(sd3->last_image == DIGITAL_IMAGE)
            delete_packet_16();
        else if(sd3->last_image == PRECIP_ARRAY_IMAGE)
            delete_packet_17();
        else if(sd3->last_image == GENERIC_RADIAL)
            delete_generic_radial();

        sd3->last_image = NO_IMAGE;
        sd3->this_image = NO_IMAGE;

        if(plot_image == FALSE) {        
            clip_rectangle.x       = 0;
            clip_rectangle.y       = 0;
            clip_rectangle.width   = pwidth;
            clip_rectangle.height  = pheight;
            XSetClipRectangles(display, gc, 0, 0, &clip_rectangle, 1, Unsorted);
            if(map_flag3 == TRUE)
                 display_map(SCREEN_3);
            if(az_line_flag3 == TRUE)
                 draw_az_lines(SCREEN_3);
            if(range_ring_flag3 == TRUE)
                 draw_range_rings(SCREEN_3);
            XCopyArea(XtDisplay(screen_3),sd3->pixmap,XtWindow(screen_3),gc,0,0,
                                pwidth+barwidth,pheight,0,0);
        } /*  end plot_image FALSE */
        
        
      }/*  end dshell not null */

      if(compare_shell)  /* test fails if both screens are not open */     
        if(compare_num == 2)
           display_compare_product();
      
    }  /*  end if screen3    */


    
} /*  end clear_screen_data */

/*----------------------------------------------------------------------------*/



/* callback to exit the program */
void exit_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
   int i;
    
     if (xmstr_prodb_list != NULL) {        

           for(i=0; i<last_prod_list_size; i++) 
               XmStringFree(xmstr_prodb_list[i]);
           XtFree((char *)xmstr_prodb_list); 
           xmstr_prodb_list=NULL;        
     }
       
     if(prod_list_msg != NULL)
         free(prod_list_msg);
     prod_list_msg = NULL;
       
    kill(new_pid, SIGTERM);
    
    XtDestroyWidget(shell);
    exit(0);
}




/* plot the current selections again */
/* this function is called from the following in callbacks.c
 *   the 8 display attribute callback functions 
 *   the zoom_callback function
 *   the img_center_meny_cb function
 *   the img_large_callback and img_small_callback functions
 *   the morm_format_Callback and bkgd_format_Callback functions
 *   the replot_image_callback function
 * called from the following in prefs.c
 *   the map_ok_callback function
 * called from the folowing in click.c
 *   the do_click_info function
 */
void replot_image(int screen_num)
{

    XRectangle  clip_rectangle;
    
/* shortcut to execute if the screen is clear */

        clip_rectangle.x       = 0;
        clip_rectangle.y       = 0;
        clip_rectangle.width   = pwidth;
        clip_rectangle.height  = pheight;

        if( screen_num == SCREEN_1 && sd1->icd_product == NULL ) { 
            clear_pixmap(SCREEN_1);
            legend_clear_pixmap(SCREEN_1);
            XSetClipRectangles(display, gc, 0, 0, &clip_rectangle, 1, Unsorted);
            if(map_flag1 == TRUE)
                 display_map(SCREEN_1);
            if(az_line_flag1 == TRUE)
                 draw_az_lines(SCREEN_1);
            if(range_ring_flag1 == TRUE)
                 draw_range_rings(SCREEN_1);
            XCopyArea(XtDisplay(screen_1),sd1->pixmap,XtWindow(screen_1),gc,0,0,
                                pwidth+barwidth,pheight,0,0);
            return;
     
        } else if( screen_num == SCREEN_2 && sd1->icd_product == NULL ) { 
            clear_pixmap(SCREEN_2);
            legend_clear_pixmap(SCREEN_2);
            XSetClipRectangles(display, gc, 0, 0, &clip_rectangle, 1, Unsorted);
            if(map_flag2 == TRUE)
                 display_map(SCREEN_2);
            if(az_line_flag2 == TRUE)
                 draw_az_lines(SCREEN_2);
            if(range_ring_flag2 == TRUE)
                 draw_range_rings(SCREEN_2);
            XCopyArea(XtDisplay(screen_2),sd2->pixmap,XtWindow(screen_2),gc,0,0,
                                pwidth+barwidth,pheight,0,0);
            return;
                                
        } else if( screen_num == SCREEN_3 && sd1->icd_product == NULL ) { 
            clear_pixmap(SCREEN_3);
            legend_clear_pixmap(SCREEN_3);
            XSetClipRectangles(display, gc, 0, 0, &clip_rectangle, 1, Unsorted);
            if(map_flag3 == TRUE)
                 display_map(SCREEN_3);
            if(az_line_flag3 == TRUE)
                 draw_az_lines(SCREEN_3);
            if(range_ring_flag3 == TRUE)
                 draw_range_rings(SCREEN_3);
            XCopyArea(XtDisplay(screen_3),sd3->pixmap,XtWindow(screen_3),gc,0,0,
                                pwidth+barwidth,pheight,0,0);
            return;
        }

/*     } // end if icd_product NULL */
    

    /* replot everything that's been plotted since the last time the 
     * screen was cleared */
    if(screen_num == SCREEN_1) {
        if(screen_1 != NULL) {
            sd = sd1;
            replay_history(sd1->history, sd1->history_size, SCREEN_1);
        }

    } else if(screen_num == SCREEN_2) {
        if(screen_2 != NULL) {
            sd = sd2;
            replay_history(sd2->history, sd2->history_size, SCREEN_2);
        }

    } else if(screen_num == SCREEN_3) {
        if(screen_3 != NULL) {
            sd = sd3;
            replay_history(sd3->history, sd3->history_size, SCREEN_3);
        }

    }


} /* replot_image */




/* This function overrides opening the packet selection dialog
 * and selects all packets
 */
void select_all_graphic_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  /* set the overlay flag depending on toggle state */
  XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct*)call_data;

  if(cbs->set) {
    select_all_flag = TRUE;

    XtVaSetValues(overlay_but,
        XmNsensitive,     True,
        XmNset,           XmSET,
    NULL);
    
    XtVaSetValues(packet_button,
        XmNsensitive,     False,
    NULL);
    
  } else {
    select_all_flag = FALSE;

    XtVaSetValues(overlay_but,
        XmNsensitive,     False,
        XmNset,           XmUNSET,
    NULL);    

    XtVaSetValues(packet_button,
        XmNsensitive,     True,
    NULL);
    
  }
  
       
}







/* this function sets the overlay_flag based upon the condition of check box
 * there are three conditions where the overlay logic should not be accomplished.
 *   1. When there is no product previously displayed.
 *      Overlaying over nothing causes a display problem in plot_image()
 *   2. When either the current product and the already displayed product
 *      is not configured as a geographic product.  The overlay would
 *      have no meaning so is not allowed.
 */
void overlayflag_callback(Widget w,XtPointer client_data,XtPointer call_data)
{
    
 
  /* set the overlay flag depending on toggle state */
  XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct*)call_data;

  if(cbs->set) {  /*  overlay check box is set */

        check_overlay(FALSE); /* do not permit overlay of non-geographic content */
        
  } else /*  overlay check box is not set */
  
    overlay_flag = FALSE;
    
  }



/* if called from within the packet selection dialog, that is from the function    */
/* select_layer_or_packet(), the parameter is TRUE, at all other times it is FALSE */
void check_overlay(int overlay_non_geographic)
{
    
Prod_header *hdr;
int *loaded_msg_type, *displayed_msg_type;    
screen_data *osd = NULL;  

      overlay_flag = TRUE;

      if(selected_screen==SCREEN_1)
          osd = sd1;
      else if(selected_screen==SCREEN_2)
          osd = sd2;
      else if(selected_screen==SCREEN_3)
          osd = sd3;

      
      /* 1. if nothing already displayed, DO NOT OVERLAY */
      if(osd->history == NULL) {
          overlay_flag = FALSE;
          
      } else { /*  else there is someting already displayed */
    
          hdr = (Prod_header *)(sd->icd_product);
          loaded_msg_type = assoc_access_i(msg_type_list, hdr->g.prod_id);   
          hdr = (Prod_header *)(sd->history[0].icd_product);
          displayed_msg_type = assoc_access_i(msg_type_list, hdr->g.prod_id); 
          
          /* if called from the packet select dialog, we wish to overlay */
          /* on top of anything for product analysis, otherwise          */
          /* 2. if displayed product or this product not configured */ 
          /*    as a geographic type, DO NOT OVERLAY                */
          if(overlay_non_geographic == FALSE) {
              if( (*loaded_msg_type != GEOGRAPHIC_PRODUCT) || 
                  (*displayed_msg_type != GEOGRAPHIC_PRODUCT) ) {
                  overlay_flag = FALSE;
              }
          /* CVG 9.0 - added this case to support analysis from the */
          /*             packet selection dialog                      */
          } else if(overlay_non_geographic == TRUE) {
              if( (*loaded_msg_type != *displayed_msg_type) ) {
                  overlay_flag = FALSE;
              }
               
          } /* end overlay_non_geographic == TRUE */ 
          
          /* 3. If current product contains radial / raster data, */
          /*    DO NOT OVERLAY                                    */
          if(osd->this_image != NO_IMAGE) {
              overlay_flag = FALSE;
          }
          
      } /*  end something already displayed */
          

    
} /* end check_overlay */






/* sets an indicator for the current active screen */
void screen1_radio_callback(Widget w, XtPointer client_data, 
                            XtPointer call_data)
{
    selected_screen = SCREEN_1;
}


void screen2_radio_callback(Widget w, XtPointer client_data, 
                            XtPointer call_data)
{
    selected_screen = SCREEN_2;
}


void screen3_radio_callback(Widget w, XtPointer client_data, 
                            XtPointer call_data)
{
    selected_screen = SCREEN_3;
}



void prodid_filter_callback(Widget w, XtPointer client_data, XtPointer call_data) {
    
    prod_filter = FILTER_PROD_ID;
      
} 





void type_filter_callback(Widget w, XtPointer client_data, XtPointer call_data) {
    
    prod_filter = FILTER_MNEMONIC;
    
} 


void pcode_filter_callback(Widget w, XtPointer client_data, XtPointer call_data) {
    
    prod_filter = FILTER_P_CODE;    
    
}



/***************************************************************************/
/* The following callbacks are from widgets on the product display screens 
 * Passing a sting type screen number with client data has problems on Solaris.
 *
 * An alternative to to check the identity of the parent shell of a widget
 *
 *
 *
 * TWO LEVELS (w on the winform)
 * 
 * if(XtParent(XtParent(w)) == dshell1) {
 * if(XtParent(XtParent(w)) == dshell2) {
 * 
 * for all animation conrols except mode select
 * for gab page
 * 
 * 
 * THREE LEVELS 
 * if(XtParent(XtParent(XtParent(w))) == dshell1) {
 * if(XtParent(XtParent(XtParent(w))) == dshell2) {
 * 
 * for: (buttons in radiobox)
 * 
 * norm_format_Callback (buttons in radiobox)
 * bkgd_format_Callback
 * 
 * 
 *
 * FOUR LEVELS 
 * if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
 * if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
 *
 * zoom_callback (buttons in option menu)
 *
 * select_time_callback (buttons in option menu)
 * select_elev_callback
 * select_update_callback
 * select_file_callback
 *
 * all disp_<attr>_callback (buttons in option menu)
 *
 * img_large_callback (buttons in option menu)
 * img_small_callback
 *
****************************************************************************/






void disp_none_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
       range_ring_flag1 = az_line_flag1 = map_flag1 = FALSE; 
       replot_image(SCREEN_1);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
       range_ring_flag2 = az_line_flag2 = map_flag2 = FALSE; 
       replot_image(SCREEN_2);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
       range_ring_flag3 = az_line_flag3 = map_flag3 = FALSE; 
       replot_image(SCREEN_3);
    }   


}




void disp_r_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
       range_ring_flag1 = TRUE;
       az_line_flag1 = map_flag1 = FALSE; 
       replot_image(SCREEN_1);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
       range_ring_flag2 = TRUE;
       az_line_flag2 = map_flag2 = FALSE; 
       replot_image(SCREEN_2);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
       range_ring_flag3 = TRUE;
       az_line_flag3 = map_flag3 = FALSE; 
       replot_image(SCREEN_3);
    }    


}




void disp_a_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
       az_line_flag1 = TRUE;
       range_ring_flag1 = map_flag1 = FALSE; 
       replot_image(SCREEN_1);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
       az_line_flag2 = TRUE;
       range_ring_flag2 = map_flag2 = FALSE; 
       replot_image(SCREEN_2);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
       az_line_flag3 = TRUE;
       range_ring_flag3 = map_flag3 = FALSE; 
       replot_image(SCREEN_3);
    }    


}




void disp_ra_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
       range_ring_flag1 = az_line_flag1 = TRUE;
       map_flag1 = FALSE; 
       replot_image(SCREEN_1);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
       range_ring_flag2 = az_line_flag2 = TRUE;
       map_flag2 = FALSE; 
       replot_image(SCREEN_2);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
       range_ring_flag3 = az_line_flag3 = TRUE;
       map_flag3 = FALSE; 
       replot_image(SCREEN_3);
    }    


}




void disp_m_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
       map_flag1 = TRUE;
       range_ring_flag1 = az_line_flag1 = FALSE; 
       replot_image(SCREEN_1);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
       map_flag2 = TRUE;
       range_ring_flag2 = az_line_flag2 = FALSE; 
       replot_image(SCREEN_2);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
       map_flag3 = TRUE;
       range_ring_flag3 = az_line_flag3 = FALSE; 
       replot_image(SCREEN_3);

    }   


}




void disp_ma_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
       az_line_flag1 = map_flag1 = TRUE;
       range_ring_flag1 = FALSE; 
       replot_image(SCREEN_1);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
       az_line_flag2 = map_flag2 = TRUE;
       range_ring_flag2 = FALSE; 
       replot_image(SCREEN_2);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
       az_line_flag3 = map_flag3 = TRUE;
       range_ring_flag3 = FALSE; 
       replot_image(SCREEN_3);
    }    


}




void disp_mr_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
       range_ring_flag1 = map_flag1 = TRUE;
       az_line_flag1 = FALSE; 
       replot_image(SCREEN_1);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
       range_ring_flag2 = map_flag2 = TRUE;
       az_line_flag2 = FALSE; 
       replot_image(SCREEN_2);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
       range_ring_flag3 = map_flag3 = TRUE;
       az_line_flag3 = FALSE; 
       replot_image(SCREEN_3);
    }   


}




void disp_all_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
       range_ring_flag1 = az_line_flag1 = map_flag1 = TRUE; 
       replot_image(SCREEN_1);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
       range_ring_flag2 = az_line_flag2 = map_flag2 = TRUE; 
       replot_image(SCREEN_2);
    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
       range_ring_flag3 = az_line_flag3 = map_flag3 = TRUE; 
       replot_image(SCREEN_3);
    }     


}



/* CVG 9.0 */
void calculate_small_screen()
{

/*  the hard-coded values 240 (related to height) and 115 (related to width) */
/*  should be calculated from the initial size of the pshell widget.  This */
/*  is accomplished by creating a StructureNotifyMask event handler to read */
/*  the initial size.  See example 20-2 in the Motif Programming Manual (6A) */

  screen1x = 20;  
  screen1y = screen2y = 45;
  height = width = 500;   /*  product display portal */
  screen2x = disp_width - spacexy - width - 115;
  /*  screen 3 position is not changed */


} /* end calculate_small_screen */


/*  The size of the product display window is changed by setting the size of  */
/*  the product display portal (the scrollable area widget) and also the  */
/*  legend frame widget and then allowing the window to resize itself.   */

void sm_win_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

  /* CVG 9.0 */
  calculate_small_screen();
  


/* CVG 9.0 */
  fprintf(stderr," Selecting small screen, image portal is %d x %d\n", 
                                                                width, height);
                                                                
  if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1)   {
    
      XtVaSetValues(dshell1,
          XmNallowShellResize,     TRUE,
      NULL);

      XtVaSetValues(port1,
          XmNwidth,    width,
          XmNheight,   height,
      NULL);

      XtVaSetValues(legend_frame_1,
/*           XmNwidth,    sidebarwidth, */
          XmNheight,   height+75,
      NULL); 

      XtVaSetValues(dshell1,
          XmNx,         screen1x,
          XmNy,         screen1y, 
          XmNallowShellResize,     FALSE,
      NULL);  

      center_screen(vbar1, hbar1);

      XCopyArea(XtDisplay(screen_1),sd1->pixmap,XtWindow(screen_1),gc,0,0,
                            pwidth+barwidth,pheight,0,0);
  
  } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
      
      XtVaSetValues(dshell2,
          XmNallowShellResize,     TRUE,
      NULL); 

      XtVaSetValues(port2,
          XmNwidth,    width,
          XmNheight,   height,
      NULL);  

      XtVaSetValues(legend_frame_2,
          XmNheight,   height+75,
      NULL); 
           
      XtVaSetValues(dshell2,
          XmNx,         screen2x,
          XmNy,         screen2y, 
          XmNallowShellResize,     FALSE,
      NULL);

      center_screen(vbar2, hbar2);

      XCopyArea(XtDisplay(screen_2),sd2->pixmap,XtWindow(screen_2),gc,0,0,
                            pwidth+barwidth,pheight,0,0);
 
  
  } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
      
      XtVaSetValues(dshell3,
          XmNallowShellResize,     TRUE,
      NULL); 

      XtVaSetValues(port3,
          XmNwidth,    width,
          XmNheight,   height,
      NULL);  

      XtVaSetValues(legend_frame_3,
          XmNheight,   height+75,
      NULL); 
           
      XtVaSetValues(dshell3,
          XmNx,         screen3x,
          XmNy,         screen3y, 
          XmNallowShellResize,     FALSE,
      NULL);

      center_screen(vbar3, hbar3);

      XCopyArea(XtDisplay(screen_3),sd3->pixmap,XtWindow(screen_3),gc,0,0,
                            pwidth+barwidth,pheight,0,0);
 
  
  }



}


/* CVG 9.0 */
void calculate_large_screen()
{

/*  the hard-coded values 240 (related to height) and 115 (related to width) */
/*  should be calculated from the initial size of the pshell widget.  This */
/*  is accomplished by creating a StructureNotifyMask event handler to read */
/*  the initial size.  See example 20-2 in the Motif Programming Manual (6A) */

  screen1x = 20;    
  screen1y = screen2y = 45;
  height = width = disp_height - screen1y - 240;  /*  product display portal */
  screen2x = disp_width - spacexy - width - 115;
  /*  screen 3 position is not changed */


} /* end calculate_large_screen */


void lg_win_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

int leg_frame_hgt;

/*  do nothing for a 1024x768 display screen */
  if(disp_height <= SMALL_IMG) {
      return;
  }

  /* CVG 9.0 */
  calculate_large_screen();
  
  
  /* CVG 9.0 */
  fprintf(stderr," Selecting large screen, image portal is %d x %d\n", 
                                                                width, height);
  /* prevent legend frame from being larger than legend pix */
  leg_frame_hgt = height + 75;
  if(leg_frame_hgt > sidebarheight)
    leg_frame_hgt = sidebarheight;

  if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1)   {
              
      XtVaSetValues(dshell1,
          XmNallowShellResize,     TRUE,
      NULL);

      XtVaSetValues(port1,
          XmNwidth,    width,
          XmNheight,   height,
      NULL);
      
      XtVaSetValues(legend_frame_1,
          XmNheight,   leg_frame_hgt,
      NULL);     

      XtVaSetValues(dshell1,
          XmNx,         screen1x,
          XmNy,         screen1y, 
          XmNallowShellResize,     FALSE,
      NULL);  

      center_screen(vbar1, hbar1);

      XCopyArea(XtDisplay(screen_1),sd1->pixmap,XtWindow(screen_1),gc,0,0,
                            pwidth+barwidth,pheight,0,0);
  
  } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
            
      XtVaSetValues(dshell2,
          XmNallowShellResize,     TRUE,
      NULL); 

      XtVaSetValues(port2,
          XmNwidth,    width,
          XmNheight,   height,
      NULL);  

      XtVaSetValues(legend_frame_2,
          XmNheight,   leg_frame_hgt,
      NULL); 
           
      XtVaSetValues(dshell2,
          XmNx,         screen2x,
          XmNy,         screen2y, 
          XmNallowShellResize,     FALSE,
      NULL);

      center_screen(vbar2, hbar2);

      XCopyArea(XtDisplay(screen_2),sd2->pixmap,XtWindow(screen_2),gc,0,0,
                            pwidth+barwidth,pheight,0,0);
  
  } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
            
      XtVaSetValues(dshell3,
          XmNallowShellResize,     TRUE,
      NULL); 

      XtVaSetValues(port3,
          XmNwidth,    width,
          XmNheight,   height,
      NULL);  

      XtVaSetValues(legend_frame_3,
          XmNheight,   leg_frame_hgt,
      NULL); 
           
      XtVaSetValues(dshell3,
          XmNx,         screen3x,
          XmNy,         screen3y, 
          XmNallowShellResize,     FALSE,
      NULL);

      center_screen(vbar3, hbar3);

      XCopyArea(XtDisplay(screen_3),sd3->pixmap,XtWindow(screen_3),gc,0,0,
                            pwidth+barwidth,pheight,0,0);
  
  }



}




/* sets what is the current scale factor for plotting, and makes
 * the changes show up immediately
 */
void zoom_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    float *zoom_selected = (float *)client_data;  

    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
        sd1->scale_factor = *zoom_selected;
        replot_image(SCREEN_1);

    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
        sd2->scale_factor = *zoom_selected;
        replot_image(SCREEN_2);

    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
        sd3->scale_factor = *zoom_selected;
        replot_image(SCREEN_3);

    }


} 
 







/*****************************************************/
/* action taken when the third mouse button is pressed as
 * the XmNpopupHandlerCallback for the drawing area widget
 *
 * 1. records the mouse pointer location
 *
 * 2. sets menu buttons to insensitive if no product displayed
 *    or if the product type is not geographic
 */
void draw_menu_callback(Widget w, XtPointer client_data, 
                                                  XtPointer call_data)
{
  
  XmPopupHandlerCallbackStruct *cbs =
                            (XmPopupHandlerCallbackStruct *) call_data;
  
  XButtonPressedEvent *bp = (XButtonPressedEvent *) cbs->event;

  screen_data *my_sd=NULL;
  Prod_header *hdr;
  Graphic_product *gp;
  int *type_ptr, msg_type = -1;
  
  mouse_x = bp->x;
  mouse_y = bp->y;  
/* test */
/* fprintf(stderr,"TEST mouse position recorded x is %d, Y is %d\n", */
/*                       mouse_x, mouse_y);          */
  

  if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
      my_sd = sd1;
      XtSetSensitive(radar_center1, True);
      XtSetSensitive(mouse_center1, True);
       
  } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
      my_sd = sd2;
      XtSetSensitive(radar_center2, True);
      XtSetSensitive(mouse_center2, True);
      
  } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
      my_sd = sd3;
      XtSetSensitive(radar_center3, True);
      XtSetSensitive(mouse_center3, True);
  }    


  /* must be configured as a Geographic Product */
  if(my_sd->icd_product != NULL) {
      hdr = (Prod_header *)(my_sd->icd_product);
      gp = (Graphic_product *) (my_sd->icd_product+ 96);
      type_ptr = assoc_access_i(msg_type_list, hdr->g.prod_id);
      msg_type = *type_ptr;
  }

  if( (my_sd->icd_product == NULL) || 
      (msg_type != GEOGRAPHIC_PRODUCT) ) {
        
        if(my_sd == sd1) {
            XtSetSensitive(radar_center1, False);
            XtSetSensitive(mouse_center1, False);
            
        } else if(my_sd == sd2) {
            XtSetSensitive(radar_center2, False);
            XtSetSensitive(mouse_center2, False);
            
        } else if(my_sd == sd3) {
            XtSetSensitive(radar_center3, False);
            XtSetSensitive(mouse_center3, False);
        }
        
  }

    
}





/*****************************************************/
/* invoked when an item in the popup menu is selected
 *
 * either sets the radar as the center or the mouse
 * pointer obtained from draw_menu_callback as the center
 *
 * The calculated offset is in units of 
 * "number of pixels / zoom factor" in order to keep the
 * same geographic location centered when changing
 * the zoom factor.
 */
void img_center_menu_cb(Widget item, XtPointer client_data, XtPointer call_data)
{

  int screen=0;
  int item_no = (int) client_data;


/* test */
/* fprintf(stderr, "TEST XtParent(XtParent(XtParent(XtParent(XtParent(XtParent(item)))))) is %s\n", */
/*             XtName( XtParent(XtParent(XtParent(XtParent(XtParent(XtParent(item)))))) )  );  */
/*  */
/* fprintf(stderr, "TEST XtParent(XtParent(XtParent(XtParent(XtParent(XtParent(XtParent(item))))))) is %s\n", */
/*             XtName( XtParent(XtParent(XtParent(XtParent(XtParent(XtParent(XtParent(item))))))) )  );                   */
/* end test */

/* DEBUG */
/* fprintf(stderr,"DEBUG center menu - item number is %d\n", item_no); */

  /* /////////////////////////////////////////////////////////////////////// */
  if(item_no == 0 || item_no == 1) { /*  one of the image center buttons pressed */
    /*  change is applied to the screen activated by the user and to a linked screen */
    if( 
        (XtParent(XtParent(XtParent(XtParent(XtParent(XtParent(XtParent(item))))))) 
                                                                     == dshell1) ) {
      screen = SCREEN_1;
      do_image_center(mouse_x, mouse_y, item_no, screen);
      replot_image(screen);
      
      if( (dshell2 != NULL) && (linked_flag==TRUE) ) {
        screen = SCREEN_2;
        do_image_center(mouse_x, mouse_y, item_no, screen);
        replot_image(screen);
      
      }
      
    }
    
    if( 
        (XtParent(XtParent(XtParent(XtParent(XtParent(XtParent(XtParent(item))))))) 
                                                                     == dshell2) ) {
      screen = SCREEN_2;
      do_image_center(mouse_x, mouse_y, item_no, screen);
      replot_image(screen);

      if( (dshell1 != NULL) && (linked_flag==TRUE) ) {
        screen = SCREEN_1;
        do_image_center(mouse_x, mouse_y, item_no, screen);
        replot_image(screen);
      
      }
      
    }
    
    if( 
        (XtParent(XtParent(XtParent(XtParent(XtParent(XtParent(XtParent(item))))))) 
                                                                     == dshell3) ) {
      screen = SCREEN_3;
      do_image_center(mouse_x, mouse_y, item_no, screen);
      replot_image(screen);
    }

  /* /////////////////////////////////////////////////////////////////////////// */
  } else if(item_no == 2) {  /*  the show / hide center location icon button pressed */
    /*  change is applied to all open screens */
    
    if(ctr_loc_icon_visible_flag == TRUE)
        ctr_loc_icon_visible_flag = FALSE;
    else
        ctr_loc_icon_visible_flag = TRUE;
        
    if(screen_1 != NULL) {
        if(ctr_loc_icon_visible_flag == TRUE)
            XtVaSetValues( hide_ctr_loc_but1, XmNlabelString, hide_xmstr, NULL);
        else
            XtVaSetValues( hide_ctr_loc_but1, XmNlabelString, show_xmstr, NULL);
        screen = SCREEN_1;
        replot_image(screen);
    }
    
    if(screen_2 != NULL) {
        if(ctr_loc_icon_visible_flag == TRUE)
            XtVaSetValues( hide_ctr_loc_but2, XmNlabelString, hide_xmstr, NULL);
        else
            XtVaSetValues( hide_ctr_loc_but2, XmNlabelString, show_xmstr, NULL);
        screen = SCREEN_2;
        replot_image(screen);
        
    }
    
    if(screen_3 != NULL) {
        if(ctr_loc_icon_visible_flag == TRUE)
            XtVaSetValues( hide_ctr_loc_but3, XmNlabelString, hide_xmstr, NULL);
        else
            XtVaSetValues( hide_ctr_loc_but3, XmNlabelString, show_xmstr, NULL);
        screen = SCREEN_3;
        replot_image(screen);
        
    }
    
  }
  

}






void do_image_center(int mouse_xin, int mouse_yin, int but_num, int screen)
{

  int x_offset, prev_x;
  int y_offset, prev_y;
  screen_data *my_sd=NULL;
  
  
  if(screen == SCREEN_1) {
       my_sd = sd1;

  } else if(screen == SCREEN_2) {
       my_sd = sd2;

  } else if(screen == SCREEN_3) {
       my_sd = sd3;
  }
  
  
  
  /* previous pixel offset (if any) */
  prev_x = my_sd->x_center_offset * my_sd->scale_factor;
  prev_y = my_sd->y_center_offset * my_sd->scale_factor;
  
  /* pixel offset of selected point */  
  x_offset = mouse_xin - (pwidth/2) + prev_x;
  y_offset = mouse_yin - (pheight/2) + prev_y; 
  /* relative distance offset to retain geographic center when changing zoom */
  x_offset = x_offset / my_sd->scale_factor;
  y_offset = y_offset / my_sd->scale_factor;


/* test */
/* fprintf(stderr, "TEST button number is %d\n", but_num); */
/* fprintf(stderr, "TEST xmouse is %d, pwidth is %d, offset is %d\n", */
/*            mouse_xin, pwidth, x_offset); */
/* fprintf(stderr, "TEST ymouse is %d, pheight is %d, offset is %d\n", */
/*            mouse_yin, pheight, y_offset);            */

/* end test// */


  if(but_num ==0) { /*  radar is the center */
    my_sd->x_center_offset = 0;
    my_sd->y_center_offset = 0;
    
  } else if(but_num == 1) { /*  mouse pointer is the center */
    my_sd->x_center_offset = x_offset;
    my_sd->y_center_offset = y_offset;
  }

/* TEST */
/* fprintf(stderr,"TEST SCR 1 xoff = %d, yoff = %d, ZOOM is %f\n", sd1->x_center_offset, */
/*                                sd1->y_center_offset, sd1->scale_factor); */
/* fprintf(stderr,"TEST SCR 2 xoff = %d, yoff = %d, ZOOM is %f\n", sd2->x_center_offset, */
/*                                sd2->y_center_offset, sd2->scale_factor);     */
/* fprintf(stderr,"TEST SCR 3 xoff = %d, yoff = %d, ZOOM is %f\n", sd3->x_center_offset, */
/*                                sd3->y_center_offset, sd3->scale_factor); */
/*  */
    
}












/* callback to center the image canvas (drawing ares) */
void center_image_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    
if(XtParent(XtParent(XtParent(XtParent(XtParent(w))))) == dshell1) {
        center_screen(vbar1, hbar1);

    } else if(XtParent(XtParent(XtParent(XtParent(XtParent(w))))) == dshell2) {
        center_screen(vbar2, hbar2);

    } else if(XtParent(XtParent(XtParent(XtParent(XtParent(w))))) == dshell3) {
        center_screen(vbar3, hbar3);

    }    
    
    
}




void img_large_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

    /* CVG 9.0 */
    large_image_flag = TRUE;

    /* CVG 9.0 - use global constant */
    pwidth = pheight = img_size = LARGE_IMG;
    if(screen_1 != NULL) {
        XtVaSetValues(img_size_opt1, 
            XmNmenuHistory,      img_sizebutnormal1, 
            NULL);
        reset_screen_size(SCREEN_1); 
        replot_image(SCREEN_1);
    }
        
    if(screen_2 != NULL) {
        XtVaSetValues(img_size_opt2, 
            XmNmenuHistory,      img_sizebutnormal2, 
            NULL);
        reset_screen_size(SCREEN_2);
        replot_image(SCREEN_2);
    }   

    if(screen_3 != NULL) {
        XtVaSetValues(img_size_opt3, 
            XmNmenuHistory,      img_sizebutnormal3, 
            NULL);
        reset_screen_size(SCREEN_3);
        replot_image(SCREEN_3);
    }        

}



void img_small_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

   /* CVG 9.0 */
    large_image_flag = FALSE;

    /* CVG 9.0 - changed small size from 760 to 768 (1.5 times PUP screen0 */
    pwidth = pheight = img_size = SMALL_IMG;
    if(screen_1 != NULL) {
        XtVaSetValues(img_size_opt1, 
            XmNmenuHistory,      img_sizebut1, 
            NULL);
        reset_screen_size(SCREEN_1);
        replot_image(SCREEN_1);
    }
        
    if(screen_2 != NULL) {
        XtVaSetValues(img_size_opt2, 
            XmNmenuHistory,      img_sizebut2, 
            NULL);
        reset_screen_size(SCREEN_2);
        replot_image(SCREEN_2);
    }

    if(screen_3 != NULL) {
        XtVaSetValues(img_size_opt3, 
            XmNmenuHistory,      img_sizebut3, 
            NULL);
        reset_screen_size(SCREEN_3);
        replot_image(SCREEN_3);
    }    

}



void linked_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
unsigned char button_state;

     /*  the auxiliary screen (screen3) is not linked */

    if(XtParent(XtParent(w)) == dshell1) {
                
        XtVaGetValues(linked_toggle1, XmNset, &button_state, NULL);
        
        if(button_state==True)
            linked_flag = TRUE;           
        else 
            linked_flag = FALSE;            
        
        if(screen_2 != NULL)
            XtVaSetValues(linked_toggle2, 
                XmNset, button_state, 
                NULL);         


    } else if(XtParent(XtParent(w)) == dshell2) {
    
        XtVaGetValues(linked_toggle2, XmNset, &button_state, NULL);
        
        if(button_state==True)
            linked_flag = TRUE;
        else
            linked_flag = FALSE; 
             
        if(screen_1 != NULL) 
            XtVaSetValues(linked_toggle1, 
                XmNset, button_state, 
                NULL);
        
    }       
    
}



void norm_format_Callback(Widget w, XtPointer client_data, XtPointer call_data)
{


    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
        norm_format1 = TRUE;
        bkgd_format1 = FALSE;
        replot_image(SCREEN_1);

    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
        norm_format2 = TRUE;
        bkgd_format2 = FALSE;
        replot_image(SCREEN_2);

    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
        norm_format3 = TRUE;
        bkgd_format3 = FALSE;
        replot_image(SCREEN_3);

    }

}




void bkgd_format_Callback(Widget w, XtPointer client_data, XtPointer call_data)
{


    if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
        norm_format1 = FALSE;
        bkgd_format1 = TRUE;
        replot_image(SCREEN_1);

    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
        norm_format2 = FALSE;
        bkgd_format2 = TRUE;
        replot_image(SCREEN_2);

    } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
        norm_format3 = FALSE;
        bkgd_format3 = TRUE;
        replot_image(SCREEN_3);

    }

}




/* the scroll window handles normal scrolling, so all we need these callbacks
 * for is to handle linked scrolling
 */
void vert_scroll_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Dimension size;
  
  /*  the auxiliary screen (screen3) is not linked */
  
  if(XtParent(XtParent(XtParent(w))) == dshell1) {

    /* if they're linked, moving the first screen can control the second */
    if( (linked_flag==TRUE) && (dshell2 != NULL) ) {
      scroll2_ypos = ((XmScrollBarCallbackStruct *)call_data)->value;
      /* cap it so that we don't move the image so far that it goes offscreen */
      XtVaGetValues(port2, XmNheight, &size, NULL);
      if(scroll2_ypos > pheight - size)
        scroll2_ypos = pheight - size;
      XtVaSetValues(vbar2, XmNvalue, scroll2_ypos, NULL);
      scroll2_ypos = -scroll2_ypos;
      XtMoveWidget(screen_2, scroll2_xpos, scroll2_ypos);
    }
  }

}



void horiz_scroll_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Dimension size;
  
  /*  the auxiliary screen (screen3) is not linked */
  
  if(XtParent(XtParent(XtParent(w))) == dshell1) {

    /* if they're linked, moving the first screen can control the second */
    if( (linked_flag==TRUE) && (dshell2 != NULL) ) {
      scroll2_xpos = ((XmScrollBarCallbackStruct *)call_data)->value;
      /* cap it so that we don't move the image so far that it goes offscreen */
      XtVaGetValues(port2, XmNwidth, &size, NULL);
      if(scroll2_xpos > pwidth + barwidth - size)
        scroll2_xpos = pwidth + barwidth - size;
      XtVaSetValues(hbar2, XmNvalue, scroll2_xpos, NULL);
      scroll2_xpos = -scroll2_xpos;
      XtMoveWidget(screen_2, scroll2_xpos, scroll2_ypos);
    }
  }

}






void gab_page_up_callback(Widget w,XtPointer client_data, XtPointer call_data)
{
     
    if(XtParent(XtParent(w)) == dshell1) {

            add_or_page_gab(SCREEN_1);
        
    } else if(XtParent(XtParent(w)) == dshell2) {

             add_or_page_gab(SCREEN_2);
        
    }else if(XtParent(XtParent(w)) == dshell3) {

             add_or_page_gab(SCREEN_3);
        
    } 



}




void display_tab_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    
    
    if(XtParent(XtParent(w)) == dshell1) {

        add_tab(SCREEN_1);    
                 
    } else if(XtParent(XtParent(w)) == dshell2) {

        add_tab(SCREEN_2);          

    } else if(XtParent(XtParent(w)) == dshell3) {

        add_tab(SCREEN_3);          
    }

    
}
 






void replot_image_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

    int screen=0;   

    if(XtParent(XtParent(XtParent(XtParent(XtParent(w))))) == dshell1) {
        screen=SCREEN_1;           

    } else if(XtParent(XtParent(XtParent(XtParent(XtParent(w))))) == dshell2) {
        screen=SCREEN_2;        

    } else if(XtParent(XtParent(XtParent(XtParent(XtParent(w))))) == dshell3) {
        screen=SCREEN_3;        

    }


    replot_image(screen);
 
    
}
 




