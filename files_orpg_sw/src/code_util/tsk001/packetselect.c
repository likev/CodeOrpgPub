/* packetselect.c */

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:51 $
 * $Id: packetselect.c,v 1.13 2009/05/15 17:52:51 ccalvert Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */
 
#include "packetselect.h"


/* CVG 9.0 - MOVED TO FILE GLOBAL */
Widget overbut;


/* callback when the packet selection button has been clicked */
void display_packetselect_callback(Widget w, XtPointer client_data, XtPointer call_data) 
{
  packet_selection_menu(w, client_data, call_data);
}




void packet_selection_menu(Widget w, XtPointer client_data, XtPointer call_data) 
{
   Widget dialog, form, d;
   Widget again_box;
   Widget area_comp_but, table_comp_but;
   int i,j;
   /* CVG 9.0 - change name for clarity */
   int list_size;

   unsigned char is_set;

   XmString xmstr;
   XmString *xmstr_packetselect;
   static char *helpfile = HELP_FILE_PACKET_SELECT;

  Prod_header *hdr;
  Graphic_product *gp;

/* DEBUG */
/* fprintf(stderr,"DEBUG - ENTERING packet_selection_menu()\n"); */


    hdr = (Prod_header *)(sd->icd_product);
    gp  = (Graphic_product *)(sd->icd_product + 96);

    
  /* set up the main active screen */    
    if(selected_screen == SCREEN_1) {
        sd = sd1;       
    }
    
    if(selected_screen == SCREEN_2) {
        sd = sd2;
    }

    if(selected_screen == SCREEN_3) {
        sd = sd3;
    }
    

   /* make sure that a icd product has been loaded and parsed */
   if(sd->icd_product == NULL) { 
     d = XmCreateErrorDialog(shell, "Error", NULL, 0);
     xmstr = XmStringCreateLtoR("A valid product has not been loaded for display",
                XmFONTLIST_DEFAULT_TAG);
     XtVaSetValues(XtParent(d), XmNtitle, "Error", NULL);
     XtVaSetValues(d, XmNmessageString, xmstr, NULL);
     XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
     XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
     XtManageChild(d);
     XmStringFree(xmstr);
     return;
   }


   if(sd->num_layers == 0) { 
     d = XmCreateInformationDialog(shell, "NOTE", NULL, 0);
     xmstr = XmStringCreateLtoR("The selected product has no data for display "
                                "(no Sym Block, no GAB, no TAB).",
                XmFONTLIST_DEFAULT_TAG);
     XtVaSetValues(XtParent(d), XmNtitle, "NOTE", NULL);
     XtVaSetValues(d, XmNmessageString, xmstr, NULL);
     XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
     XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
     XtManageChild(d);
     XmStringFree(xmstr);
     
     return;
   }
 

/*  BY=PASS HERE //////////////////////////////////////////////////////////// */
/*  THIS WORKS FOR THE NON-PROBLEM PRODUCTS USING select_all_packets */
/*  select_layer_or_packet DOES NOT WORK (for DPA) without opening */
/*  the dilog  */
   if(select_all_flag==TRUE) {
    
         /* overlay_but is on the main menu */
         XtVaGetValues(overlay_but,
             XmNset,    &is_set,
             NULL);
         
         if(is_set == XmSET) {   
            check_overlay(FALSE); /* do not overlay non-geographic content */

         } else {
            overlay_flag = FALSE;

         }

       /*  Fixed overlay issues with 56 USP, 57 DHR, and 108 DPA */
       /*          DPA still requires special handling due to multiple  */
       /*          2D array packets        */
       if(
           gp->prod_code == 81) {
            
             /*  Must open the dialog for DPA */
           
       } else {
           
           select_all_packets(NULL, NULL, NULL);
 
           return;          
       }  


    } /*  end if select_all_flag */
/* END BY-PASS HERE ///////////////////////////////////////////////////////// */


   if(verbose_flag)
       fprintf(stderr,"inside packet selection menu: num of layers=%u\n", 
                                                                  sd->num_layers);


   /* allocate memory for the composite string list */
   list_size=0;
   for(i=0; i<sd->num_layers; i++) {
      list_size += sd->layers[i].num_packets;
/* fprintf(stderr,"DEBUG num packets in layer %d is %d\n", */
/*                              i+1, sd->layers[i].num_packets + 1);     */
   }
   list_size += sd->num_layers;
/* DEBUG */
/*    fprintf(stderr,"DEBUG total number of list entries (layers + packets is %d\n", */
/*                                         list_size); */

   xmstr_packetselect = (XmString*)XtMalloc(sizeof(XmString) * list_size);




   list_index=0;  /* keep track of where we are in the output array */
   
   /* BUILD THE LIST OF LAYERS AND PACKETS FROM THE LAYER INFO SCREEN DATA */
   for(i=0; i<sd->num_layers; i++) {
       char entry_string[100];
       int index;
  
       if(verbose_flag)
           printf("current layer=%i\n",i+1);

       sprintf(entry_string,
           "%04d: Layer %3i      -----------------------------------------",
                                                   list_index,  i+1);
       xmstr_packetselect[list_index] = 
           XmStringCreateLtoR(entry_string, XmFONTLIST_DEFAULT_TAG);
       list_index++;

       if(verbose_flag)
           printf("store %s\n",entry_string);
    
       for(j=0; j<sd->layers[i].num_packets;j++) {
           char secondary_descript[51];
           /* create a string to be placed into the list */
           index = transfer_packet_code(sd->layers[i].codes[j]);
    
           if(sd->layers[i].codes[j]==0x0802 || sd->layers[i].codes[j]==0x0E03 ||
              sd->layers[i].codes[j]==0x3501 || sd->layers[i].codes[j]==0xAF1F ||
              sd->layers[i].codes[j]==0xBA07 || sd->layers[i].codes[j]==0xBA0F ) {
               sprintf(entry_string,
                       "%04d:       Item %03i:  Traditional Packet %4Xx - %s", 
                       list_index, j+1, sd->layers[i].codes[j], 
                       packet_name[index]);
           
           } else if(sd->layers[i].codes[j]==2801 || 
                     sd->layers[i].codes[j]==2802 ||
                     sd->layers[i].codes[j]==2803 || 
                     sd->layers[i].codes[j]==2804 ||
                     sd->layers[i].codes[j]==2805 || 
                     sd->layers[i].codes[j]==2806 ) {
               get_component_subtype(secondary_descript, sd->layers[i].codes[j], 
                                                       sd->layers[i].offsets[j] );
               sprintf(entry_string,
                       "%04d:       Item %03i:  Generic Component  28:%d  - %s %s", 
                   list_index, j+1, sd->layers[i].codes[j]-2800, 
                   packet_name[index], secondary_descript);
           
           } else
               sprintf(entry_string,
                       "%04d:       Item %03i:  Traditional Packet %4u  - %s", 
                       list_index, j+1, sd->layers[i].codes[j], 
                       packet_name[index]);
    
           if(verbose_flag)
               printf("store %s\n",entry_string);
           
           xmstr_packetselect[list_index] = 
             XmStringCreateLtoR(entry_string,XmFONTLIST_DEFAULT_TAG);
           list_index++;
       } /*  end for num packets */
       
   } /*  end for num layers */

   if(verbose_flag)
       fprintf(stderr, "done creating list\n");

   /* CREATE AND OPEN THE PACKET SELECTION DIALOG WINDOW */

   xmstr =  XmStringCreateLtoR("Product Packets", XmFONTLIST_DEFAULT_TAG);
   dialog = XmCreateSelectionDialog(shell, "packet_displayd", NULL, 0);
   XtVaSetValues(dialog,
         XmNlistItems,        xmstr_packetselect,
         XmNlistItemCount,    list_index,
         XmNvisibleItemCount, 12,
         XmNdialogStyle,      XmDIALOG_PRIMARY_APPLICATION_MODAL,
         XmNlistLabelString,  xmstr,
         XmNmarginHeight,     0,
         XmNmarginWidth,      0,
         NULL);
   XmStringFree(xmstr);

   XtVaSetValues(XtParent(dialog), 
         XmNtitle,     sel_prod_buf,
         NULL);    


   /* default select first item in list */
   XmListSelectPos(XmSelectionBoxGetChild(dialog, XmDIALOG_LIST), 1, True);


   if(verbose_flag)
       printf("  total number of packets and layers = %d\n",  list_index);
   overlay_flag = FALSE;

   /* add a toggle to say whether we want an overlay */

  form = XtVaCreateManagedWidget("form", xmFormWidgetClass, dialog,
     NULL);

   overbut = XtVaCreateManagedWidget("Overlay Selected Packets / Components", 
      xmToggleButtonWidgetClass, form, 
      XmNtopAttachment,      XmATTACH_FORM,
      XmNtopOffset,          5,
      XmNleftAttachment,   XmATTACH_FORM,
      XmNleftOffset,       185,
      XmNrightAttachment,   XmATTACH_FORM,
      XmNrightOffset,       185,
      NULL);

   /* CVG 9.0 - make a change to not use, instead test for is_set */
   XtAddCallback(overbut, XmNvalueChangedCallback, overlayflag_callback, NULL);



    area_comp_but = XtVaCreateManagedWidget("Area Comp Display Options...",
       xmPushButtonWidgetClass, form,
       XmNtopAttachment,       XmATTACH_WIDGET,
       XmNtopWidget,           overbut,
       XmNtopOffset,           10,
       XmNleftAttachment,      XmATTACH_FORM,
       XmNleftOffset,          122,
       XmNbottomAttachment,    XmATTACH_FORM,
       XmNbottomOffset,        10,
       XmNrightAttachment,     XmATTACH_NONE,
       XmNwidth,               170,
       XmNheight,              25,
       NULL);
    XtAddCallback(area_comp_but, XmNactivateCallback, 
                                             area_comp_opt_window_callback, NULL);




    table_comp_but = XtVaCreateManagedWidget("Table Comp Display Options...",
       xmPushButtonWidgetClass, form,
       XmNtopAttachment,       XmATTACH_WIDGET,
       XmNtopWidget,           overbut,
       XmNtopOffset,           10,
       XmNleftAttachment,      XmATTACH_WIDGET,
       XmNleftWidget,          area_comp_but,
       XmNleftOffset,          30,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNrightAttachment,     XmATTACH_NONE,
       
       XmNwidth,               170,
       XmNheight,              25,
       NULL);
    XtAddCallback(table_comp_but, XmNactivateCallback, 
                                           table_comp_opt_window_callback, NULL);

    XtSetSensitive(table_comp_but, False);





   /* get rid of buttons 'n' stuff we don't want */
   XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_SELECTION_LABEL));
   XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_TEXT));

   XtAddCallback(dialog, XmNokCallback, packetselectall_callback, NULL);
   
   XtAddCallback(dialog, XmNapplyCallback, packetselection_Callback, NULL);

   XtAddCallback(dialog, XmNhelpCallback, help_window_callback, helpfile);


   XtVaSetValues(dialog,
          XmNokLabelString,     
               XmStringCreateLtoR("OK (Select All)", XmFONTLIST_DEFAULT_TAG),
          XmNapplyLabelString,     
               XmStringCreateLtoR("Single Layer/Packet", XmFONTLIST_DEFAULT_TAG),
          NULL);
/*  is there a problem (minor memory leak) here? */
   
   XtManageChild(dialog);
   packetsel_dialog = dialog;


   
   /* free allocated memory for the xmstring */
   for(i=0; i<list_index; i++)
       XmStringFree(xmstr_packetselect[i]);
   
   XtFree((char*)xmstr_packetselect);
   
/* DEBUG */
/* fprintf(stderr,"DEBUG packet_selection_menu() - " */
/*                "number of list entry strings freed is %d\n", */
/*                                           list_index+1); */
   
    
    /*  Fixed overlay issues with 56 USP, 57 DHR, and 108 DPA */
    /*          DPA still requires special handling due to multiple  */
    /*          2D array packets */
    if(

       gp->prod_code == 81) {   


        
        if((gp->prod_code == 81) && (dpa_info_flag == TRUE)) {
            xmstr = XmStringCreateLtoR(
              "The DPA product requires special attention.  Because of the      \n"
              "nature of the product, each layer / packet must be displayed     \n"
              "individually. The 'Select All' function is ignored.            \n\n"
              "DPA is considered a non-geographic product because an LFM        \n"
              "projection is not used when displayed. The 'Overlay Packet'      \n"
              "selection is ignored when either the previously displayed product\n"
              "or the product being displayed is configured as non-geographic.  \n",
              XmFONTLIST_DEFAULT_TAG);

            d = XmCreateInformationDialog(dialog, "NOTE", NULL, 0);
            XtVaSetValues(XtParent(d), XmNtitle, "NOTE", NULL);
            XtVaSetValues(d, XmNmessageString, xmstr, NULL);
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
            XtManageChild(d);
   
            again_box = XtVaCreateManagedWidget("Do Not Show This Message Again "\
                                                "                               "\
                                                "                               ", 
                 xmToggleButtonWidgetClass,  d,
                 XmNset,                     XmUNSET,
                 NULL);
            XtAddCallback(again_box, XmNvalueChangedCallback, 
                                                         dpa_not_again_cb, NULL);
            
            XmStringFree(xmstr);
         
        } /*  end second if product_code 81 && dpa_info_flag */
        
    } /*  end first if product_code 81 */


     
}  /*  end packet_selection_menu */




 
void dpa_not_again_cb(Widget w,XtPointer client_data, XtPointer call_data)
{
    
    XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct *) call_data;
    
    if(cbs->set == XmSET) {        
        dpa_info_flag = FALSE;
/* fprintf(stderr,"DEBUG - DPA REPEAT INFO BUTTON IS SET\n");         */
    } else
        dpa_info_flag = TRUE;
    
}



/* The callbacks that implement the buttons in the packet select window
 * are responsible for setting global variables specifying what type of
 * selection was made and the exact items selected, as well as opening
 * the specified window (if it's not open), and plotting the selected
 * packets or whatever into it
 */

/* callback when a packet has been selected from the popup window */
/* since the dialog selection is returned as a string, we have to parse the string
 * to figure out what they selected */
void packetselection_Callback(Widget w,XtPointer client_data, XtPointer call_data) 
{

    select_layer_or_packet(w, client_data, call_data);

}




void select_layer_or_packet(Widget w,XtPointer client_data, XtPointer call_data)
{

    XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *)call_data;

/* in the future, get the position with list_cbs->item_position */
/* XmListCallbackStruct *list_cbs; */

    int i, pos;
    char *cbuf;
    
    /*CVG 9.0 */
    int j, packet;
    unsigned char is_set;

  Prod_header *hdr;
  Graphic_product *gp;
  int *type_ptr, msg_type;


    printf("-> inside select_layer_or_packet\n");

    /* get rid of the parent shell */
    /* required because not the OK button */
    XtPopdown(XtParent(packetsel_dialog));



/* DETERMINE THE LAYER SELECTED OR THE INDIVIDUAL PACKET SELECTED FOR DISPLAY */

    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &cbuf);

    /* get the position that was selected */
    sscanf(cbuf, "%d", &pos);

/* DEBUG */
/*printf(" DEBUG - store index selected from packet list=%d\n", pos); */
/*printf(" DEBUG - number of layers to check=%d\n", sd->num_layers);  */


/* DEBUG */
/* fprintf(stderr,"DEBUG select_layer_or_packet - looking for layer selected\n");  */

    /* find if a layer or a packet was selected, 
     * and set the selection values accordingly */
    /* CVG 9.0 */
    sd->packet_select_type = SELECT_NONE;
    sd->layer_select = 0;
    
    for(i=0; i<sd->num_layers;i++) {

        if(sd->layers[i].index == pos) {
            sd->packet_select_type = SELECT_LAYER;
            sd->layer_select = i;
            printf("Selected Layer %d\n", sd->layer_select+1); 
            break;
            
        }

    } /*  end for num layers */

    
    /* if we didn't select a layer, we need to figure out which packet
     * in which layer to select */
    if(sd->packet_select_type == SELECT_NONE) {

        sd->packet_select_type = SELECT_PACKET;
        
        /* CVG 9.0 - new logic for determining layer and packet within */
        /*             a layer when select_type is SELECT_PACKET */
/*            sd->packet_select = pos - sd->layers[sd->layer_select].index - 1; */
        for(i=0; i<sd->num_layers;i++) {
            
            if(pos > sd->layers[i].index) {
                 sd->layer_select = i;
                 sd->packet_select = pos - sd->layers[i].index -1;
            }
            
            if(pos < sd->layers[i].index)
                break;
            
        } /* end for num layers */
        
        printf("Selected Packet %d from Layer %d\n", 
                    sd->packet_select+1, sd->layer_select+1);

    } /* end if still SELECT_NONE */


/* CVG 9.0 - added this section to correctly determine the value of overlay_flag */
/* DETERMINE IF THE PACKET SELECTED IS OR THE LAYER SELECTED CONTAINS AN IMAGE */
/* AND SET OVERLAY_FLAG ACCORDINGLY */

    sd->this_image = NO_IMAGE;  /* used by check_overlay() */

    if(sd->packet_select_type == SELECT_PACKET) {
        packet = transfer_packet_code( 
                         sd->layers[sd->layer_select].codes[sd->packet_select] );
        switch(packet) {
            case DIGITAL_RADIAL_DATA_ARRAY: /* 16 */
                sd->this_image = DIGITAL_IMAGE;
                break;
            case DIGITAL_PRECIP_DATA_ARRAY: /* 17 */
                sd->this_image = PRECIP_ARRAY_IMAGE;
                break;
            case PRECIP_RATE_DATA_ARRAY:    /* 18 */
                sd->this_image = OTHER_2D_IMAGE;
                break;
            case GENERIC_RADIAL_DATA:       
                sd->this_image = GENERIC_RADIAL;
                break;
            case GENERIC_GRID_DATA:         
                sd->this_image = GENERIC_GRID;
                break;
            case RADIAL_DATA_16_LEVELS:      
                sd->this_image = RLE_IMAGE;
                break;
            case RASTER_DATA_7:
            case RASTER_DATA_F:              
                sd->this_image = RASTER_IMAGE;
                break;
            default:
                break;
        } /* end sitch */
    
    } else if(sd->packet_select_type == SELECT_LAYER) {
        for(j=0;j<sd->layers[sd->layer_select].num_packets;j++) {
            packet = transfer_packet_code(sd->layers[sd->layer_select].codes[j]);
            switch(packet) {
                case DIGITAL_RADIAL_DATA_ARRAY: /* 16 */
                    sd->this_image = DIGITAL_IMAGE;
                    break;
                case DIGITAL_PRECIP_DATA_ARRAY: /* 17 */
                    sd->this_image = PRECIP_ARRAY_IMAGE;
                    break;
                case PRECIP_RATE_DATA_ARRAY:    /* 18 */
                    sd->this_image = OTHER_2D_IMAGE;
                    break;
                case GENERIC_RADIAL_DATA:       
                    sd->this_image = GENERIC_RADIAL;
                    break;
                case GENERIC_GRID_DATA:         
                    sd->this_image = GENERIC_GRID;
                    break;
                case RADIAL_DATA_16_LEVELS:      
                    sd->this_image = RLE_IMAGE;
                    break;
                case RASTER_DATA_7:
                case RASTER_DATA_F:              
                    sd->this_image = RASTER_IMAGE;
                    break;
                default:
                    break;
            } /* end sitch */
            
        } /*  end for num_packets */
    
    }

    /* overbut is on the packet selection dialog */
    XtVaGetValues(overbut,
        XmNset,    &is_set,
        NULL);
    
    if(is_set == XmSET) {   
       check_overlay(TRUE); /* permit overlay of non-geographic content */

    } else {
       overlay_flag = FALSE;

    }




/* DISPLAY SELECTED PACKETS BY CALLNG PLOT_IMAGE FOR APPROPRIATE SCREEN */

    /* if there is a GAB, display it from the beginning */
    sd->gab_page = 1;

/* DEBUG */
/* if(sd->icd_product == NULL) */
/*     fprintf(stderr,"DEBUG - select_layer_or_packet - product not loaded\n"); */

    hdr = (Prod_header *)(sd->icd_product);
    gp = (Graphic_product *) (sd->icd_product+ 96);
    type_ptr = assoc_access_i(msg_type_list, hdr->g.prod_id);
/* DEBUG */
/* if(type_ptr == NULL) */
/* fprintf(stderr,"DEBUG - select_layer_or_packet - msg type not set for id %d\n", */
/*                   hdr->g.prod_id); */

    msg_type = *type_ptr;

    /* if it's not already open, open a screen to display stuff on */
    if(selected_screen == SCREEN_1) {
        
        if(screen_1 == NULL) 
            open_display_screen(SCREEN_1);
/* TEST - Will the window manager allow us to force the existing */
/*                window to the top?  Many do not. Are we lucky?????? */
        else {
/* fprintf(stderr,"TEST RAISING WINDOW 1\n"); */
           XRaiseWindow( XtDisplay(dshell1), XtWindow(dshell1) );
        }

        if(overlay_flag == FALSE)
             set_prod_resolution(selected_screen);

        /* if not a geographic product reset screen center */
        if(msg_type != GEOGRAPHIC_PRODUCT
           || overlay_flag == FALSE     /*  IS THIS DESIRABLE??? */
                                        ) {
            sd1->x_center_offset = 0;
            sd1->y_center_offset = 0;
        }

        /* and display product */
        plot_image(SCREEN_1, TRUE);
       
        sd1->last_plotted = TRUE;
        
    } /* end SCREEN_1 */
    
    
    if(selected_screen == SCREEN_2) {
        
        if(screen_2 == NULL) 
            open_display_screen(SCREEN_2);
/* TEST - Will the window manager allow us to force the existing */
/*                window to the top?  Many do not. Are we lucky?????? */
        else {
/* fprintf(stderr,"TEST RAISING WINDOW 2\n"); */
           XRaiseWindow( XtDisplay(dshell2), XtWindow(dshell2) );
        }

        if(overlay_flag == FALSE)
             set_prod_resolution(selected_screen);

        /* if not a geographic product reset screen center */
        if(msg_type != GEOGRAPHIC_PRODUCT
           || overlay_flag == FALSE     /*  IS THIS DESIRABLE??? */
                                        ) {
            sd2->x_center_offset = 0;
            sd2->y_center_offset = 0;
        }
                    
        /* and display product */
        plot_image(SCREEN_2, TRUE);
    
        sd2->last_plotted = TRUE;
        
    } /* end SCREEN_2 */


    if(selected_screen == SCREEN_3) {
        
        if(screen_3 == NULL) 
            open_display_screen(SCREEN_3);
/* TEST - Will the window manager allow us to force the existing */
/*                window to the top?  Many do not. Are we lucky?????? */
        else {
           XRaiseWindow( XtDisplay(dshell3), XtWindow(dshell3) );
        }

        if(overlay_flag == FALSE)
             set_prod_resolution(selected_screen);

        /* if not a geographic product reset screen center */
        if(msg_type != GEOGRAPHIC_PRODUCT
           || overlay_flag == FALSE     /*  IS THIS DESIRABLE??? */
                                        ) {
            sd3->x_center_offset = 0;
            sd3->y_center_offset = 0;
        }
                    
        /* and display product */
        plot_image(SCREEN_3, TRUE);
        
        sd3->last_plotted = TRUE;
        
    } /* end SCREEN_3 */
    
    /* reset animation information */
    reset_elev_series(selected_screen, ANIM_FULL_INIT);
    reset_time_series(selected_screen, ANIM_FULL_INIT);
    reset_auto_update(selected_screen, ANIM_FULL_INIT);


    /* make sure that now that we have a window to display stuff in, 
     * the buttons for selecting which window to display in are on */
    XtSetSensitive(s1_radio, True); 
    XtSetSensitive(s2_radio, True); 
    XtSetSensitive(s3_radio, True);

    XtSetSensitive(screen_radio_label, True);
    
} /*  end select_layer_or_packet */






void packetselectall_callback(Widget w,XtPointer client_data, XtPointer call_data)
{
  Prod_header *hdr;
  Graphic_product *gp;

    hdr = (Prod_header *)(sd->icd_product);
    gp  = (Graphic_product *)(sd->icd_product + 96);
    
    /*  Fixed overlay issues with 56 USP, 57 DHR, and 108 DPA */
    /*          DPA still requires special handling due to multiple  */
    /*          2D array packets */
    if(

         gp->prod_code == 81) {
        
        select_layer_or_packet(w, client_data, call_data);
        
        
        
    } else {

        select_all_packets(w, client_data, call_data);
        
    }

} /* end packetselectall_callback */



    
void select_all_packets(Widget w,XtPointer client_data, XtPointer call_data) {
        
  Prod_header *hdr;
  Graphic_product *gp;
  int *type_ptr, msg_type;

/*  DEBUG */
/* fprintf(stderr,"DEBUG - Enter select_all_packets()\n"); */

    /* set things to display everyting in the product */
    /* note: during display everything but the tab is displayed */
    sd->packet_select_type = SELECT_ALL;

    /* if there is a GAB, display it from the beginning */
    sd->gab_page = 1;

/* DEBUG */
/* if(sd->icd_product == NULL) */
/*     fprintf(stderr,"DEBUG - select_all_packets - product not loaded\n"); */
    
  hdr = (Prod_header *)(sd->icd_product);
  gp = (Graphic_product *) (sd->icd_product+ 96);
  type_ptr = assoc_access_i(msg_type_list, hdr->g.prod_id);
/* DEBUG */
/* if(type_ptr == NULL) */
/* fprintf(stderr,"DEBUG - select_all_packets - msg type not set for id %d\n", */
/* hdr->g.prod_id); */

  msg_type = *type_ptr;

  /* if it's not already open, open a screen to display stuff on */
  if(selected_screen == SCREEN_1) {
      if(screen_1 == NULL) 
          open_display_screen(SCREEN_1);
/* TEST - Will the window manager allow us to force the existing */
/*                window to the top?  Many do not. Are we lucky?????? */
      else {
           XRaiseWindow( XtDisplay(dshell1), XtWindow(dshell1) );
      }


      if(overlay_flag == FALSE)
          set_prod_resolution(selected_screen);

      /* if not a geographic product reset screen center */
      if(msg_type != GEOGRAPHIC_PRODUCT
         || overlay_flag == FALSE     
                                      ) {
          sd1->x_center_offset = 0;
          sd1->y_center_offset = 0;
      }
          
      /* and display product */
      plot_image(SCREEN_1, TRUE);

      sd1->last_plotted = TRUE;

  } /* end SCREEN_1 */
  

  if(selected_screen == SCREEN_2) {
    
      if(screen_2 == NULL) 
          open_display_screen(SCREEN_2);
/* TEST - Will the window manager allow us to force the existing */
/*                window to the top?  Many do not. Are we lucky?????? */
      else {
           XRaiseWindow( XtDisplay(dshell2), XtWindow(dshell2) );
      }

      if(overlay_flag == FALSE)
          set_prod_resolution(selected_screen);

      /* if not a geographic product reset screen center */
      if(msg_type != GEOGRAPHIC_PRODUCT
         || overlay_flag == FALSE     
                                      ) {
          sd2->x_center_offset = 0;
          sd2->y_center_offset = 0;
      }
          
      /* and display product */
      plot_image(SCREEN_2, TRUE);
      
      sd2->last_plotted = TRUE;
      
  } /* end SCREEN_2 */
 
  
  if(selected_screen == SCREEN_3) {
    
      if(screen_3 == NULL) 
          open_display_screen(SCREEN_3);
/* TEST - Will the window manager allow us to force the existing */
/*                window to the top?  Many do not. Are we lucky?????? */
      else {
        
           XRaiseWindow( XtDisplay(dshell3), XtWindow(dshell3) );
      }

      if(overlay_flag == FALSE)
          set_prod_resolution(selected_screen);

      /* if not a geographic product reset screen center */
      if(msg_type != GEOGRAPHIC_PRODUCT
         || overlay_flag == FALSE     
                                      ) {
          sd3->x_center_offset = 0;
          sd3->y_center_offset = 0;
      }
          
      /* and display product */
      plot_image(SCREEN_3, TRUE);
      
      sd3->last_plotted = TRUE;
      
  } /* end SCREEN_3 */

    
    /* reset animation information */
    reset_elev_series(selected_screen, ANIM_FULL_INIT);
    reset_time_series(selected_screen, ANIM_FULL_INIT);
    reset_auto_update(selected_screen, ANIM_FULL_INIT);

    /* make sure that now that we have a window to display stuff in, 
     * the buttons for selecting which window to display in are on */
    XtSetSensitive(s1_radio, True); 
    XtSetSensitive(s2_radio, True); 
    XtSetSensitive(s3_radio, True);

    XtSetSensitive(screen_radio_label, True);

    
} /*  end select_all_packets */








void get_component_subtype(char *desc,  int comp_code, int index )
{

  RPGP_product_t *generic_product;

  RPGP_radial_t *radial_comp=NULL;
  RPGP_grid_t   *grid_comp=NULL;
  RPGP_area_t   *area_comp=NULL;
  RPGP_text_t   *text_comp=NULL;
  RPGP_table_t  *table_comp=NULL;
  RPGP_event_t  *event_comp=NULL;
  int sub_type, r;
  char sub_type_str[30], sub_type_str2[10];
  size_t t_len;
  char title_str[51];

        
    generic_product = (RPGP_product_t *)sd->generic_prod_data;

    
    switch ( comp_code) {
        case 2801:
            radial_comp = (RPGP_radial_t *)generic_product->components[index];
            /*  NEED TO LIMIT STRING TO 50 CHARS */
            t_len = strlen(radial_comp->description);
            /*  title_str must be no longer than 39, for output to be 50 max */
            if(t_len > 39) t_len = 39;
            for(r=0;r<=50;r++) title_str[r] = '\0';
            strncpy(title_str, radial_comp->description, t_len);
            sprintf(desc, " %s  Elev:%4.1f", title_str,
                                        radial_comp->radials[1].elevation);
            break;
        case 2802:
            grid_comp = (RPGP_grid_t *)generic_product->components[index];
            sub_type = grid_comp->grid_type;
            if(sub_type==RPGP_GT_ARRAY)
                strcpy(sub_type_str, "Non-Geographical Array");
            else if(sub_type==RPGP_GT_EQUALLY_SPACED)
                strcpy(sub_type_str, "Flat Equally Spaced Grid");
            else if(sub_type==RPGP_GT_LAT_LON)
                strcpy(sub_type_str, "Equally Spaced Lat Lon Grid");
            else if(sub_type==RPGP_GT_POLAR)
                strcpy(sub_type_str, "Rotated Polar Grid");
            sprintf(desc, "%s", sub_type_str);
            break;
        case 2803:
            area_comp = (RPGP_area_t *)generic_product->components[index];
            sub_type = RPGP_AREA_TYPE(area_comp->area_type);
            if(sub_type==RPGP_AT_POINT)
                strcpy(sub_type_str, "Geographical Point");
            else if(sub_type==RPGP_AT_AREA)
                strcpy(sub_type_str, "Geographical Area");
            else if(sub_type==RPGP_AT_POLYLINE)
                strcpy(sub_type_str, "Geographical Polyline");
            sub_type = RPGP_LOCATION_TYPE(area_comp->area_type);
            if(sub_type==RPGP_LATLON_LOCATION)
                strcpy(sub_type_str2, "(LAT LON)");
            else if(sub_type==RPGP_XY_LOCATION)
                strcpy(sub_type_str2, "(XY)");
            else if(sub_type==RPGP_AZRAN_LOCATION)
                strcpy(sub_type_str2, "(AZRAN)");
            sprintf(desc, "%s %s", sub_type_str, sub_type_str2);
            break;
        case 2804:
            text_comp = (RPGP_text_t *)generic_product->components[index];
            sprintf(desc, "     ");
            break;
        case 2805:
            table_comp = (RPGP_table_t *)generic_product->components[index];
           /*  NEED TO LIMIT STRING TO 50 chars */
            t_len = strlen(table_comp->title.text);
            /* title_str must be no longer than 43, for output to be 50 max */
            if(t_len > 43) t_len = 43;
            for(r=0;r<=50;r++) title_str[r] = '\0';
            strncpy(title_str, table_comp->title.text, t_len);
            sprintf(desc, "Title: %s", title_str);
            break;
        case 2806:
            event_comp = (RPGP_event_t *)generic_product->components[index];
            sprintf(desc, "     ");
            break;
        default:
            break;
    } /*  end switch */
    



} /*  end get_component_subtype */









int transfer_packet_code(unsigned int val) 
{
    /* take the packet code from the packet code selection menu and
       return the proper enumeration index */
  
    if(val==0x0802) return(50);
    else if(val==0x0E03) return(51);
    else if(val==0x3501) return(52);
    else if(val==0xAF1F) return(53);
    else if(val==0xBA07) return(54);
    else if(val==0xBA0F) return(55);
    else if(val==2801)   return(41);
    else if(val==2802)   return(42);
    else if(val==2803)   return(43);
    else if(val==2804)   return(44);
    else if(val==2805)   return(45);
    else if(val==2806)   return(46);
    else return(val);
    
} /*  end transfer_packet_code */






/* The resolution logic moved from the file select function */
/* function is only called if the overlay_flag is FALSE just*/
/* prior to calling plot_image()                            */

/* Generic radial products do not use this method of        */
/* obtaining the product resolion.  A temporary fix is to   */
/* reset the sd->resolution field during display. This is   */
/* required so overlay products can get the correct resolution*/
void set_prod_resolution(int screen_num)
{
int *res;   
Prod_header *hdr;
    
    hdr = (Prod_header *)(sd->icd_product);

    /* set the resolution */
    res = assoc_access_i(product_res, hdr->g.prod_id);
  
    if(verbose_flag)
        fprintf(stderr, "got product resolution\n");



    /* save the loaded resolution in the screen data structure */

    if(screen_num == SCREEN_1) { 
        if(res != NULL)
            sd1->resolution = *res;
        else
            sd1->resolution = 0;
    }

    if(screen_num == SCREEN_2) {
        if(res != NULL)
            sd2->resolution = *res;
        else
            sd2->resolution = 0;
    }

    if(screen_num == SCREEN_3) {
        if(res != NULL)
            sd3->resolution = *res;
        else
            sd3->resolution = 0;
    }


} /*  end set_prod_resolution */

